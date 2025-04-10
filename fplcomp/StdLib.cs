static class StdLib {

    public static AstFunction print  = new(Location.unknown, "print", [], null , [], null);
    public static AstFunction malloc = new(Location.unknown, "malloc", [], null , [], null);
    public static AstFunction memcpy = new(Location.unknown, "memcpy", [], null , [], null);
    public static AstFunction free   = new(Location.unknown, "free", [], null , [], null);
    public static AstFunction abort  = new(Location.unknown, "abort", [], null , [], null);
    public static AstFunction stringConcat  = new(Location.unknown, "stringConcat", [], null , [], null);
    public static AstFunction strcmp   = new(Location.unknown, "strcmp", [], null , [], null);
    public static AstFunction strequals= new(Location.unknown, "strequals", [], null , [], null);
    public static AstFunction resumeTask = new(Location.unknown, "ResumeTask", [], null , [], null);
    public static AstFunction success = new(Location.unknown, "Success", [], null , [], null);

    public static readonly FieldSymbol rcField = new("refcount", IntType.Instance, true);
    public static readonly FieldSymbol lengthField = new("length", IntType.Instance, true);
    public static readonly FieldSymbol objectTypeField = new("objectType", IntType.Instance, true);

    public static void AddSymbols(AstBlock scope) {
        // Print is currently handled as a special case in the type checker. So we don't need to add it to the symbol table.

        Type type_int_str_to_ptr = FunctionType.MakeFunctionType([IntType.Instance, StringType.Instance], PointerType.Instance);
        scope.AddSymbol(Location.unknown, new FunctionSymbol("malloc", type_int_str_to_ptr, malloc));

        Type ptr_ptr_int_to_unit = FunctionType.MakeFunctionType([PointerType.Instance, PointerType.Instance, IntType.Instance], UnitType.Instance);
        scope.AddSymbol(Location.unknown, new FunctionSymbol("memcpy", ptr_ptr_int_to_unit, memcpy));

        Type ptr_to_unit = FunctionType.MakeFunctionType([PointerType.Instance], UnitType.Instance);
        scope.AddSymbol(Location.unknown, new FunctionSymbol("free", ptr_to_unit, free));

        Type int_to_unit = FunctionType.MakeFunctionType([IntType.Instance], UnitType.Instance);
        scope.AddSymbol(Location.unknown, new FunctionSymbol("abort", int_to_unit, abort));

        Type array_string = ArrayType.MakeArrayType(StringType.Instance);
        Type array_string_int_to_string = FunctionType.MakeFunctionType([array_string, IntType.Instance], StringType.Instance);
        scope.AddSymbol(Location.unknown, new FunctionSymbol("stringConcat", array_string_int_to_string, stringConcat));

        Type str_str_to_int = FunctionType.MakeFunctionType([StringType.Instance,StringType.Instance], IntType.Instance);
        scope.AddSymbol(Location.unknown, new FunctionSymbol("strcmp", str_str_to_int, strcmp));

        Type str_str_to_bool = FunctionType.MakeFunctionType([StringType.Instance,StringType.Instance], BoolType.Instance);
        scope.AddSymbol(Location.unknown, new FunctionSymbol("strequals", str_str_to_bool, strequals));

        Type array_int = ArrayType.MakeArrayType(IntType.Instance);
        Type array_int_to_int = FunctionType.MakeFunctionType([array_int], IntType.Instance);
        scope.AddSymbol(Location.unknown, new FunctionSymbol("ResumeTask", array_int_to_int, resumeTask));

        Type voidFunc = FunctionType.MakeFunctionType([], UnitType.Instance);
        scope.AddSymbol(Location.unknown, new FunctionSymbol("Success", voidFunc, success));
        scope.AddSymbol(Location.unknown, new FunctionSymbol("_start", voidFunc, success));

        rcField.offset = -4;
        lengthField.offset = -4;
        objectTypeField.offset = -8;
    }
}