using System.Collections.Concurrent;

class AstIntLit(Location location, int value) : AstExpression(location) {
    public int value = value;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"INTLIT {value} ({type})");
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        SetType(IntType.Instance);
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        Symbol ret = IntegerSymbol.Make(type, value);
        func.Add( new InstrLdi(ret, value));
        return ret;
    }

}

class IntegerSymbol : Symbol {
    public readonly int value;

    private IntegerSymbol(Type type, int value) : base(UniqueName(), type) {
        this.value = value;
    }

    private IntegerSymbol(string name, Type type, int value) : base(name, type) {
        this.value = value;
    }


    private static readonly ConcurrentDictionary<Tuple<int, Type>, IntegerSymbol> cache = [];

    public static IntegerSymbol Make(Type type, int value) {
        var key = Tuple.Create(value, type);
        return cache.GetOrAdd(key, _ => new IntegerSymbol(type, value));
    }

    public static IntegerSymbol Make(int value) {
        return Make(IntType.Instance, value);
    }

    public static IntegerSymbol Make(string name, Type type, int value) {
        var key = Tuple.Create(value, type);
        return cache.GetOrAdd(key, _ => new IntegerSymbol(name, type, value));
    }

}