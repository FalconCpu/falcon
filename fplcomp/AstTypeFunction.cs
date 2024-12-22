class AstTypeFunction(Location location, List<AstType> astArgs, AstType astRetType) : AstType(location) {
    public List<AstType> astArgs = astArgs;
    public AstType astRetType = astRetType;

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "Function");
        foreach (AstType arg in astArgs) {
            arg.Print(indent + 1);
        }
        astRetType.Print(indent + 1);
    }

    public override Type ResolveAsType(AstBlock scope) {
        List<Type> args = astArgs.Select(x => x.ResolveAsType(scope)).ToList();
        Type retType = astRetType.ResolveAsType(scope);
        return FunctionType.MakeFunctionType(args, retType);
    }
}


