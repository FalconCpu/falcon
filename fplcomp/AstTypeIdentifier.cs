
class AstTypeIdentifier(Location location, string name, List<AstType> astTypeArgs) : AstType(location) {
    public string name = name;
    public List<AstType> astTypeArgs = astTypeArgs;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "TYPE " + name);
    }

    public override Type ResolveAsType(AstBlock scope)
    {
        Symbol? sym = Type.predefindedScope.GetSymbol(name) ??
                     scope.GetSymbol(name);

        List<Type> typeArgs = astTypeArgs.Select(it=>it.ResolveAsType(scope)).ToList();

        if (sym==null)
           return ErrorType.MakeErrorType(location, $"Undefined symbol '{name}'");
        
        if (sym is TypeSymbol ts) {
            if (ts.type is GenericClassType gct) {
                // We have a (possibly generic) class - look to see if it has type parameters
                if (gct.typeParameters.Count!=typeArgs.Count) {
                    Log.Error(location, $"Type {name} takes {gct.typeParameters.Count} type arguments, but {typeArgs.Count} were given");
                    return ErrorType.Instance;
                }
                return ClassType.MakeClassType(gct, typeArgs);

            } else {
                if (typeArgs.Count > 0)
                    Log.Error(location,$"Type {ts} does not take type arguments");
            }
            return sym.type;
        } else
            return ErrorType.MakeErrorType(location, $"Symbol '{name}' is not a type");
    }
}