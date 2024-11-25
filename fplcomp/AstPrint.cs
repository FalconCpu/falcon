class AstPrint(Location location, List<AstExpression> args) : AstStatement(location) {
    public List<AstExpression> args = args;

    public override void Print(int indent)    {
        Console.WriteLine(new string(' ', indent * 2) + "PRINT");
        foreach(AstExpression arg in args)
            arg.Print(indent + 1);
    }


    public override void TypeCheck(AstBlock scope)    {
        foreach(AstExpression arg in args)
            arg.TypeCheckRvalue(scope);

        if (args.Count == 0) {
            Log.Error(location, "No format string given to print");
            return;
        }

        // Check the first argument is a the format string
        StringType.Instance.IsAssignableFrom(args[0].type);

        if (args.Count > 4)
            Log.Error(location, "Currently print only supports a max of 3 arguments");
    }

    public override void CodeGen(AstFunction func)
    {
        // Generate code for the arguments
        List<Symbol> argSyms = args.Select(it => it.CodeGenRvalue(func)).ToList();

        // Copy the arguments into API registers
        for(int i=0; i<argSyms.Count; i++)
            func.Add(new InstrMov(RegisterSymbol.registers[i+1], argSyms[i]));
        
        func.Add(new InstrCall(StdLib.print, argSyms.Count, UnitType.Instance));
    }
}