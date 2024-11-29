
class AstIndex(Location location, AstExpression left, AstExpression index) : AstExpression(location) {
    public AstExpression index = index;
    public AstExpression left = left;

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "INDEX");
        left.Print(indent + 1);
        index.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        left.TypeCheckRvalue(scope);
        index.TypeCheckRvalue(scope);

        if (left.type.IsErrorType() || index.type.IsErrorType()) {
            SetError();
            return;
        }

        IntType.Instance.CheckAssignableFrom(index);

        if (left.type is ArrayType la)
            SetType( la.elementType );
        else if (left.type is StringType)
            SetType(CharType.Instance);
        else
            SetError(location, $"Cannot index non-array type {left.type}");
    }

    public override void TypeCheckLvalue(AstBlock scope) {
        TypeCheckRvalue(scope);
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        Symbol lhs = left.CodeGenRvalue(func);
        Symbol rhs = index.CodeGenRvalue(func);
        Symbol scaled_rhs = func.NewTemp(IntType.Instance);  //TODO check size
        Symbol addr = func.NewTemp(IntType.Instance);
        Symbol ret =  func.NewTemp(type);
        int size = type.GetSize();
        int offset = (lhs.type is StringType)? 4  : 0;  // Account for the length at the start of a string  
        func.Add(new InstrAlui(scaled_rhs, AluOp.MUL_I, rhs, size) );
        func.Add(new InstrAlu(addr, AluOp.ADD_I, lhs, scaled_rhs));
        func.Add(new InstrLoadMem(size,ret, addr,offset));
        return ret;
    }

    public override void CodeGenLvalue(AstFunction func, AluOp op, Symbol value) {
        Symbol lhs = left.CodeGenRvalue(func);
        Symbol rhs = index.CodeGenRvalue(func);
        Symbol scaled_rhs = func.NewTemp(IntType.Instance);
        Symbol addr = func.NewTemp(IntType.Instance);
        int size = type.GetSize();
        int offset = (lhs.type is StringType)? 4  : 0;  // Account for the length at the start of a string
        func.Add(new InstrAlui(scaled_rhs, AluOp.MUL_I, rhs, size) );
        func.Add(new InstrAlu(addr, AluOp.ADD_I, lhs, scaled_rhs));
        if (op==AluOp.UNDEFINED)
            func.Add(new InstrStoreMem(size, value, addr, offset));
        else {
            Symbol temp1 = func.NewTemp(type);
            Symbol temp2 = func.NewTemp(type);
            func.Add(new InstrLoadMem(size, temp1, addr, offset));
            func.Add(new InstrAlu(temp2, op, temp1, value));
            func.Add(new InstrStoreMem(size, temp2, addr, offset));
        }
    }
}
