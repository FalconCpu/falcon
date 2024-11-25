class AstParameter(Location location, TokenKind kind, AstIdentifier name, AstType astType) : Ast(location) {
    public AstIdentifier name = name;
    public AstType astType = astType;
    public TokenKind kind = kind;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "PARAM " + kind + " " + name.name);
    }

    public Symbol GenerateSymbol(AstBlock scope) {
        Symbol sym = new VariableSymbol(name.name, astType.ResolveAsType(scope), false, false);
        return sym;
    }

    public Symbol GenerateFieldSymbol(AstBlock scope) {
        // Generate symbols for constructor of a class. If the field is marked VAL/VAR then it's a symbol for a field.
        // Otherwise it's a parameter for the constructor
        Symbol sym;
        if (kind==TokenKind.Var || kind==TokenKind.Val)
            sym = new FieldSymbol(name.name, astType.ResolveAsType(scope), kind==TokenKind.Var);
        else
            sym = new VariableSymbol(name.name, astType.ResolveAsType(scope), false, false);

        return sym;
    }

}