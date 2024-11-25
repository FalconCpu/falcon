class AstIfClause(Location location, AstExpression? cond, AstBlock parent) : AstBlock(location, parent) {
    public AstExpression? cond = cond;   

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "CLAUSE");
        cond?.Print(indent + 1);
        foreach(AstStatement stmt in statements) {
            stmt.Print(indent + 1);
        }
    }

    public override void TypeCheck(AstBlock scope) {
        if (cond != null) {
            cond.TypeCheckRvalue(scope);
            BoolType.Instance.CheckAssignableFrom(cond);
        }

        foreach(AstStatement stmt in statements) {
            stmt.TypeCheck(this);
        }
    }

    public override void CodeGen(AstFunction func)    {
        foreach(AstStatement stmt in statements)
            stmt.CodeGen(func);
    }
}