
class AstTop() : AstFunction(Location.unknown, "_start", [], null, [], null) {

    public static readonly TypeSymbol stringTypeSymbol = new("String", StringType.Instance);
    
    public static readonly List<FieldSymbol> globalVariables = [];

    // Generate the parameter symbols.
    public VariableSymbol arg1 = new("_arg1", IntType.Instance, false);
    public VariableSymbol arg2 = new("_arg2", IntType.Instance, false);

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "TOP");
        foreach (AstStatement statement in statements)
            statement.Print(indent + 1);
    }

    public override void IdentifyFunctions(AstBlock scope) {
        allFunctions.Add(this);

        // Create two parameters to pass through to the main function
        parameters.Add(arg1);
        AddSymbol(location, arg1);
        parameters.Add(arg2);
        AddSymbol(location, arg2);
        returnType = IntType.Instance;

    }

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext) {
        // allFunctions.Add(this);

        IdentifyFunctions(this);
        foreach (AstStatement statement in statements)
            if (statement is AstBlock blk)
                blk.IdentifyFunctions(this);


        foreach (AstStatement statement in statements)
            statement.TypeCheck(this, pathContext);
        return pathContext;
    }

    public static void AddGlobalVariable(FieldSymbol global) {
        global.isGlobal = true;
        global.offset = globalVariables.Count * 4;
        globalVariables.Add(global);
        if (globalVariables.Count==1000)
            throw new Exception("Currently compiler only supports 1000 global variables");
    }

    public AstTop AddPredefinedSymbols() {
        AddSymbol(Location.unknown, new TypeSymbol("Int", IntType.Instance));
        AddSymbol(Location.unknown, new TypeSymbol("Bool", BoolType.Instance));
        AddSymbol(Location.unknown, new TypeSymbol("Char", CharType.Instance));
        AddSymbol(Location.unknown, new TypeSymbol("Short", ShortType.Instance));
        AddSymbol(Location.unknown, stringTypeSymbol);
        AddSymbol(Location.unknown, new TypeSymbol("Real", RealType.Instance));
        AddSymbol(Location.unknown, new TypeSymbol("Unit", UnitType.Instance));
        AddSymbol(Location.unknown, new TypeSymbol("Pointer", PointerType.Instance));
        AddSymbol(Location.unknown, new ConstantSymbol("true", BoolType.Instance, 1));
        AddSymbol(Location.unknown, new ConstantSymbol("false", BoolType.Instance, 0));
        AddSymbol(Location.unknown, new ConstantSymbol("null", NullType.Instance, 0));
        AddSymbol(Location.unknown, new TypeSymbol("Any", AnyType.Instance));
        StdLib.AddSymbols(this);

        // Bit of a hack - but this seems the cleanest place to put it. 
        // Force the offset of the 'refcount' symbol 

        return this;
    }

}