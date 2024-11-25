
class AstTypeIdentifier(Location location, string name) : AstType(location) {
    public string name = name;

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "TYPE " + name);
    }

    public override Type ResolveAsType(AstBlock scope)
    {
        Symbol? sym = Type.predefindedScope.GetSymbol(name) ??
                     scope.GetSymbol(name);

        if (sym==null)
           return ErrorType.MakeErrorType(location, $"Undefined symbol '{name}'");
        if (sym is TypeSymbol)
            return sym.type;
        return ErrorType.MakeErrorType(location, $"Symbol '{name}' is not a type");
    }
}