class OutputAssembly(StreamWriter filehandle) {
    private readonly StreamWriter filehandle = filehandle;

    public void Write(string s) {
        filehandle.Write(s);
    }

    public void WriteLine(string s) {
        filehandle.WriteLine(s);
    }

    public void Output(Instr ins) {
        switch(ins) {
            case InstrLabel label:
                WriteLine($"{label.label.name}:");
                break;

            case InstrMov mov:
                WriteLine($"ld {mov.dest.name}, {mov.src.name}");
                break;

            case InstrLdi ldi:
                WriteLine($"ld {ldi.dest.name}, {ldi.value}");
                break;

            case InstrLea lea:
                if (lea.value is StringLitSymbol sls)
                    WriteLine($"ld {lea.dest.name}, {sls.stringLabel}");
                else
                    WriteLine($"ld {lea.dest.name}, {lea.value}");
                break;

            case InstrAlu alu:
                switch(alu.op) {
                    case AluOp.ADD_I: WriteLine($"add {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    case AluOp.SUB_I: WriteLine($"sub {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    case AluOp.MUL_I: WriteLine($"mul {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    case AluOp.DIV_I: WriteLine($"divs {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    case AluOp.MOD_I: WriteLine($"mods {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    case AluOp.AND_I: WriteLine($"and {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    case AluOp.OR_I:  WriteLine($"or {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    case AluOp.XOR_I: WriteLine($"xor {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    case AluOp.LSL_I: WriteLine($"lsl {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    case AluOp.LSR_I: WriteLine($"lsr {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    case AluOp.EQ_I:  WriteLine($"xor {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); 
                                      WriteLine($"cltu {alu.dest.name}, {alu.dest.name}, 1");
                                      break;
                    case AluOp.NE_I:  WriteLine($"xor {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}");    
                                      WriteLine($"cltu {alu.dest.name}, {alu.dest.name}, 1");
                                      WriteLine($"xor {alu.dest.name}, {alu.dest.name}, 1");
                                      break;
                    case AluOp.LT_I:  WriteLine($"clt {alu.dest.name}, {alu.lhs.name}, {alu.rhs.name}"); break;
                    default: throw new NotImplementedException($"AluOp {alu.op}");
                }
                break;

            case InstrAlui alui:
                switch(alui.op) {
                    case AluOp.ADD_I: WriteLine($"add {alui.dest.name}, {alui.lhs.name}, {alui.value}"); break;
                    case AluOp.SUB_I: WriteLine($"sub {alui.dest.name}, {alui.lhs.name}, {alui.value}"); break;
                    case AluOp.MUL_I: WriteLine($"mul {alui.dest.name}, {alui.lhs.name}, {alui.value}"); break;
                    case AluOp.DIV_I: WriteLine($"divs {alui.dest.name}, {alui.lhs.name}, {alui.value}"); break;
                    case AluOp.MOD_I: WriteLine($"mods {alui.dest.name}, {alui.lhs.name}, {alui.value}"); break;
                    case AluOp.AND_I: WriteLine($"and {alui.dest.name}, {alui.lhs.name}, {alui.value}"); break;
                    case AluOp.OR_I:  WriteLine($"or  {alui.dest.name}, {alui.lhs.name}, {alui.value}"); break;
                    case AluOp.XOR_I: WriteLine($"xor {alui.dest.name}, {alui.lhs.name}, {alui.value}"); break;
                    case AluOp.LSL_I: WriteLine($"lsl {alui.dest.name}, {alui.lhs.name}, {alui.value}"); break;
                    case AluOp.LSR_I: WriteLine($"lsr {alui.dest.name}, {alui.lhs.name}, {alui.value}"); break;
                    default: throw new NotImplementedException($"AluOp {alui.op}");
                }
                break;

            case InstrBra bra:
                switch(bra.op) {
                    case AluOp.EQ_I: WriteLine($"beq {bra.lhs.name}, {bra.rhs.name}, {bra.label.name}"); break;
                    case AluOp.NE_I: WriteLine($"bne {bra.lhs.name}, {bra.rhs.name}, {bra.label.name}"); break;
                    case AluOp.LT_I: WriteLine($"blt {bra.lhs.name}, {bra.rhs.name}, {bra.label.name}"); break;
                    case AluOp.LE_I: WriteLine($"bge {bra.rhs.name}, {bra.lhs.name}, {bra.label.name}"); break;
                    case AluOp.GT_I: WriteLine($"blt {bra.rhs.name}, {bra.lhs.name}, {bra.label.name}"); break;
                    case AluOp.GE_I: WriteLine($"bge {bra.lhs.name}, {bra.rhs.name}, {bra.label.name}"); break;
                    default: throw new NotImplementedException($"AluOp {bra.op}");
                }
                break;

            case InstrJmp jmp:
                WriteLine($"jmp {jmp.label.name}");
                break;

            case InstrCall call:
                WriteLine($"jsr {call.function.qualifiedName}");
                break;

            case InstrCallr callr:
                WriteLine($"jsr {callr.function.name}");
                break;

            case InstrEnd:
            case InstrStart:
                break;  
    
            case InstrNop:
                WriteLine($"nop");
                break;

            case InstrLoadMem ldm: 
                switch(ldm.size) {
                    case 1: WriteLine($"ldb {ldm.dest.name}, {ldm.src.name}[{ldm.offset}]"); break;
                    case 2: WriteLine($"ldh {ldm.dest.name}, {ldm.src.name}[{ldm.offset}]"); break;
                    case 4: WriteLine($"ldw {ldm.dest.name}, {ldm.src.name}[{ldm.offset}]"); break;
                    default: throw new ArgumentException($"LoadMem size {ldm.size}");
                }
                break;

            case InstrStoreMem stm: 
                switch(stm.size) {
                    case 1: WriteLine($"stb {stm.value.name}, {stm.src.name}[{stm.offset}]"); break;
                    case 2: WriteLine($"sth {stm.value.name}, {stm.src.name}[{stm.offset}]"); break;
                    case 4: WriteLine($"stw {stm.value.name}, {stm.src.name}[{stm.offset}]"); break;
                    default: throw new ArgumentException($"StoreMem size {stm.size}");
                }
                break;

            case InstrLoadField ldm: 
                switch(ldm.size) {
                    case 1: WriteLine($"ldb {ldm.dest.name}, {ldm.src.name}[{ldm.field.offset}]"); break;
                    case 2: WriteLine($"ldh {ldm.dest.name}, {ldm.src.name}[{ldm.field.offset}]"); break;
                    case 4: WriteLine($"ldw {ldm.dest.name}, {ldm.src.name}[{ldm.field.offset}]"); break;
                    default: throw new ArgumentException($"LoadMem size {ldm.size}");
                }
                break;

            case InstrStoreField stm: 
                switch(stm.size) {
                    case 1: WriteLine($"stb {stm.value.name}, {stm.src.name}[{stm.field.offset}]"); break;
                    case 2: WriteLine($"sth {stm.value.name}, {stm.src.name}[{stm.field.offset}]"); break;
                    case 4: WriteLine($"stw {stm.value.name}, {stm.src.name}[{stm.field.offset}]"); break;
                    default: throw new ArgumentException($"StoreMem size {stm.size}");
                }
                break;

            default:
                throw new NotImplementedException($"Instr {ins.GetType()}");
        }
    }

    private void GenPreamble(AstFunction func) {
        int stackSize = func.stackAlloc;
        if (func.maxRegister>8)
            stackSize += (func.maxRegister-8)*4;
        if (func.makesCalls)
            stackSize += 4;

        if (stackSize!=0)
            WriteLine($"sub $sp, {stackSize}");
        int offset = func.stackAlloc;
        for(int reg=9; reg<=func.maxRegister; reg++) {
            WriteLine($"stw {RegisterSymbol.registers[reg].name}, $sp[{offset}]");
            offset += 4;
        }
        if (func.makesCalls) {
            WriteLine($"stw $30, $sp[{offset}]");
            offset += 4;
        }
    }

    private void GenPostamble(AstFunction func) {
        int stackSize = func.stackAlloc;
        if (func.maxRegister>8)
            stackSize += (func.maxRegister-8)*4;
        if (func.makesCalls)
            stackSize += 4;

        int offset = func.stackAlloc;
        for(int reg=9; reg<=func.maxRegister; reg++) {
            WriteLine($"ldw {RegisterSymbol.registers[reg].name}, $sp[{offset}]");
            offset += 4;
        }
        if (func.makesCalls) {
            WriteLine($"ldw $30, $sp[{offset}]");
            offset += 4;
        }
        if (stackSize!=0)
            WriteLine($"add $sp, {stackSize}");
    }



    public void Output(AstFunction func) {
        WriteLine("");
        WriteLine($"{func.qualifiedName}:");

        GenPreamble(func);
        foreach(Instr ins in func.code)
            Output(ins);
        GenPostamble(func);

        WriteLine("ret");
    }

    public void Close() {
        filehandle.Close();
    }
}