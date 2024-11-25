class AstUnary(Location location, TokenKind op, AstExpression expr) : AstExpression(location) {
    public TokenKind op = op;
    public AstExpression expr = expr;


    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "UNARY" + op + " (" + type + ")");
        expr.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        throw new NotImplementedException();
    }
    
    public override Symbol CodeGenRvalue(AstFunction func)    {
        throw new NotImplementedException();
    }
}
