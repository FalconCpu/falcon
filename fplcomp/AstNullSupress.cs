class AstNullSupress(Location location, AstExpression expr) : AstExpression(location) {
    public AstExpression expr = expr;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "NULL_SUPRESS");
        expr.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope, PathContext pathContext) {
        expr.TypeCheckRvalue(scope, pathContext);
        if (expr.type.IsErrorType()) {
            SetError();
            return;
        }

        if (expr.type is NullableType nt)
            SetType(nt.elementType);
        else
            SetError(location, "Null suppression can only be applied to nullable types");
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        return expr.CodeGenRvalue(func);
    }

}