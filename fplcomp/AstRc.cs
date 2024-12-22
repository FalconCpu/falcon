
class AstRc(Location location, AstExpression astExpr) : AstExpression(location) {
    public AstExpression astExpr = astExpr;

    public override void Print(int indent)    {
        Console.WriteLine(new string(' ', indent * 2) + "RC");
        astExpr.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope, PathContext pathContext)    {
        Log.Error(location, "Ref counting no longer supported");
        astExpr.TypeCheckRvalue(scope,pathContext);
        SetType(astExpr.type);
        
        if (astExpr.type is ClassType || astExpr.type is ArrayType || astExpr.type is NullableType || 
            astExpr.type is PointerType || astExpr.type is StringType) 
            return;
        Log.Error(location, $"invalid type to rc '{astExpr.type}'");
    }

    private void GenCodeIncRc(AstFunction func, Symbol addr, Type type) {
        Symbol addr1;
        if (type is ArrayType || type is StringType) {
            // allow for the length field
            addr1 = func.NewTemp(PointerType.Instance);
            func.Add( new InstrAlui(addr1, AluOp.SUB_I, addr, 4) );
        } else
            addr1 = addr;
            
        Symbol tmp1 = func.NewTemp(IntType.Instance);
        Symbol tmp2 = func.NewTemp(IntType.Instance);
        func.Add( new InstrLoadField(4, tmp1, addr1, StdLib.rcField) );
        func.Add( new InstrAlui(tmp2, AluOp.ADD_I, tmp1, 1) );
        func.Add( new InstrStoreField(4, tmp2, addr1, StdLib.rcField) );
    }


    public override Symbol CodeGenRvalue(AstFunction func) {
        Symbol ret = astExpr.CodeGenRvalue(func);
        Symbol tmp1 = func.NewTemp(IntType.Instance);
        Symbol tmp2 = func.NewTemp(IntType.Instance);

        if (type is PointerType) {
            // Need to guard the increment with a null check
            Label l1 = func.NewLabel();
            func.Add( new InstrBra(AluOp.EQ_I, ret, RegisterSymbol.zero, l1) );
            GenCodeIncRc(func, ret, type);
            func.Add( new InstrLabel(l1) );
        } else if (type is NullableType nt) {
            // Need to guard the increment with a null check, and account for the element type
            Label l1 = func.NewLabel();
            func.Add( new InstrBra(AluOp.EQ_I, ret, RegisterSymbol.zero, l1) );
            GenCodeIncRc(func, ret, nt.elementType);
            func.Add( new InstrLabel(l1) );
        } else
            // No need to guard the increment with a null check
            GenCodeIncRc(func, ret, type);

        return ret;
    }
}