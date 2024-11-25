
using System.Diagnostics.Contracts;

class AstRepeat(Location location, AstBlock parent) : AstBlock(location, parent) {
    public AstExpression? cond = null;

    public void SetCond(AstExpression cond) {
        if (this.cond != null)
            throw new Exception("Repeat statement already has a condition");
        this.cond = cond;
    }
 
    public override void Print(int indent) {
        Console.WriteLine(new String(' ', indent * 2) + "REPEAT");
        cond?.Print(indent + 1);
        foreach(AstStatement stmt in statements) {
            stmt.Print(indent + 1);
        }
    }

    public override void TypeCheck(AstBlock scope) {
        cond!.TypeCheckRvalue(scope);
        BoolType.Instance.CheckAssignableFrom(cond);
        foreach(AstStatement stmt in statements) {
            stmt.TypeCheck(this);
        }
    }

    public override void CodeGen(AstFunction func)    {
        if (cond == null)     throw new ArgumentException("Repeat statement has no condition");
        Label start = func.NewLabel();
        Label end = func.NewLabel();
        func.Add(new InstrLabel(start));
        foreach (AstStatement stmt in statements)
            stmt.CodeGen(func);
        cond.CodeGenBool(func, end, start);
        func.Add(new InstrLabel(end));
    }



}