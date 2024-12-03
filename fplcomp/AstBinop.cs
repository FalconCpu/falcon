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
        new(TokenKind.Lsl,       IntType.Instance,  IntType.Instance,  AluOp.LSL_I,  IntType.Instance),
        new(TokenKind.Lsr,       IntType.Instance,  IntType.Instance,  AluOp.LSR_I,  IntType.Instance),
        new(TokenKind.Asr,       IntType.Instance,  IntType.Instance,  AluOp.ASR_I,  IntType.Instance),
        new(TokenKind.Eq,        IntType.Instance,  IntType.Instance,  AluOp.EQ_I,   BoolType.Instance),
        new(TokenKind.Neq,       IntType.Instance,  IntType.Instance,  AluOp.NE_I,   BoolType.Instance),
        new(TokenKind.Lt,        IntType.Instance,  IntType.Instance,  AluOp.LT_I,   BoolType.Instance),
        new(TokenKind.Lte,       IntType.Instance,  IntType.Instance,  AluOp.LE_I,   BoolType.Instance),
        new(TokenKind.Gt,        IntType.Instance,  IntType.Instance,  AluOp.GT_I,   BoolType.Instance),
        new(TokenKind.Gte,       IntType.Instance,  IntType.Instance,  AluOp.GE_I,   BoolType.Instance),
        new(TokenKind.And,       BoolType.Instance, BoolType.Instance, AluOp.AND_B,  BoolType.Instance),
        new(TokenKind.Or,        BoolType.Instance, BoolType.Instance, AluOp.OR_B,   BoolType.Instance),
        new(TokenKind.Eq,        BoolType.Instance, BoolType.Instance, AluOp.EQ_I,   BoolType.Instance),
        new(TokenKind.Neq,       BoolType.Instance, BoolType.Instance, AluOp.NE_I,   BoolType.Instance),
        new(TokenKind.Eq,        StringType.Instance,StringType.Instance,AluOp.EQ_S, BoolType.Instance),
        new(TokenKind.Neq,       StringType.Instance,StringType.Instance,AluOp.NE_S, BoolType.Instance),
        new(TokenKind.Lt,        StringType.Instance,StringType.Instance,AluOp.LT_S, BoolType.Instance),
        new(TokenKind.Lte,       StringType.Instance,StringType.Instance,AluOp.LE_S, BoolType.Instance),
        new(TokenKind.Gt,        StringType.Instance,StringType.Instance,AluOp.GT_S, BoolType.Instance),
        new(TokenKind.Gte,       StringType.Instance,StringType.Instance,AluOp.GE_S, BoolType.Instance),
        new(TokenKind.Plus,      PointerType.Instance, IntType.Instance, AluOp.ADD_I,   PointerType.Instance),
        new(TokenKind.Minus,     PointerType.Instance, IntType.Instance, AluOp.SUB_I,   PointerType.Instance),
        new(TokenKind.Minus,     PointerType.Instance, PointerType.Instance, AluOp.SUB_I,   IntType.Instance)

    ];

    public override void TypeCheckRvalue(AstBlock scope) {
        left.TypeCheckRvalue(scope);
        right.TypeCheckRvalue(scope);
        
        if (left.type.IsErrorType() || right.type.IsErrorType()) {
            SetError();
            return;
        }

        // Do automatic type promotion of Char to Int
        Type leftType = (left.type==CharType.Instance) ? IntType.Instance : left.type;
        Type rightType = (right.type==CharType.Instance) ? IntType.Instance : right.type;

        foreach (var entry in binopTable) {
            if (entry.op == kind && entry.leftType.IsAssignableFrom(leftType) && entry.rightType.IsAssignableFrom(rightType)) {
                aluOp = entry.aluOp;
                SetType(entry.resultType);
                return;
            }
        }

        // Some special cases for = and !=
        if (kind == TokenKind.Eq || kind==TokenKind.Neq) {
            aluOp = kind == TokenKind.Eq ? AluOp.EQ_I : AluOp.NE_I;
            SetType(BoolType.Instance);
            
            bool leftIsReferenceType = leftType is PointerType || leftType is NullableType || leftType is NullType ||
                                       leftType is ArrayType || leftType is StringType || leftType is ClassType;  
            bool rightIsReferenceType = rightType is PointerType || rightType is NullableType || rightType is NullType ||
                                        rightType is ArrayType || rightType is StringType || rightType is ClassType;

            // allow T? = null checks
            if (leftType  is NullableType && rightType is NullType)   return;
            if (rightType is NullableType && leftType  is NullType)   return;

            // allow comparisons between type Pointer and any reference type
            if (leftType is PointerType && rightIsReferenceType)      return;
            if (rightType is PointerType && leftIsReferenceType)      return;
        }

        SetError( location, $"Invalid operation {kind} on types {left.type} and {right.type}");
    }

    private Symbol GenCall(AstFunction func, AstFunction call, Symbol arg1, Symbol arg2) {
        Symbol ret = func.NewTemp(type);
        func.Add( new InstrMov(RegisterSymbol.registers[1], arg1) );
        func.Add( new InstrMov(RegisterSymbol.registers[2], arg2) );
        func.Add( new InstrCall(call,2, IntType.Instance) );
        func.Add( new InstrMov(ret, RegisterSymbol.registers[8]) );
        return ret;
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        Symbol left = this.left.CodeGenRvalue(func);
        Symbol right = this.right.CodeGenRvalue(func);
        Symbol ret =  func.NewTemp(type);
        Symbol tmp;

        switch(aluOp) {
            case AluOp.ADD_I:
            case AluOp.SUB_I:
            case AluOp.MUL_I:
            case AluOp.DIV_I:
            case AluOp.MOD_I:
            case AluOp.AND_I:
            case AluOp.OR_I:
            case AluOp.XOR_I:
            case AluOp.LSL_I:
            case AluOp.LSR_I:
            case AluOp.ASR_I:
            case AluOp.EQ_I:
            case AluOp.NE_I:
            case AluOp.LT_I:
            case AluOp.LE_I:
            case AluOp.GT_I:
            case AluOp.GE_I:
                func.Add( new InstrAlu(ret, aluOp, left, right) );
                return ret;

            case AluOp.EQ_S:
                ret = GenCall(func, StdLib.strequals, left, right);
                return ret;

            case AluOp.NE_S:
                tmp = GenCall(func, StdLib.strequals, left, right);
                func.Add( new InstrAlui(ret, AluOp.XOR_I, tmp, 1) );
                return ret;

            case AluOp.LT_S:
            case AluOp.LE_S:
            case AluOp.GT_S:
            case AluOp.GE_S:
                tmp = GenCall(func, StdLib.strcmp, left, right);
                AluOp opx = aluOp switch {
                    AluOp.LT_S => AluOp.LT_I,
                    AluOp.LE_S => AluOp.LE_I,
                    AluOp.GT_S => AluOp.GT_I,
                    AluOp.GE_S => AluOp.GE_I,
                    _ => throw new Exception($"Invalid AluOp {aluOp}")
                };
                func.Add( new InstrAlu(ret, opx, tmp, RegisterSymbol.zero) );
                return ret;

            default:
                throw new Exception($"Invalid aluOp {aluOp}");
        }
    }

    public override void CodeGenBool(AstFunction func, Label labTrue, Label labFalse) {
        switch(aluOp) {
            case AluOp.EQ_I:
            case AluOp.NE_I:
            case AluOp.LT_I:
            case AluOp.LE_I:
            case AluOp.GT_I:
            case AluOp.GE_I: {
                Symbol lhs = left.CodeGenRvalue(func);
                Symbol rhs = right.CodeGenRvalue(func);
                func.Add( new InstrBra(aluOp, lhs, rhs, labTrue) );
                func.Add( new InstrJmp(labFalse) );
                break;
            }

            case AluOp.EQ_S:
            case AluOp.NE_S: {
                Symbol lhs = left.CodeGenRvalue(func);
                Symbol rhs = right.CodeGenRvalue(func);
                Symbol ret = GenCall(func, StdLib.strequals, lhs, rhs);
                AluOp opx = aluOp == AluOp.EQ_S ? AluOp.NE_I : AluOp.EQ_I;
                func.Add( new InstrBra(opx, ret, RegisterSymbol.zero, labTrue) );
                func.Add( new InstrJmp(labFalse) );
                break;
            }

            case AluOp.GT_S:
            case AluOp.GE_S:
            case AluOp.LT_S:
            case AluOp.LE_S: {
                Symbol lhs = left.CodeGenRvalue(func);
                Symbol rhs = right.CodeGenRvalue(func);
                Symbol ret = GenCall(func, StdLib.strcmp, lhs, rhs);
                AluOp opx = aluOp switch {
                    AluOp.GT_S => AluOp.GT_I,
                    AluOp.GE_S => AluOp.GE_I,
                    AluOp.LT_S => AluOp.LT_I,
                    AluOp.LE_S => AluOp.LE_I,
                    _ => throw new Exception($"Invalid aluOp {aluOp}")
                };
                func.Add( new InstrBra(opx, ret, RegisterSymbol.zero, labTrue) );
                func.Add( new InstrJmp(labFalse) );
                break;

            }




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

