
class AstIntLit(Location location, int value) : AstExpression(location) {
    public int value = value;
    private Symbol symbol = Symbol.undefined;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"INTLIT {value} ({type})");
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        symbol = IntegerSymbol.Make(type, value);
        SetType(IntType.Instance);
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        func.Add(new InstrLdi(symbol, value));
        return symbol;
    }

    public override Symbol? GetConstValue() {
        return symbol;
    }
}

