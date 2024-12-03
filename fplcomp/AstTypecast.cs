class AstTypecast(Location location, AstExpression astExpression, AstType targetType) : AstExpression(location) {
    public readonly AstExpression astExpression = astExpression;
    public readonly AstType targetType = targetType;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"Typecast ({type})");
        astExpression.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope, PathContext pathContext) {
        astExpression.TypeCheckRvalue(scope, pathContext);
        // Todo - are there any typecasts that cannot be allowed?

        SetType( targetType.ResolveAsType(scope) );
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        Symbol sym =  astExpression.CodeGenRvalue(func);
        Symbol ret = func.NewTemp(type);
        func.Add( new InstrMov(ret, sym) );
        return ret;
    }
}