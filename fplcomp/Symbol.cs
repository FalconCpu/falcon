abstract class Symbol(string name, Type type) {
    public readonly string name = name;
    public readonly Type type = type;

    // For use by backend
    public int index;
    public int reg;
    public int useCount;

    public static Symbol undefined = new VariableSymbol("<undefined>", UndefinedType.Instance, false, false);

    public override string ToString() {
        return name;
    }

    private static int nextId = 0;
    public static string UniqueName() => $"&{nextId++}";  

    public bool IsInteger() => (this is TempSymbol ts) && ts.hasKnownValue;
    public bool IsIntegerZero() => (this is TempSymbol ts) && ts.hasKnownValue && ts.value==0;
    public bool IsSmallInteger() => (this is TempSymbol ts) && ts.hasKnownValue && (ts.value>=-0x1000 && ts.value<0x1000);
    public int  GetIntegerValue() => (this is TempSymbol ts) ? ts.value : throw new Exception("not an integer");
}

class FunctionSymbol(string name, Type type, AstFunction function) : Symbol(name, type) {
    public readonly AstFunction function = function;
}

class FieldSymbol(string name, Type type, bool isMutable) : Symbol(name, type) {
    public readonly bool isMutable = isMutable;
    public int offset = 0;
    public static new FieldSymbol undefined = new FieldSymbol("<undefined>", UndefinedType.Instance, false);
}

class TypeSymbol(string name, Type type) : Symbol( name, type) {
}

class ConstantSymbol(string name, Type type, int value) : Symbol(name, type) {
    public readonly int value = value;
}

class ConstObjectAliasSymbol(string name, Symbol alias) : Symbol(name, alias.type) {
    public readonly Symbol alias = alias;
}

// Used to represent a constant object (i.e. a string literal)
class ConstObjectSymbol(string name, Type type, List<int> value, string? displayContents=null) : Symbol(name, type) {
    public List<int> value = value;
    public string? displayContents = displayContents;
    
    public static readonly List<ConstObjectSymbol> allConstObjects = [];
    
    public static ConstObjectSymbol Make(Type type, List<int> value, string? displayContents=null) {
        foreach (var cos in allConstObjects)
            if (cos.type==type && cos.value.SequenceEqual(value))
                return cos;
        string name = (type is StringType) ? $"&string{allConstObjects.Count}" : $"&object{allConstObjects.Count}";
        ConstObjectSymbol ret = new(name, type, value, displayContents);
        allConstObjects.Add(ret);
        return ret;
    }
}

// Used to represent a member access for smartcast tracking. 
// Should not be used for anything else.

class SmartcastMemberAccessSymbol(Symbol lhs, Symbol rhs) : Symbol("<smartcast>", rhs.type) {
    public readonly Symbol lhs = lhs;
    public readonly Symbol rhs = rhs;
}



class TempSymbol(string name, Type type) : Symbol(name, type) {
    public readonly bool hasKnownValue = false;
    public readonly int  value;

    public TempSymbol(string name, Type type, int value) : this(name, type) {
        this.value = value;
        hasKnownValue = true;
    }
}   

class RegisterSymbol(string name) : Symbol(name, UndefinedType.Instance)
{
    public static readonly List<RegisterSymbol> registers = [
        new RegisterSymbol("0"),
        new RegisterSymbol("$1"),
        new RegisterSymbol("$2"),
        new RegisterSymbol("$3"),
        new RegisterSymbol("$4"),
        new RegisterSymbol("$5"),
        new RegisterSymbol("$6"),
        new RegisterSymbol("$7"),
        new RegisterSymbol("$8"),
        new RegisterSymbol("$9"),
        new RegisterSymbol("$10"),
        new RegisterSymbol("$11"),
        new RegisterSymbol("$12"),
        new RegisterSymbol("$13"),
        new RegisterSymbol("$14"),
        new RegisterSymbol("$15"),
        new RegisterSymbol("$16"),
        new RegisterSymbol("$17"),
        new RegisterSymbol("$18"),
        new RegisterSymbol("$19"),
        new RegisterSymbol("$20"),
        new RegisterSymbol("$21"),
        new RegisterSymbol("$22"),
        new RegisterSymbol("$23"),
        new RegisterSymbol("$24"),
        new RegisterSymbol("$25"),
        new RegisterSymbol("$26"),
        new RegisterSymbol("$27"),
        new RegisterSymbol("$28"),
        new RegisterSymbol("$29"),
        new RegisterSymbol("$30"),
        new RegisterSymbol("$sp"),
    ];

    public static RegisterSymbol zero = registers[0];
    public static RegisterSymbol sp = registers[31];
}



