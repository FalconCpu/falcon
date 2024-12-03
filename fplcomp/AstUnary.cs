class AstUnary(Location location, TokenKind op, AstExpression expr) : AstExpression(location) {
    public TokenKind op = op;
    public AstExpression expr = expr;


    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "UNARY" + op + " (" + type + ")");
        expr.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope, PathContext pathContext) {
        switch(op) {
            case TokenKind.Minus: {
                expr.TypeCheckRvalue(scope, pathContext);
                IntType.Instance.CheckAssignableFrom(expr);
                SetType(expr.type);
                break;
            }

            case TokenKind.Not: {
                expr.TypeCheckRvalue(scope, pathContext);
                BoolType.Instance.CheckAssignableFrom(expr);
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

            case TokenKind.Not: {
                Symbol rhs = expr.CodeGenRvalue(func);
                Symbol ret = func.NewTemp(type);
                func.Add(new InstrAlui(ret, AluOp.XOR_I, rhs, 1));
                return ret;
            }

            default:
                throw new NotImplementedException($"Unary operator {op}");
        }
    }

    public override bool HasKnownIntValue() =>  expr.HasKnownIntValue();
    public override int GetKnownIntValue() {
        switch(op) {
            case TokenKind.Minus: return -expr.GetKnownIntValue();
            case TokenKind.Not: return expr.GetKnownIntValue() == 0 ? 1 : 0;
            default: throw new NotImplementedException($"Unary operator {op}");
        }
    }

}
