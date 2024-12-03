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

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext) {
        foreach(AstExpression c in cond) {
            c.TypeCheckRvalue(scope, pathContext);
        }

        pathContext = pathContext.Clone();
        foreach(AstStatement stmt in statements) {
            stmt.TypeCheck(this, pathContext);
        }
        return pathContext;
    }

    public override void CodeGen(AstFunction func)    {
        foreach(AstStatement stmt in statements)
            stmt.CodeGen(func);
    }
}