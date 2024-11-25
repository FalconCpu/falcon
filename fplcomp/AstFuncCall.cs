class AstFuncCall(Location location, AstExpression left, List<AstExpression> args) : AstExpression(location) {
    public AstExpression left = left;
    public List<AstExpression> args = args;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"FUNC CALL ({type})");
        left.Print(indent + 1);
        foreach (AstExpression arg in args)
            arg.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        left.TypeCheckRvalue(scope);
        foreach (AstExpression arg in args)
            arg.TypeCheckRvalue(scope);

        if (left.type==ErrorType.Instance) {
            SetError();
            return;
        }

        if (left.type is FunctionType func) {
            List<Type> paramTypes = func.parameterTypes;

            if (args.Count != paramTypes.Count) {
                SetError(location, $"Expected {paramTypes.Count} arguments, got {args.Count}");
                return;
            }

            for (int i = 0; i < args.Count; i++)
                if (!paramTypes[i].IsAssignableFrom(args[i].type))
                    Log.Error(location, $"Expected argument {i+1} to be type {paramTypes[i]}, got {args[i].type}");
            SetType(func.returnType);

        } else {
            SetError(location, $"Expected function, got {left.type}");
            return;
        }
    }

    private void CodeGenMetodCall(AstFunction func, FunctionSymbol funcSym, Symbol? thisSym, List<Symbol> argSyms) {
        // Do some sanity check that what we are calling is indeed a method of this
        if (thisSym == null) throw new ArgumentException("No 'this' symbol");
        if (thisSym.type is not ClassType) throw new ArgumentException("thisSym must be a class type");
        if (thisSym.type is ClassType classType && !classType.methods.Contains(funcSym)) 
            throw new ArgumentException($"{funcSym} is not a method of this");

        // Generate code for the call
        func.Add(new InstrMov(RegisterSymbol.registers[1], thisSym));
        for(int i=0; i<argSyms.Count; i++)
            func.Add(new InstrMov(RegisterSymbol.registers[i+2], argSyms[i]));
        func.Add(new InstrCall(funcSym.function, argSyms.Count+1, type));
    }

    private void CodeGenFunctionCall(AstFunction func, FunctionSymbol funcSym, List<Symbol> argSyms) {
        for(int i=0; i<argSyms.Count; i++)
            func.Add(new InstrMov(RegisterSymbol.registers[i+1], argSyms[i]));
        func.Add(new InstrCall(funcSym.function, argSyms.Count, type));
    }

    private void CodeGenIndirectFunctionCall(AstFunction func, Symbol funcPtrSym, List<Symbol> argSyms) {
        for(int i=0; i<argSyms.Count; i++)
            func.Add(new InstrMov(RegisterSymbol.registers[i+1], argSyms[i]));
        func.Add(new InstrCallr(funcPtrSym, argSyms.Count, type));
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        List<Symbol> argSyms = args.Select(it => it.CodeGenRvalue(func)).ToList();

        // Check to see if we have a method call
        if (left is AstMember mem) {
            if (mem.field is FunctionSymbol funcSym) {
                // We have the form  expr.methodname(args...)
                Symbol thisArg = mem.left.CodeGenRvalue(func);
                CodeGenMetodCall(func, funcSym, thisArg, argSyms);
            } else {
                // We have a call to a function pointer call
                Symbol funcPtrSym = mem.CodeGenRvalue(func);
                CodeGenIndirectFunctionCall(func, funcPtrSym, argSyms);
            }
        } else {
            Symbol leftSym = left.CodeGenRvalue(func);
            if (leftSym is FunctionSymbol fs) {
                if (fs.function.methodOf != null) {
                    // We have the form  m(args...)   where m is a method
                    CodeGenMetodCall(func, fs, func.thisSymbol, argSyms);
                } else {
                    // We have a simple function call  funcname(args...)
                    CodeGenFunctionCall(func, fs, argSyms);
                }
            }
        }

        // Collect the return value
        Symbol ret = func.NewTemp(type);
        if (! (type==UnitType.Instance))
            func.Add(new InstrMov(ret, RegisterSymbol.registers[8]));
        else
            func.Add(new InstrMov(ret, RegisterSymbol.registers[0]));

        return ret;
    }

}
