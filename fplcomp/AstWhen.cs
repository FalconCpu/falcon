class AstWhen(Location location, AstExpression astExpr, List<AstWhenClause> clauses) : AstStatement(location) {
    private AstExpression astExpr = astExpr;
    public List<AstWhenClause> clauses = clauses;


    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "WHEN");
        foreach(AstWhenClause clause in clauses) {
            clause.Print(indent + 1);
        }
    }

    public override void TypeCheck(AstBlock scope) {
        HashSet<int> seen = [];
        astExpr.TypeCheckRvalue(scope);
        foreach(AstWhenClause clause in clauses) {
            clause.TypeCheck(scope);
            foreach(AstExpression c in clause.cond) {
                if (c.HasKnownIntValue()) {
                    int value = c.GetKnownIntValue();
                    if (seen.Contains(value))
                        Log.Error(c.location, $"Duplicate when clause {value}");
                    seen.Add(value);
                }
            }
        }
    }

    public override void CodeGen(AstFunction func)    {
        Symbol expr = astExpr.CodeGenRvalue(func);

        Label endLabel = func.NewLabel();
        List<Label> branchLabels = [];
        foreach(AstWhenClause clause in clauses) {
            Label branchLabel = func.NewLabel();
            branchLabels.Add(branchLabel);
            if (clause.cond.Count > 0) {
                foreach(AstExpression c in clause.cond) {
                    Symbol s = c.CodeGenRvalue(func);
                    func.Add( new InstrBra(AluOp.EQ_I, s, expr, branchLabel));
                }
            } else {
                func.Add(new InstrJmp(branchLabel));
            }
        }
        func.Add(new InstrJmp(endLabel));

        for(int i=0; i<clauses.Count; i++) {
            AstWhenClause clause = clauses[i];
            func.Add(new InstrLabel(branchLabels[i]));
            clause.CodeGen(func);
            func.Add(new InstrJmp(endLabel));
        }
        func.Add(new InstrLabel(endLabel));
    }
}