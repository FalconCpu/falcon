
class AstNew(Location location, AstType astType, List<AstExpression> astArgs) : AstExpression(location) {
    private readonly AstType astType = astType;
    private readonly List<AstExpression> astArgs = astArgs;

    public override void Print(int indent)    {
        Console.WriteLine(new String(' ', indent * 2) + $"NEW ({type})");
        astType.Print(indent + 1);
        foreach (var astArg in astArgs)
            astArg.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        type = astType.ResolveAsType(scope);
        foreach (var astArg in astArgs)
            astArg.TypeCheckRvalue(scope);
        if (type == ErrorType.Instance)
            return;

        if (type is ArrayType arrayType) {
            if (astArgs.Count != 1) {
                Log.Error(location, $"new Array<> requires exactly one argument");
                return;
            }
            IntType.Instance.CheckAssignableFrom(astArgs[0]);

        } else if (type is ClassType classType) {
            // Type check the arguments
            List<Symbol> parameters = classType.constructor.parameters;

            if (astArgs.Count != parameters.Count) {
                SetError(location, $"Expected {parameters.Count} arguments, got {astArgs.Count}");
                return;
            }

            for (int i = 0; i < astArgs.Count; i++)
                if (!parameters[i].type.IsAssignableFrom(astArgs[i].type))
                    Log.Error(location, $"Expected argument {i+1} to be type {parameters[i].type}, got {astArgs[i].type}");

        } else
            Log.Error(location, $"new {type} is not supported");

    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        if (type is ArrayType arrayType) {
            List<Symbol> args = astArgs.Select(it => it.CodeGenRvalue(func)).ToList();
            int elementSize = arrayType.elementType.GetSize();
            Symbol arraySize = func.NewTemp(IntType.Instance);
            func.Add( new InstrAlui(arraySize, AluOp.MUL_I, args[0], elementSize));
            func.Add( new InstrMov(RegisterSymbol.registers[1], arraySize));
            func.Add( new InstrCall(StdLib.malloc, 1, IntType.Instance));
            Symbol ret = func.NewTemp(type);
            func.Add( new InstrMov(ret, RegisterSymbol.registers[8]));
            return ret;

        } else if (type is ClassType classType) {
            // Allocate memory for the object
            func.Add( new InstrLdi(RegisterSymbol.registers[1], classType.size));
            func.Add( new InstrCall(StdLib.malloc, 1, IntType.Instance));
            Symbol ret = func.NewTemp(type);
            func.Add( new InstrMov(ret, RegisterSymbol.registers[8]));

            // Call the constructor
            List<Symbol> argSyms = astArgs.Select(it => it.CodeGenRvalue(func)).ToList();
            func.Add(new InstrMov(RegisterSymbol.registers[1], ret));
            for(int i=0; i<argSyms.Count; i++)
                func.Add(new InstrMov(RegisterSymbol.registers[i+2], argSyms[i]));
            func.Add(new InstrCall(classType.constructor, argSyms.Count, type));

            return ret;
        
        } else
            throw new NotImplementedException();
    }


}