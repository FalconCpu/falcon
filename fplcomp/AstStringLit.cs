using System.Text;
using  System.Collections.Concurrent;

class AstStringLit(Location location, string value) : AstExpression(location) {
    public string value = value;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "STRING " + value + " (" + type + ")");
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        SetType( StringType.Instance ); 
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        Symbol ret = StringLitSymbol.Make(value);
        func.Add(new InstrLea(ret, ret));
        return ret;
    }
}

class StringLitSymbol : Symbol {
    public readonly string value;
    public readonly string stringLabel;

    public string GetEscapedString() {
        StringBuilder ret = new();
        foreach(char c in value)
            ret.Append(c switch {
                '\n' => "\\n",
                '\t' => "\\t",
                '\r' => "\\r",
                '\0' => "\\0",
                _ => c,
            });
        return ret.ToString();
    }

    private StringLitSymbol(Type type, string value) : base(UniqueName(), type) {
        this.value = value;
        stringLabel = $"_string_{allStringLit.Count}";
    }

    public static readonly ConcurrentDictionary<string, StringLitSymbol> allStringLit = [];

    public static StringLitSymbol Make(string value) {
        return allStringLit.GetOrAdd(value, _ => new StringLitSymbol(StringType.Instance, value));
    }
}