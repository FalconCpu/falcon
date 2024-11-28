using System.Collections.Concurrent;

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
        func.Add(new InstrAlui(scaled_rhs, AluOp.MUL_I, rhs, size) );
        func.Add(new InstrAlu(addr, AluOp.ADD_I, lhs, scaled_rhs));
        func.Add(new InstrLoadMem(size,ret, addr,0));
        return ret;
    }

    public override void CodeGenLvalue(AstFunction func, Symbol value) {
        Symbol lhs = left.CodeGenRvalue(func);
        Symbol rhs = index.CodeGenRvalue(func);
        Symbol scaled_rhs = func.NewTemp(IntType.Instance);
        Symbol addr = func.NewTemp(IntType.Instance);
        int size = type.GetSize();
        func.Add(new InstrAlui(scaled_rhs, AluOp.MUL_I, rhs, size) );
        func.Add(new InstrAlu(addr, AluOp.ADD_I, lhs, scaled_rhs));
        func.Add(new InstrStoreMem(size, value, addr, 0));
    }
}


class IndexSymbol : Symbol {
    public readonly Tuple<Symbol,Symbol> value;

    private IndexSymbol(Type type, Tuple<Symbol,Symbol> value) : base(UniqueName(), type) {
        this.value = value;
    }

    private static readonly ConcurrentDictionary<Tuple<Symbol,Symbol>, IndexSymbol> cache = [];

    public static IndexSymbol Make(Type type, Symbol left, Symbol index) {
        var value = Tuple.Create(left, index);
        return cache.GetOrAdd(value, _ => new IndexSymbol(type, value));
    }
}