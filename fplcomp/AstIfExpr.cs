class AstIfExpr(Location location, AstExpression cond, AstExpression then, AstExpression els) : AstExpression(location) {
    public AstExpression cond = cond;
    public AstExpression then = then;
    public AstExpression els = els;


    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "IF EXPR ({type})");
        cond.Print(indent + 1);
        then.Print(indent + 1);
        els.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        throw new NotImplementedException();
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        throw new NotImplementedException();
    }
}