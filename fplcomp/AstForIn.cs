class AstForIn(Location location, AstIdentifier astId, AstExpression astList, AstBlock? parent)
: AstBlock(location, parent) {
    public AstIdentifier astId = astId;
    public AstExpression astList = astList;

    private AstFunction? getFunc = null;
    private FieldSymbol? lengthField = null;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"FOR_IN");
        astId.Print(indent + 1);
        astList.Print(indent + 1);
    }

    private Type VerifyIterator() {
        if (astList.type is not ClassType ct)
            return ErrorType.MakeErrorType(astList.location, $"{astList.type} is not a class");
        if (ct.GetField("length") is not FieldSymbol ls)
            return ErrorType.MakeErrorType(location, $"Class {astList.type} does not have length defined");
        if (ct.GetField("get") is not FunctionSymbol fs)
            return ErrorType.MakeErrorType(location, $"Class {astList.type} does not have get defined");   
        if (fs.type is not FunctionType ft)
            return ErrorType.MakeErrorType(location, $"Class {astList.type}.get is not a function");
        if (ft.parameterTypes.Count != 1)
            return ErrorType.MakeErrorType(location, $"Class {astList.type}.get does not have 1 parameters");
        if (ft.parameterTypes[0]!=IntType.Instance)
            return ErrorType.MakeErrorType(location, $"Class {astList.type}.get does not have an int parameter");

        getFunc = fs.function;
        lengthField = ls;
        return ft.returnType;
    }

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext) {
        // Save a copy of the initial path context in case the loop iterates zero times
        PathContext pathContextInitial = pathContext.Clone();

        astList.TypeCheckRvalue(scope, pathContext);
        Type type = VerifyIterator();
        Symbol sym = new VariableSymbol(astId.name, type, false, false);
        AddSymbol(location, sym);
        astId.SetSymbol(sym);

        foreach(AstStatement stmt in statements)
            pathContext = stmt.TypeCheck(this, pathContext);
        
        return PathContext.Merge([pathContext, pathContextInitial]);
    }

    public override void CodeGen(AstFunction func) {
        // Create Symbols for the list, item, index, and end
        Symbol itemSym = astId.symbol;
        Symbol listSym = astList.CodeGenRvalue(func);
        Symbol lenSym = func.NewTemp(IntType.Instance);
        Symbol index = func.NewTemp(IntType.Instance);
        func.Add(new InstrLoadField(4, lenSym, listSym, lengthField!));
        func.Add(new InstrMov(index, RegisterSymbol.zero));

        // Create labels for the start and end of the loop
        Label labStart = func.NewLabel();
        Label labCond = func.NewLabel();
        func.Add(new InstrJmp(labCond));
        func.Add(new InstrLabel(labStart));

        // Fetch the next item
        func.Add(new InstrMov(RegisterSymbol.registers[1], listSym));
        func.Add(new InstrMov(RegisterSymbol.registers[2], index));
        func.Add(new InstrCall(getFunc!, 2, itemSym.type));
        func.Add(new InstrMov(itemSym, RegisterSymbol.registers[8]));

        // Create the loop body
        foreach (AstStatement stmt in statements)
            stmt.CodeGen(func);

        // Increment the index and loop back
        func.Add(new InstrAlui(index, AluOp.ADD_I, index, 1));
        func.Add(new InstrLabel(labCond));
        func.Add(new InstrBra(AluOp.LT_I, index, lenSym, labStart));
    }
}


