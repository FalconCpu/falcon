

class AstNew(Location location, AstType astType, List<AstExpression> astArgs, bool hasInitializerList) : AstExpression(location) {
    private readonly AstType astType = astType;
    private readonly List<AstExpression> astArgs = astArgs;
    private readonly bool hasInitializerList = hasInitializerList;
    private ConstObjectSymbol? constObject=null;

    public override void Print(int indent)    {
        Console.WriteLine(new String(' ', indent * 2) + $"NEW ({type}) {hasInitializerList}");
        astType?.Print(indent + 1);
        foreach (var astArg in astArgs)
            astArg.Print(indent + 1);
    }

    private void TypeCheckArrayWithInitializerList(ArrayType arrayType) {
        // inder the array type from the elements
        if (arrayType.elementType==Type.undefined && astArgs.Count>=1)
            type = ArrayType.MakeArrayType(astArgs[0].type);
    
        foreach(AstExpression astArg in astArgs)
            ((ArrayType)type).elementType.CheckAssignableFrom(astArg);

        // If all elements are constant, we can make a const object
        if (astArgs.All(it=> it.HasKnownIntValue())) {
            List<int> values = astArgs.Select(it=>it.GetKnownIntValue()).ToList();
            constObject = ConstObjectSymbol.Make(arrayType, values);
        }
    }

    private void TypeCheckArrayNoInitializerList(ArrayType arrayType) {
        if (arrayType.elementType==Type.undefined)
            Log.Error(location, $"Cannot determine array element type");
        if (astArgs.Count != 1) 
            Log.Error(location, $"new Array<> requires exactly one argument");
        else
            IntType.Instance.CheckAssignableFrom(astArgs[0]);
    }

    private void TypeCheckClass(ClassType classType) {
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
    }


    public override void TypeCheckRvalue(AstBlock scope) {
        type = astType.ResolveAsType(scope);
        foreach (var astArg in astArgs)
            astArg.TypeCheckRvalue(scope);
        if (type == ErrorType.Instance)
            return;

        if (type is ArrayType arrayType)
            if (hasInitializerList)
                TypeCheckArrayWithInitializerList(arrayType);
            else
                TypeCheckArrayNoInitializerList(arrayType);
        else if (type is ClassType classType)
            TypeCheckClass(classType);
        else
            Log.Error(location, $"new {type} is not supported");

    }

    private Symbol CodeGenArrayWithInitializerList(AstFunction func, ArrayType arrayType) {
        // For an array with an initializer list, we calculate the size based on the number of elements
        int elementSize = arrayType.elementType.GetSize();
        int size = elementSize * astArgs.Count;
        func.Add( new InstrLdi(RegisterSymbol.registers[1], size));
        func.Add( new InstrLea(RegisterSymbol.registers[2], AstStringLit.genSym(type.name)));
        func.Add( new InstrCall(StdLib.malloc, 2, IntType.Instance));
        Symbol ret = func.NewTemp(type);
        func.Add( new InstrMov(ret, RegisterSymbol.registers[8]));
        
        // Populate the array
        if (constObject!=null) {
            // If all elements in the array are constant, we can do a memcpy to initialize the array
            func.Add( new InstrMov(RegisterSymbol.registers[1], ret));
            func.Add( new InstrLea(RegisterSymbol.registers[2], constObject));
            func.Add( new InstrLdi(RegisterSymbol.registers[3], size));
            func.Add( new InstrCall(StdLib.memcpy, 3, IntType.Instance));
        } else {
            // Otherwise we need to evaluate the elements at runtime
            for (int i = 0; i < astArgs.Count; i++) {
                Symbol element = astArgs[i].CodeGenRvalue(func);
                func.Add( new InstrStoreMem(elementSize, element, ret, i*elementSize));
            }
        }
        return ret;
    }

    private Symbol CodeGenArrayNoInitializerList(AstFunction func, ArrayType arrayType) {
        List<Symbol> args = astArgs.Select(it => it.CodeGenRvalue(func)).ToList();
        int elementSize = arrayType.elementType.GetSize();
        Symbol arraySize = func.NewTemp(IntType.Instance);
        func.Add( new InstrAlui(arraySize, AluOp.MUL_I, args[0], elementSize));
        func.Add( new InstrMov(RegisterSymbol.registers[1], arraySize));
        func.Add( new InstrLea(RegisterSymbol.registers[2], AstStringLit.genSym(type.name)));
        func.Add( new InstrCall(StdLib.malloc, 2, IntType.Instance));
        Symbol ret = func.NewTemp(type);
        func.Add( new InstrMov(ret, RegisterSymbol.registers[8]));
        return ret;
    }

    private Symbol CodeGenClassInstance(AstFunction func, ClassType classType) {
        // Allocate memory for the object
        func.Add( new InstrLdi(RegisterSymbol.registers[1], classType.size));
        func.Add( new InstrLea(RegisterSymbol.registers[2], AstStringLit.genSym(type.name)));
        func.Add( new InstrCall(StdLib.malloc, 2, IntType.Instance));
        Symbol ret = func.NewTemp(type);
        func.Add( new InstrMov(ret, RegisterSymbol.registers[8]));

        // Call the constructor
        List<Symbol> argSyms = astArgs.Select(it => it.CodeGenRvalue(func)).ToList();
        func.Add(new InstrMov(RegisterSymbol.registers[1], ret));
        for(int i=0; i<argSyms.Count; i++)
            func.Add(new InstrMov(RegisterSymbol.registers[i+2], argSyms[i]));
        func.Add(new InstrCall(classType.constructor, argSyms.Count+1, type));

        return ret;

    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        if (type is ArrayType arrayType) {
            if (hasInitializerList)
                return CodeGenArrayWithInitializerList(func, arrayType);
            else
                return CodeGenArrayNoInitializerList(func, arrayType);
        } else if (type is ClassType classType) {
            return CodeGenClassInstance(func, classType);
        } else
            throw new NotImplementedException();
    }

    public override bool HasKnownConstObjectValue() => constObject != null;
    public override ConstObjectSymbol GetKnownConstObjectValue() => constObject ?? throw new ArgumentException("Attempt to get ConstObject value from non-ConstObject");

}