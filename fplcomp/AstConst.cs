
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

        Symbol newSym;
        if (initializer.HasKnownIntValue())
            newSym = new ConstantSymbol(identifier.name, initializer.type, initializer.GetKnownIntValue());
        else if (initializer.HasKnownConstObjectValue())
            newSym = new ConstObjectAliasSymbol(identifier.name, initializer.GetKnownConstObjectValue());
        else {
            Log.Error(location, "Initializer is not a constant");
            newSym = new ConstantSymbol(identifier.name, ErrorType.Instance, 0);
        }

        scope.AddSymbol(location, newSym);
    }

    public override void CodeGen(AstFunction func) {

    }
}