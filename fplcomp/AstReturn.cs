class AstReturn (Location location, AstExpression? expr) : AstStatement(location) {
    public AstExpression? expr = expr;

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "RETURN");
        expr?.Print(indent + 1);
    }

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext) {
        if (pathContext.unreachable)
            Log.Error(location, "Statement is unreachable");

        AstFunction? enclosingFunction = scope.FindEnclosingFunction();
        if (enclosingFunction == null) {
            Log.Error(location, "return outside function");
            return pathContext;
        }

        if (expr != null) {
            expr.TypeCheckRvalue(scope, pathContext);
            enclosingFunction.returnType.CheckAssignableFrom(expr);
        } else {
            if (enclosingFunction.returnType != UnitType.Instance)
                Log.Error(location, "return with no value in non-void function");
        }

        pathContext.SetUnreachable();
        return pathContext;
    }

    public override void CodeGen(AstFunction func) {
        if (expr != null) {
            Symbol sym = expr.CodeGenRvalue(func);
            func.Add(new InstrMov(RegisterSymbol.registers[8], sym));
        }
        func.Add(new InstrJmp(func.endLabel));
    }

}