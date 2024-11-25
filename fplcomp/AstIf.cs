class AstIf(Location location, List<AstIfClause> clauses) : AstStatement(location) {
    public List<AstIfClause> clauses = clauses;


    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "IF");
        foreach(AstIfClause clause in clauses) {
            clause.Print(indent + 1);
        }
    }

    public override void TypeCheck(AstBlock scope) {
        foreach(AstIfClause clause in clauses)
            clause.TypeCheck(scope);
    }

    public override void CodeGen(AstFunction func)    {
        Label endLabel = func.NewLabel();
        List<Label> branchLabels = [];
        Label nextLabel = func.NewLabel();
        foreach(AstIfClause clause in clauses) {
            Label branchLabel = func.NewLabel();
            branchLabels.Add(branchLabel);
            if (clause.cond!=null)
                clause.cond.CodeGenBool(func, branchLabel, nextLabel);
            else
                func.Add(new InstrJmp(branchLabel));
            func.Add(new InstrLabel(nextLabel));
            nextLabel = func.NewLabel();
        }
        func.Add(new InstrJmp(endLabel));

        for(int i=0; i<clauses.Count; i++) {
            AstIfClause clause = clauses[i];
            func.Add(new InstrLabel(branchLabels[i]));
            clause.CodeGen(func);
            func.Add(new InstrJmp(endLabel));
        }
        func.Add(new InstrLabel(endLabel));

    }


}