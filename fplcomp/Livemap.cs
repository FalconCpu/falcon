// Build an map showing which symbols are live at each point in the code

using System.Collections;

class Livemap {
    private readonly AstFunction func;

    // livemap[line][sym] is true if when [line] is executed, the symbol may be read along some path
    public  readonly BitArray[] livemap;
    private readonly BitArray[] propmap;
    private bool madeChange = false;

    private void BuildMap() {
        for (int i = 0; i < func.code.Count; i++) {
            Symbol? dest = func.code[i].GetDest();
            if (dest != null)
                propmap[i][dest.index] = false;
            List<Symbol> sources = func.code[i].GetSources();
            foreach (Symbol s in sources)
                livemap[i][s.index] = true;
        }
    }

    private bool bitArrayEqual(BitArray a, BitArray b) {
        for (int i = 0; i < a.Length; i++)
            if (a[i] != b[i])
                return false;
        return true;
    }

    private void Propagate() {
        for (int index = func.code.Count-2; index>=0; index--) {
            Instr ins = func.code[index];

            if (ins is InstrJmp jumpIns) {
                livemap[index].Or(livemap[ jumpIns.label.index ]);
            
            } else if (ins is InstrBra braIns) {
                livemap[index].Or(livemap[ index+1 ]);
                livemap[index].Or(livemap[ braIns.label.index ]);

            } else if (ins is InstrLabel instrLabel) {
                // If any bits change at a label then we may need to run propagate again
                if (! bitArrayEqual(livemap[index], livemap[index+1]))
                    madeChange = true;
                livemap[index].Or(livemap[ index+1 ]);

            } else {
                BitArray x = (BitArray)propmap[index].Clone();
                x.And(livemap[index+1]);
                livemap[index].Or(x);
            }
        }
    }

    private void ChangeToNop(Instr instr) {
        Debug.PrintLn($"Replacing {instr.index} with NOP");
        func.code[instr.index] = new InstrNop();
        madeChange = true;
    }

    public Livemap(AstFunction func) {
        this.func = func;
        livemap = new BitArray[func.code.Count];
        propmap = new BitArray[func.code.Count];
        for (int i = 0; i < livemap.Length; i++) {
            livemap[i] = new BitArray(func.allsym.Count);
            propmap[i] = new BitArray(func.allsym.Count);
            propmap[i].Not();
        }
        BuildMap();

        do {
            madeChange = false;
            Debug.PrintLn($"Propagate {func.name}");
            Propagate();
        } while (madeChange);

        // Do a cleanup pass to remove dead code
        for(int index=0; index<func.code.Count; index++) {
            Instr ins = func.code[index];
            if (ins is InstrCall || ins is InstrCallr)  continue; // Don't touch calls as they may have side effects
            Symbol? dest = ins.GetDest();
            if (dest!=null && livemap[index+1][dest.index] == false && dest.index!=31)
                ChangeToNop(ins);
        }
    }

    public void Print() {
        Debug.PrintLn("");
        // Print a header
        for(int line=0; line<3; line++) {
            Debug.Print("                          ");
            for (int j = 0; j < func.allsym.Count; j++) {
                Debug.Print(func.allsym[j].name.PadLeft(3)[line]);
                if ((j&7)==7) Debug.Print(" ");
            }
            Debug.PrintLn("");
        }

        for (int i = 0; i < livemap.Length; i++) {
            Debug.Print($"{i,3}: {func.code[i],-20} ");
            for (int j = 0; j < livemap[i].Count; j++) {
                if (livemap[i][j] && propmap[i][j])
                    Debug.Print('X');
                else if (livemap[i][j])
                    Debug.Print('B');
                else if (!propmap[i][j])
                    Debug.Print('K');
                else
                    Debug.Print('.');
                if ((j&7)==7) Debug.Print(" ");
            }
            Debug.PrintLn("");
        }
    }
}