class AstIfExpr(Location location, AstExpression cond, AstExpression then, AstExpression els) : AstExpression(location) {
    public AstExpression cond = cond;
    public AstExpression then = then;
    public AstExpression els = els;


    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "IF EXPR ({type})");
        cond.Print(indent + 1);
        then.Print(indent + 1);
        els.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        cond.TypeCheckRvalue(scope);
        then.TypeCheckRvalue(scope);
        els.TypeCheckRvalue(scope);
        BoolType.Instance.CheckAssignableFrom(cond);
        then.type.CheckAssignableFrom(els);
        SetType(then.type);
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        Label labTrue = func.NewLabel();
        Label labFalse = func.NewLabel();
        Label labEnd = func.NewLabel();
        Symbol ret = func.NewTemp(type);
        cond.CodeGenBool(func, labTrue, labFalse);

        func.Add(new InstrLabel(labTrue));
        Symbol thenSym = then.CodeGenRvalue(func);
        func.Add(new InstrMov(ret, thenSym));
        func.Add(new InstrJmp(labEnd));

        func.Add(new InstrLabel(labFalse));
        Symbol elsSym = els.CodeGenRvalue(func);
        func.Add(new InstrMov(ret, elsSym));
        func.Add(new InstrJmp(labEnd));

        func.Add(new InstrLabel(labEnd));
        return ret;

    }
}