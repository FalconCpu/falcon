class AstDeclaration(
    Location location,
    TokenKind kind,
    AstIdentifier identifier,
    AstType? astType,
    AstExpression? initializer
) : AstStatement(location) {

    private readonly TokenKind      kind        = kind;
    private readonly AstIdentifier  identifier  = identifier;
    private readonly AstType?       astType     = astType;
    public  readonly AstExpression? initializer = initializer;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"DECLARATION {kind}");
        identifier.Print(indent + 1);
        astType?.Print(indent + 1);
        initializer?.Print(indent + 1);
    }

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext) {
        initializer?.TypeCheckRvalue(scope, pathContext);

        if (pathContext.unreachable)
            Log.Error(location, "Statement is unreachable");

        // TODO - handle the type if present
        Type type = astType?.ResolveAsType(scope) ??
                    initializer?.type ?? ErrorType.MakeErrorType(identifier.location,$"Cannot determine type for '{identifier}'");

        bool isMutable = kind==TokenKind.Var;

        Symbol sym;
        if (scope is AstClass astClass) {
            // declaring a field of a class
            sym = new FieldSymbol(identifier.name, type, isMutable);
            astClass.classType.AddField((FieldSymbol)sym);
        } else if (scope is AstTop) {
            // declaring a global variable
            FieldSymbol fs = new FieldSymbol(identifier.name, type, isMutable, true);
            AstTop.AddGlobalVariable(fs);
            sym = fs;   
        } else {
            // declaring a local variable
            sym = new VariableSymbol(identifier.name, type, isMutable);
        }
        scope.AddSymbol(location, sym);
        identifier.SetSymbol(sym);

        if (initializer!=null)
            sym.type.CheckAssignableFrom(initializer);
        else
            pathContext.AddUninitialized(sym);
        return pathContext;
    }

    public override void CodeGen(AstFunction func) {
        if (initializer==null)
            return;
        Symbol rhs = initializer.CodeGenRvalue(func);
        identifier.CodeGenLvalue(func, AluOp.UNDEFINED, rhs);
    }
}