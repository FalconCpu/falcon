
class AstNew(Location location, AstType astType, List<AstExpression> astArgs, bool hasInitializerList) : AstExpression(location) {
    private readonly AstType astType = astType;
    private readonly List<AstExpression> astArgs = astArgs;
    private readonly bool hasInitializerList = hasInitializerList;

    public override void Print(int indent)    {
        Console.WriteLine(new String(' ', indent * 2) + $"NEW ({type}) {hasInitializerList}");
        astType?.Print(indent + 1);
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
            if (hasInitializerList) {
                if (arrayType.elementType==Type.undefined && astArgs.Count>=1)
                    // inder the array type from the elements
                    type = ArrayType.MakeArrayType(astArgs[0].type);

                foreach(AstExpression astArg in astArgs) {
                    ((ArrayType)type).elementType.CheckAssignableFrom(astArg);
                }
            } else {
                if (arrayType.elementType==Type.undefined)
                    Log.Error(location, $"Cannot determine array element type");
                if (astArgs.Count != 1) 
                    Log.Error(location, $"new Array<> requires exactly one argument");
                else
                    IntType.Instance.CheckAssignableFrom(astArgs[0]);
            }

        } else if (type is ClassType classType) {
            if (hasInitializerList)
                SetError(location, "new class with initializer list not supported");

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
            if (hasInitializerList) {
                // For an array with an initializer list, we calculate the size based on the number of elements
                int elementSize = arrayType.elementType.GetSize();
                func.Add( new InstrLdi(RegisterSymbol.registers[1], elementSize * astArgs.Count));
                func.Add( new InstrCall(StdLib.malloc, 1, IntType.Instance));
                Symbol ret = func.NewTemp(type);
                func.Add( new InstrMov(ret, RegisterSymbol.registers[8]));
                
                // Populate the array
                for (int i = 0; i < astArgs.Count; i++) {
                    Symbol element = astArgs[i].CodeGenRvalue(func);
                    func.Add( new InstrStoreMem(elementSize, element, ret, i*elementSize));
                }
                return ret;

            } else {
                // For an array with a specified size, we need to evaluate the size expression to get the size
                List<Symbol> args = astArgs.Select(it => it.CodeGenRvalue(func)).ToList();
                int elementSize = arrayType.elementType.GetSize();
                Symbol arraySize = func.NewTemp(IntType.Instance);
                func.Add( new InstrAlui(arraySize, AluOp.MUL_I, args[0], elementSize));
                func.Add( new InstrMov(RegisterSymbol.registers[1], arraySize));
                func.Add( new InstrCall(StdLib.malloc, 1, IntType.Instance));
                Symbol ret = func.NewTemp(type);
                func.Add( new InstrMov(ret, RegisterSymbol.registers[8]));
                return ret;
            }

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
            func.Add(new InstrCall(classType.constructor, argSyms.Count+1, type));

            return ret;
        
        } else
            throw new NotImplementedException();
    }


}