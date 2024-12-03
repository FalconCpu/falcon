class AstFuncCallStatement(Location location, AstExpression expr) : AstStatement(location) {
    public AstExpression expr = expr;


    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "FUNC CALL");
        expr.Print(indent + 1);
    }

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext) {
        expr.TypeCheckRvalue(scope, pathContext);
        return pathContext;
    }
    
    public override void CodeGen(AstFunction func) {
        expr.CodeGenRvalue(func);
    }

}
