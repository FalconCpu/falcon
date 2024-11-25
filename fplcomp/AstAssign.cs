class AstAssign(Location location, AstExpression lhs, AstExpression rhs) : AstStatement(location) {
    public AstExpression lhs = lhs;
    public AstExpression rhs = rhs;

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "ASSIGN");
        lhs.Print(indent + 1);
        rhs.Print(indent + 1);
    }

    public override void TypeCheck(AstBlock scope)
    {
        rhs.TypeCheckRvalue(scope);
        lhs.TypeCheckLvalue(scope);
        lhs.type.CheckAssignableFrom(rhs);
    }

    public override void CodeGen(AstFunction func) {
        Symbol rhs = this.rhs.CodeGenRvalue(func);
        lhs.CodeGenLvalue(func, rhs);
    }

}