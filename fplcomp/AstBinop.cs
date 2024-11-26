using System.Collections.Concurrent;

class AstBinop(Location location, TokenKind kind, AstExpression left, AstExpression right) : AstExpression(location) {
    private readonly TokenKind     kind = kind;
    private readonly AstExpression left = left;
    private readonly AstExpression right = right;

    private AluOp aluOp;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"BINOP {kind} ({type})");
        left.Print(indent + 1);
        right.Print(indent + 1);
    }

    private class BinopEntry(TokenKind op, Type leftType, Type rightType, AluOp aluOp, Type resultType) {
        public readonly TokenKind op = op;
        public readonly Type leftType = leftType;
        public readonly Type rightType = rightType;
        public readonly AluOp aluOp = aluOp;
        public readonly Type resultType = resultType;
    };

    static readonly List<BinopEntry> binopTable = [
        new(TokenKind.Plus,      IntType.Instance,  IntType.Instance,  AluOp.ADD_I,  IntType.Instance),
        new(TokenKind.Minus,     IntType.Instance,  IntType.Instance,  AluOp.SUB_I,  IntType.Instance),
        new(TokenKind.Star,      IntType.Instance,  IntType.Instance,  AluOp.MUL_I,  IntType.Instance),
        new(TokenKind.Slash,     IntType.Instance,  IntType.Instance,  AluOp.DIV_I,  IntType.Instance),
        new(TokenKind.Percent,   IntType.Instance,  IntType.Instance,  AluOp.MOD_I,  IntType.Instance),
        new(TokenKind.Ampersand, IntType.Instance,  IntType.Instance,  AluOp.AND_I,  IntType.Instance),
        new(TokenKind.Bar,       IntType.Instance,  IntType.Instance,  AluOp.OR_I,   IntType.Instance),
        new(TokenKind.Carat,     IntType.Instance,  IntType.Instance,  AluOp.XOR_I,  IntType.Instance),
        new(TokenKind.Eq,        IntType.Instance,  IntType.Instance,  AluOp.EQ_I,   BoolType.Instance),
        new(TokenKind.Neq,       IntType.Instance,  IntType.Instance,  AluOp.NE_I,   BoolType.Instance),
        new(TokenKind.Lt,        IntType.Instance,  IntType.Instance,  AluOp.LT_I,   BoolType.Instance),
        new(TokenKind.Lte,       IntType.Instance,  IntType.Instance,  AluOp.LE_I,   BoolType.Instance),
        new(TokenKind.Gt,        IntType.Instance,  IntType.Instance,  AluOp.GT_I,   BoolType.Instance),
        new(TokenKind.Gte,       IntType.Instance,  IntType.Instance,  AluOp.GE_I,   BoolType.Instance),
        new(TokenKind.And,       BoolType.Instance, BoolType.Instance, AluOp.AND_B,  BoolType.Instance),
        new(TokenKind.Or,        BoolType.Instance, BoolType.Instance, AluOp.OR_B,   BoolType.Instance)
    ];

    public override void TypeCheckRvalue(AstBlock scope) {
        left.TypeCheckRvalue(scope);
        right.TypeCheckRvalue(scope);
        
        if (left.type.IsErrorType() || right.type.IsErrorType()) {
            SetError();
            return;
        }

        foreach (var entry in binopTable) {
            if (entry.op == kind && entry.leftType.IsAssignableFrom(left.type) && entry.rightType.IsAssignableFrom(right.type)) {
                aluOp = entry.aluOp;
                SetType(entry.resultType);
                return;
            }
        }

        SetError( location, $"Invalid operation {kind} on types {left.type} and {right.type}");
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        if (aluOp==AluOp.AND_B || aluOp==AluOp.OR_B) 
            throw new NotImplementedException("Short circuiting not yet implemented for rvalues");

        Symbol left = this.left.CodeGenRvalue(func);
        Symbol right = this.right.CodeGenRvalue(func);
        Symbol ret =  func.NewTemp(type);
        func.Add( new InstrAlu(ret, aluOp, left, right) );
        return ret;
    }

    public override void CodeGenBool(AstFunction func, Label labTrue, Label labFalse) {
        switch(aluOp) {
            case AluOp.EQ_I:
            case AluOp.NE_I:
            case AluOp.LT_I:
            case AluOp.LE_I:
            case AluOp.GT_I:
            case AluOp.GE_I:
                Symbol lhs = left.CodeGenRvalue(func);
                Symbol rhs = right.CodeGenRvalue(func);
                func.Add( new InstrBra(aluOp, lhs, rhs, labTrue) );
                func.Add( new InstrJmp(labFalse) );
                break;

            case AluOp.AND_B:
                Label labAnd = func.NewLabel();
                left.CodeGenBool(func, labAnd, labFalse);
                func.Add( new InstrLabel(labAnd) );
                right.CodeGenBool(func, labTrue, labFalse);
                break;  

            case AluOp.OR_B:
                Label labOr = func.NewLabel();
                left.CodeGenBool(func, labTrue, labOr);
                func.Add( new InstrLabel(labOr) );
                right.CodeGenBool(func, labTrue, labFalse);
                break;
         
            default:
                base.CodeGenBool(func, labTrue, labFalse);
                break;
        }
    }


}

