
abstract class AstExpression(Location location) : Ast(location) {
    public Type type = UndefinedType.Instance;

    public void SetType(Type newType) {
        if (type != UndefinedType.Instance)
            throw new ArgumentException($"Type already set to {type}");
        type = newType;
    }

    public void SetError() {
        type = ErrorType.Instance;
    }

    public void SetError(Location location, string message) {
        Log.Error(location, message);
        type = ErrorType.Instance;
    }

    public abstract void TypeCheckRvalue(AstBlock scope);

    public virtual void TypeCheckLvalue(AstBlock scope) {
        Log.Error(location, "Not an lvalue");
    }

    public abstract Symbol CodeGenRvalue(AstFunction func);

    public virtual void CodeGenLvalue(AstFunction func, Symbol value) {
        Log.Error(location, "Not an lvalue");
    }

    public virtual void CodeGenBool(AstFunction func, Label labTrue, Label labFalse) {
        Symbol s = CodeGenRvalue(func);
        func.Add( new InstrBra(AluOp.NE_I, s, RegisterSymbol.zero, labTrue));
        func.Add( new InstrJmp(labFalse));
    }

}