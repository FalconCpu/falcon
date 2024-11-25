using System.Collections;

class InterferenceGraph {
    private readonly AstFunction func;
    private readonly BitArray[] livemap;
    public readonly HashSet<Symbol>[] graph;

    private void AddEdge(Symbol a, Symbol b) {
        if (a.index != b.index) {
            graph[a.index].Add(b);
            graph[b.index].Add(a);
        }
    }

    private void BuildGraph() {
        // Iterate through the code, wherever an instruction writes to a symbol
        // add an edges to every signal that is live at that point
        for (int i = 0; i < func.code.Count-1; i++) {
            Instr ins = func.code[i];

            if (ins is InstrCall || ins is InstrCallr) {
                // called functions may write to any caller-save registers
                // So add edges to all caller-save registers 
                // Assuming registers 1-8 are caller-save
                for(int j=0; j<func.allsym.Count; j++)
                    if (livemap[i+1][j])
                        for(int k=1; k<=8; k++)
                            AddEdge(func.allsym[k], func.allsym[j]);

            } else if (ins is InstrMov movIns) {
                // Special case for moves: exclude the source from interference
                Symbol? dest = ins.GetDest();
                if (dest==null) continue;
                for(int j=0; j<func.allsym.Count; j++)
                    if (livemap[i+1][j] && j!=movIns.src.index)
                        AddEdge(dest, func.allsym[j]);

            } else {
                // General case: add edges between dest and all live symbols
                Symbol? dest = ins.GetDest();
                if (dest!=null)
                    for(int j=0; j<func.allsym.Count; j++)
                        if (livemap[i+1][j])
                            AddEdge(dest, func.allsym[j]);
            }
        }
    }


    public InterferenceGraph(AstFunction func, Livemap livemap) {
        this.func = func;
        this.livemap = livemap.livemap;
        graph = new HashSet<Symbol>[func.allsym.Count];
        for (int i = 0; i < func.allsym.Count; i++)
            graph[i] = [];
        BuildGraph();
    }

    public void Print() {
        Debug.PrintLn("Interference Graph");
        for (int i = 0; i < func.allsym.Count; i++) {
            if (graph[i].Count == 0) continue;
            Debug.Print($"{func.allsym[i].name} -> ");
            foreach (Symbol s in graph[i])
                Debug.Print($"{s.name} ");
            Debug.PrintLn("");
        }
    }
}