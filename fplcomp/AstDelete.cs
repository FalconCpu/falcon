class AstDelete(Location location, AstExpression astExpr) : AstStatement(location) {

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "DELETE");
        astExpr.Print(indent + 1);
    }

    public override void TypeCheck(AstBlock scope)
    {
        astExpr.TypeCheckRvalue(scope);
        if (astExpr.type is ClassType || astExpr.type is ArrayType || astExpr.type is NullableType || 
            astExpr.type is PointerType || astExpr.type is StringType)
            return;
        Log.Error(location, $"invalid type to delete '{astExpr.type}'");
    }

    public override void CodeGen(AstFunction func)
    {
        Symbol ret = astExpr.CodeGenRvalue(func);
        Symbol tmp1 = func.NewTemp(IntType.Instance);
        Symbol tmp2 = func.NewTemp(IntType.Instance);
        Label label = func.NewLabel();

        if (astExpr.type is PointerType || astExpr.type is NullableType) {
            // Need to guard the increment with a null check
            func.Add( new InstrBra(AluOp.EQ_I, ret, RegisterSymbol.zero, label) );
        }
        Type type = astExpr.type is NullableType nt? nt.elementType : astExpr.type;

        // Decrement the refcount    
        func.Add( new InstrLoadField(4, tmp1, ret, StdLib.rcField) );
        func.Add( new InstrAlui(tmp2, AluOp.SUB_I, tmp1, 1) );
        func.Add( new InstrStoreField(4, tmp2, ret, StdLib.rcField) );
        func.Add( new InstrBra(AluOp.NE_I, tmp2, RegisterSymbol.zero, label) );

        // If the objet has a custom destructor then call it
        if (type is ClassType ct) {
            Symbol? dtor = ct.GetField("delete");
            if (dtor != null && dtor is FunctionSymbol fs) {
                func.Add( new InstrMov(RegisterSymbol.registers[1], ret) );
                func.Add( new InstrCall(fs.function, 1, UnitType.Instance) );
            }
        }
        
        // Free the object
        func.Add( new InstrMov(RegisterSymbol.registers[1], ret) );
        func.Add( new InstrCall(StdLib.free, 1, UnitType.Instance) );
        func.Add( new InstrLabel(label) );
    }

}