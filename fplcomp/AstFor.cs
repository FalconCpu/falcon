class AstFor(Location location, AstIdentifier astId, AstExpression astStart, AstExpression astEnd, TokenKind op, AstBlock? parent)
: AstBlock(location, parent) {
    public AstIdentifier astId = astId;
    public AstExpression astStart = astStart;
    public AstExpression astEnd = astEnd;
    public TokenKind     op = op;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"FOR EXPR {op}");
        astId.Print(indent + 1);
        astStart.Print(indent + 1);
        astEnd.Print(indent + 1);
    }

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext) {
        astStart.TypeCheckRvalue(scope, pathContext);
        astEnd.TypeCheckRvalue(scope, pathContext);

        // Save a copy of the initial path context in case the loop iterates zero times
        PathContext pathContextInitial = pathContext.Clone();

        IntType.Instance.CheckAssignableFrom(astStart);
        astStart.type.CheckAssignableFrom(astEnd);
        Symbol sym = new VariableSymbol(astId.name, astStart.type, false);
        AddSymbol(location, sym);
        astId.SetSymbol(sym);

        foreach(AstStatement stmt in statements)
            pathContext = stmt.TypeCheck(this, pathContext);
        return PathContext.Merge([pathContextInitial, pathContext]);
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
        if (op==TokenKind.Gt || op==TokenKind.Gte)
            func.Add(new InstrAlui(iterSym, AluOp.SUB_I, iterSym, 1));
        else
            func.Add(new InstrAlui(iterSym, AluOp.ADD_I, iterSym, 1));
        func.Add(new InstrLabel(labCond));
        AluOp aluOp = op switch {
            TokenKind.Lt => AluOp.LT_I,
            TokenKind.Lte => AluOp.LE_I,
            TokenKind.Gt => AluOp.GT_I,
            TokenKind.Gte => AluOp.GE_I,
            _ => throw new Exception("Invalid operator")
        };
        func.Add(new InstrBra(aluOp, iterSym, end, labStart));
    }
}


