
class AstTop() : AstFunction(Location.unknown, "_start", [], null, null) {
    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "TOP");
        foreach (AstStatement statement in statements)
            statement.Print(indent + 1);
    }

    public override void TypeCheck(AstBlock scope) {
        allFunctions.Add(this);

        foreach (AstStatement statement in statements)
            statement.TypeCheck(this);
    }

    public AstTop AddPredefinedSymbols() {
        AddSymbol(Location.unknown, new TypeSymbol("Int", IntType.Instance));
        AddSymbol(Location.unknown, new TypeSymbol("Bool", BoolType.Instance));
        AddSymbol(Location.unknown, new TypeSymbol("Char", CharType.Instance));
        AddSymbol(Location.unknown, new TypeSymbol("String", StringType.Instance));
        AddSymbol(Location.unknown, new TypeSymbol("Real", RealType.Instance));
        AddSymbol(Location.unknown, new TypeSymbol("Unit", UnitType.Instance));
        return this;
    }

}