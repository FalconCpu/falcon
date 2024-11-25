class AstWhile(Location location, AstExpression condition, AstBlock parent) : AstBlock(location,parent) {
    public AstExpression condition = condition;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "WHILE");
        condition.Print(indent + 1);
        foreach (AstStatement stmt in statements)
            stmt.Print(indent + 1);
    }

    public override void TypeCheck(AstBlock scope) {
        condition.TypeCheckRvalue(scope);
        BoolType.Instance.CheckAssignableFrom(condition);

        foreach (AstStatement stmt in statements)
            stmt.TypeCheck(this);
    }

    public override void CodeGen(AstFunction func)    {
        Label start = func.NewLabel();
        Label end = func.NewLabel();
        Label cond = func.NewLabel();
        func.Add(new InstrJmp(cond) );
        func.Add(new InstrLabel(start));
        foreach (AstStatement stmt in statements)
            stmt.CodeGen(func);
        func.Add(new InstrLabel(cond));
        condition.CodeGenBool(func, start, end);
        func.Add(new InstrLabel(end));
    }
}