using System.Collections.Concurrent;

class AstRealLit(Location location, double value) : AstExpression(location) {
    public double value = value;

    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "REAL " + value + " (" + type + ")");
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        SetType( RealType.Instance);
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        throw new NotImplementedException();
    }

}

class RealLitSymbol : Symbol {
    public readonly double value;

    private RealLitSymbol(Type type, double value) : base(UniqueName(), type) {
        this.value = value;
    }

    private static readonly ConcurrentDictionary<double, RealLitSymbol> cache = [];

    public static RealLitSymbol Make(double value) {
        return cache.GetOrAdd(value, _ => new RealLitSymbol(RealType.Instance, value));
    }
}