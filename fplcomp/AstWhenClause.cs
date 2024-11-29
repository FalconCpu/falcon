class AstWhenClause(Location location, List<AstExpression> cond, AstBlock parent) : AstBlock(location, parent) {
    public List<AstExpression> cond = cond;   

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "CLAUSE");
        foreach(AstExpression c in cond)
            c.Print(indent + 1);
        foreach(AstStatement stmt in statements) {
            stmt.Print(indent + 1);
        }
    }

    public override void TypeCheck(AstBlock scope) {
        foreach(AstExpression c in cond) {
            c.TypeCheckRvalue(scope);
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