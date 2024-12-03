
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

    public abstract void TypeCheckRvalue(AstBlock scope, PathContext pathContext);

    public virtual void TypeCheckLvalue(AstBlock scope, PathContext pathContext) {
        Log.Error(location, "Not an lvalue");
    }

    public virtual Tuple<PathContext,PathContext> TypeCheckBool(AstBlock scope, PathContext pathContext) {
        TypeCheckRvalue(scope, pathContext);
        BoolType.Instance.CheckAssignableFrom(this);
        return new Tuple<PathContext,PathContext>(pathContext.Clone(),pathContext.Clone());
    }

    public abstract Symbol CodeGenRvalue(AstFunction func);

    public virtual bool HasKnownIntValue() => false;
    public virtual int  GetKnownIntValue() => throw new ArgumentException("Attempt to get Int value from non-int"); 

    public virtual bool HasKnownConstObjectValue() => false;
    public virtual ConstObjectSymbol GetKnownConstObjectValue() => throw new ArgumentException("Attempt to get ConstObject value from non-ConstObject");

    public virtual void CodeGenLvalue(AstFunction func, AluOp op, Symbol value) {
        Log.Error(location, "Not an lvalue");
    }

    public virtual void CodeGenBool(AstFunction func, Label labTrue, Label labFalse) {
        Symbol s = CodeGenRvalue(func);
        func.Add( new InstrBra(AluOp.NE_I, s, RegisterSymbol.zero, labTrue));
        func.Add( new InstrJmp(labFalse));
    }

}