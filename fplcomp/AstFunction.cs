
class AstFunction(Location location, string name, List<AstParameter> astParameters, AstType? astReturnType, List<TokenKind> qualifiers, AstBlock? parent) 
: AstBlock(location,parent) {
    public readonly string name = name;
    public readonly List<AstParameter> astParameters = astParameters;
    public readonly AstType? astReturnType = astReturnType;
    public readonly List<TokenKind> qualifiers = qualifiers;
    public readonly GenericClassType? methodOf = (parent is AstClass classAst) ? classAst.classType : null;
    public readonly ClassType? instanceOf = (parent is AstClass classAst) ? classAst.instanceClassType : null;
    public readonly string qualifiedName = (parent==null || parent is AstTop) ? $"/{name}" : $"{((AstFunction)parent).qualifiedName}/{name}";
    public static readonly List<AstFunction> allFunctions = [];
    public int virtualMethodNumber = -1; // populated when the function is added into a class
    public bool isExternal = false;

    // Filled in by the type checker
    public          List<Symbol> parameters = [];
    public          Type         returnType = Type.undefined;
    public          Type         functionType = Type.undefined;
    private         int          nextTemp = 0;
    public          Symbol?      thisSymbol = null;

    // Filled in by the code generator
    public          List<Instr>  code = [];
    public          List<Label>  labels = [];
    public          Label        endLabel = new("@ret");
    public          List<Symbol> allsym = [];
    public          int          maxRegister = 0;
    public          int          stackAlloc = 0;
    public          bool         makesCalls = false;



    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "FUNC " + name);
        foreach(AstParameter param in astParameters) {
            param.Print(indent + 1);
        }
        foreach(AstStatement stmt in statements) {
            stmt.Print(indent + 1);
        }
    }

    public override void IdentifyFunctions(AstBlock scope) {
        // Add this function to the list of all functions to have code generation, unless its external
        if (instanceOf!=null && instanceOf.isExternal)
            isExternal = true;
        else
            allFunctions.Add(this);

        // Generate the parameter symbols.
        // For a variadic types - we use the variadic type for the function type, but convert to an array 
        // for the symbol inside the function
        List<Type> paramTypes = [];
        for(int i = 0; i < astParameters.Count; i++) {
            Symbol sym = astParameters[i].GenerateSymbol(scope);
            if (sym.type is ArrayType ary && astParameters[i].isVariadic)
                paramTypes.Add( VariadicType.MakeVariadicType(ary.elementType));
            else
                paramTypes.Add(sym.type);

            parameters.Add(sym);
            AddSymbol(astParameters[i].location, sym);
        }

        if (instanceOf!=null) {
            thisSymbol = new VariableSymbol("this", instanceOf, false);
            AddSymbol(location, thisSymbol);
        }

        // Resolve the return type
        returnType = astReturnType?.ResolveAsType(scope) ?? UnitType.Instance;

        // Check the function has a body, unless it is in an extern class
        bool isMemberOfExternClass = (parent is AstClass classAst && classAst.qualifiers.Contains(TokenKind.Extern));
        if (statements.Count==0 && !isMemberOfExternClass)
            Log.Error(location, $"Function {name} does not have a body");
        if (statements.Count!=0 && isMemberOfExternClass)
            Log.Error(location, $"Function {name} is defined in an extern class");

        // Create the function symbol
        functionType = FunctionType.MakeFunctionType(paramTypes, returnType);
        FunctionSymbol funcSym = new(name, functionType, this);
        scope.AddSymbol(location, funcSym);
        methodOf?.AddMethod(funcSym);
    }

    public override PathContext TypeCheck(AstBlock scope, PathContext _) {
        // Make sure 'virtual' is only used on methods
        if (qualifiers.Contains(TokenKind.Virtual) && methodOf==null)
            Log.Error(location, "Virtual can only be used on methods");
    
        // Type check the body
        PathContext pathContext = new ();
        foreach(AstStatement stmt in statements) {
            pathContext = stmt.TypeCheck(this, pathContext);
        }
        if (returnType!=UnitType.Instance && !pathContext.unreachable) {
            Log.Error(location, $"Function does not return '{returnType}' along all paths");
        }
        return pathContext;
    }

    public TempSymbol NewTemp(Type type) {
        TempSymbol sym = new($"&{nextTemp++}", type);
        return sym;
    }

    public TempSymbol NewTemp(Type type, int value) {
        TempSymbol sym = new($"&{nextTemp++}", type, value);
        return sym;
    }

    public override void CodeGen(AstFunction func) {
        Add(new InstrStart()); 
        labels.Add(endLabel);

        // Copy the parameters into the local variables
        int offset = 1;
        if (thisSymbol!=null) {
            Add(new InstrMov(thisSymbol, RegisterSymbol.registers[1]) );
            offset = 2;
        }
        for(int i=0; i<parameters.Count; i++)
            Add (new InstrMov(parameters[i], RegisterSymbol.registers[i+offset]) );

        // Generate the code for the body of the function
        foreach(AstStatement stmt in statements)
            if (stmt is not AstFunction)
                stmt.CodeGen(this);

        Add(new InstrLabel(endLabel) );
        if (this is AstTop astTop) {
            AstFunction funcMain = allFunctions.Find(it => it.name == "main") ?? throw new Exception("No main function");
            Add(new InstrMov(RegisterSymbol.registers[1], astTop.arg1) );
            Add(new InstrMov(RegisterSymbol.registers[2], astTop.arg2) );
            Add( new InstrCall(funcMain, 2, IntType.Instance) );
        }
        Add(new InstrEnd(returnType) );
    }


    // **************************************************************
    //                           Backend
    // **************************************************************

    public void Add(Instr instr) {
        code.Add(instr);
        if (instr is InstrCall call || instr is InstrCallr callr) {
            makesCalls = true;
        }
    }

    public Label NewLabel() {
        Label label = new($"@{labels.Count}");
        labels.Add(label);
        return label;
    }

    public virtual void PrintCode() {
        Debug.PrintLn($"\n{name}:");
        foreach(Instr instr in code)
            Debug.PrintLn($"{instr.index,3} {instr.ToString()}");
    }

    private void AddAllSym(Symbol? s) {
        if (s==null) return;    
        if (allsym.Contains(s)) return;
        s.index = allsym.Count;
        s.useCount = 0;
        allsym.Add(s);
    }

    public void RebuildIndex() {
        // Get the set of registers + all the symbols in the code
        code.RemoveAll(instr => instr is InstrNop);
        allsym = RegisterSymbol.registers.ToList<Symbol>();
        allsym.ForEach(it => it.useCount = 0);
        foreach(Label label in labels) {
            label.index = -1;
            label.useCount = 0;
        }

        foreach(Instr instr in code){
            AddAllSym(instr.GetDest());
            foreach(Symbol sym in instr.GetSources()) {
                AddAllSym(sym);
                sym.useCount ++;                
            }
            if (instr is InstrBra bra) bra.label.useCount++;
            if (instr is InstrJmp jmp) jmp.label.useCount++;
        }
        
        for(int i=0; i<allsym.Count; i++)
            allsym[i].index = i;
        for(int i=0; i<code.Count; i++) {
            code[i].index = i;
            if (code[i] is InstrLabel labelIns)
                labelIns.label.index = i;
        }
    }

    public virtual void RunBackend() {
        _ = new Peephole(this);
        Livemap livemap = new(this);
        livemap.Print();
        InterferenceGraph graph = new(this, livemap);
        graph.Print();
        _ = new RegisterAllocator(this, graph);
        _ = new Peephole(this);
    }
}
