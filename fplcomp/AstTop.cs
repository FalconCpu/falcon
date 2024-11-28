
class AstTop() : AstFunction(Location.unknown, "_start", [], null, null) {
    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "TOP");
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
        AddSymbol(Location.unknown, new TypeSymbol("Pointer", PointerType.Instance));
        AddSymbol(Location.unknown, new ConstantSymbol("true", BoolType.Instance, 1));
        AddSymbol(Location.unknown, new ConstantSymbol("false", BoolType.Instance, 0));
        AddSymbol(Location.unknown, new ConstantSymbol("null", NullType.Instance, 0));
        StdLib.AddSymbols(this);

        // Bit of a hack - but this seems the cleanest place to put it. 
        // Force the offset of the 'refcount' symbol 

        return this;
    }

}