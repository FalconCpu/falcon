
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

    public bool IsSmallInteger() => (this is IntegerSymbol sym) && (sym.value>=-0x1000 && sym.value<0x1000);
    public int  GetIntegerValue() => (this is IntegerSymbol sym) ? sym.value : throw new Exception("not an integer");
}

class FunctionSymbol(string name, Type type, AstFunction function) : Symbol(name, type) {
    public readonly AstFunction function = function;
}

class FieldSymbol(string name, Type type, bool mutable) : Symbol(name, type) {
    public readonly bool mutable = mutable;
    public int offset = 0;
    public static new FieldSymbol undefined = new FieldSymbol("<undefined>", UndefinedType.Instance, false);
}

class TypeSymbol(string name, Type type) : Symbol( name, type) {
}

class TempSymbol(string name, Type type) : Symbol(name, type) {
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
        new RegisterSymbol("$gp"),
        new RegisterSymbol("$lr"),
        new RegisterSymbol("$sp"),
    ];

    public static RegisterSymbol zero = registers[0];
}



