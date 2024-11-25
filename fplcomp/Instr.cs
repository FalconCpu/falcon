
abstract class Instr() {
    public int index;  // Used to track the location of the instruction in the code
    public abstract Symbol? GetDest();
    public abstract List<Symbol> GetSources();
    public abstract Instr ReplaceSymbolWithReg();
}

class InstrNop : Instr {
    public override string ToString() => "NOP";
    public override Symbol? GetDest() => null;
    public override List<Symbol> GetSources() => [];
    public override Instr ReplaceSymbolWithReg() => this;
}

class InstrMov(Symbol dest, Symbol src) : Instr {
    public readonly Symbol dest = dest;
    public readonly Symbol src = src;
    public override string ToString() => $"MOV {dest}, {src}";
    public override Symbol? GetDest() => dest;
    public override List<Symbol> GetSources() => [src];
    public override Instr ReplaceSymbolWithReg() => new InstrMov(RegisterSymbol.registers[dest.reg], RegisterSymbol.registers[src.reg]);
}

class InstrLdi(Symbol dest, int value) : Instr {
    public readonly Symbol dest = dest;
    public readonly int value = value;
    public override string ToString() => $"LDI {dest}, {value}";
    public override Symbol? GetDest() => dest;
    public override List<Symbol> GetSources() => [];
    public override Instr ReplaceSymbolWithReg() => new InstrLdi(RegisterSymbol.registers[dest.reg], value);
}

class InstrLea(Symbol dest, Symbol value) : Instr {
    public readonly Symbol dest = dest;
    public readonly Symbol value = value;
    public override string ToString() => $"LEA {dest}, {value}";
    public override Symbol? GetDest() => dest;
    public override List<Symbol> GetSources() => [];
    public override Instr ReplaceSymbolWithReg() => new InstrLea(RegisterSymbol.registers[dest.reg], value);
}

class InstrAlu(Symbol dest, AluOp op, Symbol lhs, Symbol rhs) : Instr {
    public readonly Symbol dest = dest;
    public readonly AluOp op = op;
    public readonly Symbol lhs = lhs;
    public readonly Symbol rhs = rhs;
    public override string ToString() => $"{op} {dest}, {lhs}, {rhs}";
    public override Symbol? GetDest() => dest;
    public override List<Symbol> GetSources() => [lhs, rhs];
    public override Instr ReplaceSymbolWithReg() => new InstrAlu(RegisterSymbol.registers[dest.reg], op, RegisterSymbol.registers[lhs.reg], RegisterSymbol.registers[rhs.reg]);
}

class InstrAlui(Symbol dest, AluOp op, Symbol lhs, int value) : Instr {
    public readonly Symbol dest = dest;
    public readonly AluOp op = op;
    public readonly Symbol lhs = lhs;
    public readonly int value = value;
    public override string ToString() => $"{op} {dest}, {lhs}, {value}";
    public override Symbol? GetDest() => dest;
    public override List<Symbol> GetSources() => [lhs];
    public override Instr ReplaceSymbolWithReg() => new InstrAlui(RegisterSymbol.registers[dest.reg], op, RegisterSymbol.registers[lhs.reg], value);
}

class InstrBra(AluOp op, Symbol lhs, Symbol rhs, Label label) : Instr{
    public readonly AluOp op = op;
    public readonly Symbol lhs = lhs;
    public readonly Symbol rhs = rhs;
    public readonly Label label = label;
    public override string ToString() => $"B{op} {lhs}, {rhs}, {label}";
    public override Symbol? GetDest() => null;
    public override List<Symbol> GetSources() => [lhs, rhs];
    public override Instr ReplaceSymbolWithReg() => new InstrBra(op, RegisterSymbol.registers[lhs.reg], RegisterSymbol.registers[rhs.reg], label);
}

class InstrJmp(Label label) : Instr{
    public readonly Label label = label;
    public override string ToString() => $"JMP {label}";
    public override Symbol? GetDest() => null;
    public override List<Symbol> GetSources() => [];
    public override Instr ReplaceSymbolWithReg() => this;
}

class InstrLabel(Label label) : Instr{
    public readonly Label label = label;
    public override string ToString() => $"{label}:";
    public override Symbol? GetDest() => null;
    public override List<Symbol> GetSources() => [];
    public override Instr ReplaceSymbolWithReg() => this;
}

class InstrLoadMem(int size, Symbol dest, Symbol address, int offset) : Instr {
    public readonly int size = size;
    public readonly Symbol dest = dest;
    public readonly Symbol src = address;
    public readonly int offset = offset;
    public override string ToString() => $"LD{size} {dest}, {src}[{offset}]";
    public override Symbol? GetDest() => dest;
    public override List<Symbol> GetSources() => [src];
    public override Instr ReplaceSymbolWithReg() => new InstrLoadMem(size, RegisterSymbol.registers[dest.reg], RegisterSymbol.registers[src.reg], offset);
}

class InstrStoreMem(int size, Symbol value, Symbol address, int offset) : Instr {
    public readonly int size = size;
    public readonly Symbol value = value;
    public readonly Symbol src = address;  // Size information encoded in address's type
    public readonly int offset = offset;
    public override string ToString() => $"ST{size} {value}, {src}[{offset}]";
    public override Symbol? GetDest() => null;
    public override List<Symbol> GetSources() => [value, src];
    public override Instr ReplaceSymbolWithReg() => new InstrStoreMem(size,RegisterSymbol.registers[value.reg], RegisterSymbol.registers[src.reg],offset);
}

class InstrLoadField(int size, Symbol dest, Symbol address, FieldSymbol field) : Instr {
    public readonly int size = size;
    public readonly Symbol dest = dest;
    public readonly Symbol src = address;
    public readonly FieldSymbol field = field;
    public override string ToString() => $"LD{size} {dest}, {src}[{field}]";
    public override Symbol? GetDest() => dest;
    public override List<Symbol> GetSources() => [src];
    public override Instr ReplaceSymbolWithReg() => new InstrLoadField(size, RegisterSymbol.registers[dest.reg], RegisterSymbol.registers[src.reg], field);
}

class InstrStoreField(int size, Symbol value, Symbol address, FieldSymbol field) : Instr {
    public readonly int size = size;
    public readonly Symbol value = value;
    public readonly Symbol src = address;  // Size information encoded in address's type
    public readonly FieldSymbol field = field;
    public override string ToString() => $"ST{size} {value}, {src}[{field}]";
    public override Symbol? GetDest() => null;
    public override List<Symbol> GetSources() => [value, src];
    public override Instr ReplaceSymbolWithReg() => new InstrStoreField(size,RegisterSymbol.registers[value.reg], RegisterSymbol.registers[src.reg],field);
}

class InstrStart() : Instr {
    public override string ToString() => "START";
    public override Symbol? GetDest() => null;
    public override List<Symbol> GetSources() => [];
    public override Instr ReplaceSymbolWithReg() => this;
}

class InstrEnd(Type type) : Instr {
    public readonly Type type = type;
    public override string ToString() => "END";
    public override Symbol? GetDest() => null;
    public override List<Symbol> GetSources() => type==UnitType.Instance ? [] : [RegisterSymbol.registers[8]];
    public override Instr ReplaceSymbolWithReg() => this;
}

class InstrCall(AstFunction function, int numArgs, Type retType) : Instr {
    public readonly AstFunction function = function;
    public readonly int numArgs = numArgs;
    public readonly Type retType = retType;
    public override string ToString() => $"CALL {function.name}";
    public override Symbol? GetDest() => retType==UnitType.Instance ? RegisterSymbol.registers[8] : null;
    public override List<Symbol> GetSources() => RegisterSymbol.registers.GetRange(1, numArgs).ToList<Symbol>();
    public override Instr ReplaceSymbolWithReg() => this;
}

class InstrCallr(Symbol function, int numArgs, Type retType) : Instr {
    public readonly Symbol function = function;
    public readonly int numArgs = numArgs;
    public readonly Type retType = retType;
    public override string ToString() => $"CALLR {function}";
    public override Symbol? GetDest() => retType==UnitType.Instance ? RegisterSymbol.registers[8] : null;
    public override List<Symbol> GetSources() => [.. RegisterSymbol.registers.GetRange(1, numArgs), function];
    public override Instr ReplaceSymbolWithReg() => new InstrCallr(RegisterSymbol.registers[function.reg], numArgs, retType);
}

class Label(string name) {
    public int index;
    public string name = name;
    public int useCount;
    public override string ToString() => name;
}