class AstFor(Location location, AstIdentifier astId, AstExpression astStart, AstExpression astEnd, AstBlock? parent)
: AstBlock(location, parent) {
    public AstIdentifier astId = astId;
    public AstExpression astStart = astStart;
    public AstExpression astEnd = astEnd;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "FOR EXPR");
        astId.Print(indent + 1);
        astStart.Print(indent + 1);
        astEnd.Print(indent + 1);
    }

    public override void TypeCheck(AstBlock scope) {
        astStart.TypeCheckRvalue(scope);
        astEnd.TypeCheckRvalue(scope);
        IntType.Instance.CheckAssignableFrom(astStart);
        astStart.type.CheckAssignableFrom(astEnd);
        Symbol sym = new VariableSymbol(astId.name, astStart.type, false, false);
        AddSymbol(location, sym);
        astId.SetSymbol(sym);

        foreach(AstStatement stmt in statements)
            stmt.TypeCheck(this);
    }

    public override void CodeGen(AstFunction func) {
        Symbol iterSym = astId.symbol;
        Symbol start = astStart.CodeGenRvalue(func);
        func.Add(new InstrMov(iterSym, start));
        Symbol end = astEnd.CodeGenRvalue(func);
        Label labStart = func.NewLabel();
        Label labCond = func.NewLabel();
        func.Add(new InstrJmp(labCond));
        func.Add(new InstrLabel(labStart));
        foreach (AstStatement stmt in statements)
            stmt.CodeGen(func);
        func.Add(new InstrAlui(iterSym, AluOp.ADD_I, iterSym, 1));
        func.Add(new InstrLabel(labCond));
        func.Add(new InstrBra(AluOp.LE_I, iterSym, end, labStart));
    }
}


