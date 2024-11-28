
class AstCharLit(Location location, int value) : AstExpression(location) {
    public int value = value;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"CHARLIT {value} ({type})");
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        SetType(CharType.Instance);
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        Symbol ret = func.NewTemp(type,value);
        func.Add(new InstrLdi(ret, value));
        return ret;
    }

    public override bool HasKnownIntValue() => true;
    public override int GetKnownIntValue() => value;
}
