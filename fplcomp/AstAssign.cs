class AstAssign(Location location, TokenKind op, AstExpression lhs, AstExpression rhs) : AstStatement(location) {
    public TokenKind op = op;
    public AstExpression lhs = lhs;
    public AstExpression rhs = rhs;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"ASSIGN {op}");
        lhs.Print(indent + 1);
        rhs.Print(indent + 1);
    }

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext)
    {
        if (pathContext.unreachable)
            Log.Error(location, "Statement is unreachable");

        rhs.TypeCheckRvalue(scope, pathContext);
        lhs.TypeCheckLvalue(scope, pathContext);
        lhs.type.CheckAssignableFrom(rhs);

        if (op!=TokenKind.Eq && lhs.type!=IntType.Instance)
            Log.Error(location, $"{op} only supported for integer operands");
        
        if (lhs is AstIdentifier identifier)
            pathContext.Initialize(identifier.symbol);
        return pathContext;
    }

    public override void CodeGen(AstFunction func) {
        Symbol rhs = this.rhs.CodeGenRvalue(func);
        AluOp aluOp = op switch {
            TokenKind.Eq => AluOp.UNDEFINED,
            TokenKind.PlusEq => AluOp.ADD_I,
            TokenKind.MinusEq => AluOp.SUB_I,
            TokenKind.StarEq => AluOp.MUL_I,
            TokenKind.SlashEq => AluOp.DIV_I,
            _ => throw new Exception("Invalid assign op")
        };


        lhs.CodeGenLvalue(func, aluOp, rhs);
    }

}