class AstRc(Location location, AstExpression astExpr) : AstExpression(location) {
    public AstExpression astExpr = astExpr;

    public override void Print(int indent)    {
        Console.WriteLine(new string(' ', indent * 2) + "RC");
        astExpr.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope)    {
        astExpr.TypeCheckRvalue(scope);
        SetType(astExpr.type);
        
        if (astExpr.type is ClassType || astExpr.type is ArrayType || astExpr.type is NullableType || 
            astExpr.type is PointerType || astExpr.type is StringType) 
            return;
        Log.Error(location, $"invalid type to rc '{astExpr.type}'");
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        Symbol ret = astExpr.CodeGenRvalue(func);
        Symbol tmp1 = func.NewTemp(IntType.Instance);
        Symbol tmp2 = func.NewTemp(IntType.Instance);

        if (type is PointerType || type is NullableType) {
            // Need to guard the increment with a null check
            Label l1 = func.NewLabel();
            func.Add( new InstrBra(AluOp.EQ_I, ret, RegisterSymbol.zero, l1) );
            func.Add( new InstrLoadField(4, tmp1, ret, StdLib.rcField) );
            func.Add( new InstrAlui(tmp2, AluOp.ADD_I, tmp1, 1) );
            func.Add( new InstrStoreField(4, tmp2, ret, StdLib.rcField) );
            func.Add( new InstrLabel(l1) );
        } else {
            func.Add( new InstrLoadField(4, tmp1, ret, StdLib.rcField) );
            func.Add( new InstrAlui(tmp2, AluOp.ADD_I, tmp1, 1) );
            func.Add( new InstrStoreField(4, tmp2, ret, StdLib.rcField) );
        }
        return ret;
    }
}