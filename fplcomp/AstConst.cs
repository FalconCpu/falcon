
class AstConst(
    Location location,
    AstIdentifier identifier,
    AstType? astType,
    AstExpression initializer
) : AstStatement(location) {
    private readonly AstIdentifier  identifier  = identifier;
    private readonly AstType?       astType     = astType;
    private readonly AstExpression  initializer = initializer;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"CONST ");
        identifier.Print(indent + 1);
        astType?.Print(indent + 1);
        initializer?.Print(indent + 1);
    }

    public override void TypeCheck(AstBlock scope) {
        initializer.TypeCheckRvalue(scope);
        Symbol? value = initializer.GetConstValue();

        Symbol newSym;
        if (value==null) {
            Log.Error(location, "Initializer is not a constant");
            newSym = IntegerSymbol.AliasSymbol(identifier.name, ErrorType.Instance, 0);
        } else if (value is IntegerSymbol isym)
            newSym = IntegerSymbol.AliasSymbol(identifier.name, isym.type, isym.value);
        else if (value is StringLitSymbol slsym)
            throw new NotImplementedException("String literals as constants");
        else
            throw new NotImplementedException("Other constant types");
        scope.AddSymbol(location, newSym);
    }

    public override void CodeGen(AstFunction func) {

    }
}