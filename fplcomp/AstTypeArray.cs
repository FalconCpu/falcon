class AstTypeArray(Location location, AstType elementType) : AstType(location) {
    public AstType elementType = elementType;

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "ARRAY");
        elementType.Print(indent + 1);
    }

    public override Type ResolveAsType(AstBlock scope) {
        Type eType = elementType.ResolveAsType(scope);
        return ArrayType.MakeArrayType(eType);
    }
}


