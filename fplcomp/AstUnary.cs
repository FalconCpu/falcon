class AstUnary(Location location, TokenKind op, AstExpression expr) : AstExpression(location) {
    public TokenKind op = op;
    public AstExpression expr = expr;


    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "UNARY" + op + " (" + type + ")");
        expr.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        switch(op) {
            case TokenKind.Minus: {
                expr.TypeCheckRvalue(scope);
                if (expr.type is not IntType)
                    Log.Error(location, $"Unary minus requires an integer expression");
                SetType(expr.type);
                break;
            }

            default:
                throw new NotImplementedException($"Unary operator {op}");

        }
    }
    
    public override Symbol CodeGenRvalue(AstFunction func)    {
        switch(op) {
            case TokenKind.Minus: {
                Symbol rhs = expr.CodeGenRvalue(func);
                Symbol ret = func.NewTemp(type);
                func.Add(new InstrAlu(ret, AluOp.SUB_I, RegisterSymbol.registers[0], rhs));
                return ret;
            }

            default:
                throw new NotImplementedException($"Unary operator {op}");

        }


    }
}
