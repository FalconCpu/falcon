class AstTypeNullable(Location location, AstType type) : AstType(location) {
    public AstType type = type; 

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "NULLABLE");
        type.Print(indent + 1);
    }

    public override Type ResolveAsType(AstBlock scope) {
        Type t = type.ResolveAsType(scope);
        return NullableType.MakeNullableType(t);
    }
}