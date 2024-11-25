class RegisterAllocator{
    private readonly AstFunction func;
    private readonly HashSet<Symbol>[] graph;    // graph[i] is the set of symbols that interfere with symbol i
    private readonly List<InstrMov> movInstructions;

    private void AssignReg(Symbol sym, int reg) {
        if (sym.reg != -1) 
            throw new Exception($"Symbol {sym} already assigned to register {sym.reg}");
        if (reg>=32 || reg<0)
            throw new Exception($"Register {reg} out of range");
        sym.reg = reg;
        graph[reg].UnionWith(graph[sym.index]);
        if (reg>func.maxRegister)
            func.maxRegister = reg;
        Debug.PrintLn($"Assigning {sym} to register {reg}");
    }

    private void MoveCoalese() {
        // Look to for MOV instructions where one of the operands is already in a register
        // and the other is not, then see if the other can be assigned to the same register
        foreach(InstrMov ins in movInstructions) {
            if (ins.dest.reg==-1 && ins.src.reg!=-1 && ! graph[ins.src.reg].Contains(ins.dest))
                AssignReg(ins.dest, ins.src.reg);
            if (ins.dest.reg!=-1 && ins.src.reg==-1 && ! graph[ins.dest.reg].Contains(ins.src))
                AssignReg(ins.src, ins.dest.reg);
        }
    }

    private void FindRegFor(Symbol sym) {
        // Find a register that doesn't interfere with any of the symbols that sym interferes with
        for(int reg=1; reg<29; reg++) {
            if (! graph[reg].Contains(sym)) {
                AssignReg(sym, reg);
                MoveCoalese();
                return;
            }
        }
        throw new Exception($"No free registers for {sym}");
    }

    private int FindUnassinedSymbol() {
        // Find the unassigned symbol with the highest number of interfering symbols
        // Returns -1 if there are no unassigned symbols
        int max = -1;
        int maxIndex = -1;
        for(int k=32; k<func.allsym.Count; k++)
            if (func.allsym[k].reg==-1 && graph[k].Count>max) {
                max = graph[k].Count;
                maxIndex = k;
            }
        return maxIndex;
    }

    public RegisterAllocator(AstFunction func, InterferenceGraph graph) {
        this.func = func;
        this.graph = graph.graph;
        movInstructions = func.code.OfType<InstrMov>().ToList();

        Debug.PrintLn("Register allocation");
        // Mark all the symbols as unassinged
        for(int k=0; k<func.allsym.Count; k++) {
            if (k<32)
                func.allsym[k].reg = k;
            else
                func.allsym[k].reg = -1;
        }

        // Go through the symbols and assign them to registers
        while(true) {
            MoveCoalese();
            int k = FindUnassinedSymbol();
            if (k==-1)
                break;
            FindRegFor(func.allsym[k]);
        }

        // Replace all symbols with their register
        for(int k=0; k<func.code.Count; k++) {
            func.code[k] = func.code[k].ReplaceSymbolWithReg();
        }
    }
}