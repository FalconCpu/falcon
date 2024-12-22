
class AstStringLit(Location location, string value) : AstExpression(location) {
    public string value = value;
    private ConstObjectSymbol symbol = GenSym(value);


    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "STRING " + value + " (" + type + ")");
    }

    public override void TypeCheckRvalue(AstBlock scope, PathContext pathContext) {
        SetType( StringType.Instance ); 
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        Symbol ret = func.NewTemp(type);
        func.Add(new InstrLea(ret, symbol));
        return ret;
    }

    public static ConstObjectSymbol GenSym(string value) {
        List<int> data = [value.Length];
        foreach(char[] s in (value+"\0").Chunk(4)) {
            int v = 0;
            foreach(char c in s.Reverse()) 
                v = (v<<8) | c;
            data.Add(v);
        }

        return ConstObjectSymbol.Make(StringType.Instance, data, value);
    }
}

