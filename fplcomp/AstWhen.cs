class AstWhen(Location location, AstExpression astExpr, List<AstWhenClause> clauses) : AstStatement(location) {
    private AstExpression astExpr = astExpr;
    public List<AstWhenClause> clauses = clauses;


    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "WHEN");
        foreach(AstWhenClause clause in clauses) {
            clause.Print(indent + 1);
        }
    }

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext) {
        HashSet<int> seen = [];
        astExpr.TypeCheckRvalue(scope, pathContext);
        if (astExpr.type!=IntType.Instance && astExpr.type!=CharType.Instance && astExpr.type!=StringType.Instance &&
            astExpr.type!=ErrorType.Instance)
            Log.Error(location,$"Invalid type for when statement {astExpr.type}");

        List<PathContext> pathContexts = [];
        foreach(AstWhenClause clause in clauses) {
            pathContexts.Add( clause.TypeCheck(scope, pathContext) );
            foreach(AstExpression c in clause.cond) {
                astExpr.type.CheckAssignableFrom(c);
                if (c.HasKnownIntValue()) {
                    int value = c.GetKnownIntValue();
                    if (seen.Contains(value))
                        Log.Error(c.location, $"Duplicate when clause {value}");
                    seen.Add(value);
                }
            }
        }
        return PathContext.Merge(pathContexts);
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
                    if (s.type==StringType.Instance) {
                        func.Add(new InstrMov(RegisterSymbol.registers[1], s));
                        func.Add(new InstrMov(RegisterSymbol.registers[2], expr));
                        func.Add(new InstrCall(StdLib.strequals,2, IntType.Instance));
                        func.Add(new InstrBra(AluOp.NE_I, RegisterSymbol.registers[8], RegisterSymbol.zero, branchLabel));
                    }
                    else
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