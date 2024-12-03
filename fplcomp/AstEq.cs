class AstEq(Location location, TokenKind kind, AstExpression left, AstExpression right) : AstExpression(location) {
    private readonly TokenKind kind = kind;
    private readonly AstExpression left = left;
    private readonly AstExpression right = right;

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "EQ " + kind+ " (" + type + ")");
        left.Print(indent + 1);
        right.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope, PathContext pathContext) {
        throw new NotImplementedException();
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        throw new NotImplementedException();
    }

}