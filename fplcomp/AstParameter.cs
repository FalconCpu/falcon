class AstParameter(Location location, TokenKind kind, AstIdentifier name, AstType astType, bool isVariadic) : Ast(location) {
    public AstIdentifier name = name;
    public AstType astType = astType;
    public TokenKind kind = kind;
    public bool isVariadic = isVariadic;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "PARAM " + kind + " " + name.name);
    }

    // Generate a symbol for the parameter. If this field is the last parameter in a variadic function,
    // then make its type be Array<T> where T is the type of the variadic parameter.
    public Symbol GenerateSymbol(AstBlock scope) {
        Type type = astType.ResolveAsType(scope);
        if (isVariadic)
            type = ArrayType.MakeArrayType(type);
        Symbol sym = new VariableSymbol(name.name, type, false);
        return sym;
    }

    public Symbol GenerateFieldSymbol(AstBlock scope) {
        // Generate symbols for constructor of a class. If the field is marked VAL/VAR then it's a symbol for a field.
        // Otherwise it's a parameter for the constructor
        Symbol sym;
        if (kind==TokenKind.Var || kind==TokenKind.Val)
            sym = new FieldSymbol(name.name, astType.ResolveAsType(scope), kind==TokenKind.Var);
        else
            sym = new VariableSymbol(name.name, astType.ResolveAsType(scope), false);

        return sym;
    }

}