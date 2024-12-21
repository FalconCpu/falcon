class AstIfClause(Location location, AstExpression? cond, AstBlock parent) : AstBlock(location, parent) {
    public AstExpression? cond = cond;   
    public PathContext pathContextOut = new();

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "CLAUSE");
        cond?.Print(indent + 1);
        foreach(AstStatement stmt in statements) {
            stmt.Print(indent + 1);
        }
    }

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext) {
        // Returns the path context if the condition is false
        // Sets the path context of the true path in pathContextOut
        
        Tuple<PathContext,PathContext> pc;
        if (cond != null)
            pc = cond.TypeCheckBool(scope, pathContext);
        else
            pc = Tuple.Create(pathContext, pathContext);
    
        pathContext = pc.Item1;
        foreach(AstStatement stmt in statements) {
            pathContext = stmt.TypeCheck(this, pathContext);
        }
        pathContextOut = pathContext;
        return pc.Item2;
    }

    public override void CodeGen(AstFunction func)    {
        foreach(AstStatement stmt in statements)
            stmt.CodeGen(func);
    }
}