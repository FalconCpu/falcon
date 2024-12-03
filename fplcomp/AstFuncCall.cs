class AstFuncCall(Location location, AstExpression left, List<AstExpression> args) : AstExpression(location) {
    public AstExpression left = left;
    public List<AstExpression> args = args;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + $"FUNC CALL ({type})");
        left.Print(indent + 1);
        foreach (AstExpression arg in args)
            arg.Print(indent + 1);
    }

    public override void TypeCheckRvalue(AstBlock scope, PathContext pathContext) {
        // Typecheck the function reference itself (e.g., resolve what "left" is)
        left.TypeCheckRvalue(scope, pathContext);
        if (left.type==ErrorType.Instance) {
            SetError();
            return;
        }

        if (left.type is not FunctionType func) {
            SetError(location, $"Expected function, got {left.type}");
            return;
        }

        // Typecheck the arguments
        foreach (AstExpression arg in args)
            arg.TypeCheckRvalue(scope, pathContext);

        // Extract the par  ameter types from the function type
        List<Type> paramTypes = func.parameterTypes;


        if (paramTypes.Count>0 && paramTypes.Last() is VariadicType vt) {
            // Ensure there are enough arguments for the non-variadic parameters
            if (args.Count < paramTypes.Count-1) {
                SetError(location, $"Expected {paramTypes.Count-1} arguments, got {args.Count}");
                return;
            }

            // Validate each argument's type
            // Match regular parameters first, then variadic ones
            for (int i = 0; i < args.Count; i++) {
                Type paramType = i<paramTypes.Count-1 ? paramTypes[i] : vt.elementType;
                if (! paramType.IsAssignableFrom(args[i].type))
                    Log.Error(location, $"Expected argument {i+1} to be type {paramType}, got {args[i].type}");
            }

        } else {
            // For non-variadic functions, ensure the argument count matches exactly
            if (args.Count != paramTypes.Count) {
                SetError(location, $"Expected {paramTypes.Count} arguments, got {args.Count}");
                return;
            }

            for (int i = 0; i < args.Count; i++) {
                if (! paramTypes[i].IsAssignableFrom(args[i].type))
                    Log.Error(location, $"Expected argument {i+1} to be type {paramTypes[i]}, got {args[i].type}");
            }
        }

        // Set the return type
        SetType(func.returnType);
    }

    private void CodeGenArgsNoVariadic(AstFunction func, Symbol? thisSym) {
        // Copy the arguments into the local variables
        List<Symbol> argSyms = args.Select(it => it.CodeGenRvalue(func)).ToList();  
        int destReg = 1;
        if (thisSym!=null)
            func.Add(new InstrMov(RegisterSymbol.registers[destReg++], thisSym));

        for(int i=0; i<argSyms.Count; i++)
            func.Add(new InstrMov(RegisterSymbol.registers[destReg++], argSyms[i]));
    }

    private void CodeGenArgsWithVariadic(AstFunction func, Symbol? thisSym, int numberNonVariadicParams) {
        // Generate the Variadic arguments first
        int numVariadicArgs = args.Count - numberNonVariadicParams;

        // Create space on the stack for the variadic arguments, plus a count value
        func.Add( new InstrAlui(RegisterSymbol.sp, AluOp.SUB_I, RegisterSymbol.sp, 4 + numVariadicArgs*4) );
        Symbol lengthSym = func.NewTemp(IntType.Instance);
        func.Add( new InstrLdi(lengthSym, numVariadicArgs));
        func.Add( new InstrStoreMem(4, lengthSym, RegisterSymbol.sp, 0));

        // Store the variadic arguments into the newly created space
        for(int i=0; i<numVariadicArgs; i++) {
            Symbol argSym = args[numberNonVariadicParams+i].CodeGenRvalue(func);
            func.Add( new InstrStoreMem(4, argSym, RegisterSymbol.sp, 4 + i*4));
        }

        // Now evaluate the non-variadic arguments
        List<Symbol> argSyms = args[0..numberNonVariadicParams].Select(it => it.CodeGenRvalue(func)).ToList();  
        int destReg = 1;
        if (thisSym!=null)
            func.Add(new InstrMov(RegisterSymbol.registers[destReg++], thisSym));
        for(int i=0; i<argSyms.Count; i++)
            func.Add(new InstrMov(RegisterSymbol.registers[destReg++], argSyms[i]));
        
        // And add the variadic arguments as a pointer to the variadic arguments (allowing for the legth field)
        func.Add(new InstrAlui(RegisterSymbol.registers[destReg++], AluOp.ADD_I, RegisterSymbol.sp, 4));
    }

    private void CodeGenCall(AstFunction output, FunctionSymbol funcSym, Symbol? thisSym) {
        FunctionType funcType = (FunctionType)funcSym.type;
        List<Type> paramTypes = funcType.parameterTypes;
        int numParameters = paramTypes.Count + (thisSym!=null ? 1 : 0);   // Allow for a 'this' parameter if needed
        if (paramTypes.Count>0 && paramTypes.Last() is VariadicType vt) {
            // For variadic functions, we need to generate the arguments in a special way
            CodeGenArgsWithVariadic(output, thisSym, paramTypes.Count-1);
            output.Add(new InstrCall(funcSym.function, numParameters, funcType.returnType));
            // And then clean up the stack
            output.Add(new InstrAlui(RegisterSymbol.sp, AluOp.ADD_I, RegisterSymbol.sp, 4*(args.Count-paramTypes.Count+2)));
        } else {
            CodeGenArgsNoVariadic(output, thisSym);
            output.Add(new InstrCall(funcSym.function, numParameters, funcType.returnType));
        }
    }


    private void CodeGenIndirectCall(AstFunction func, Symbol funcSym) {
        throw new NotImplementedException("Indirect calls not yet implemented");
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        // Check to see if we have a method call
        if (left is AstMember mem) {
            if (mem.field is FunctionSymbol funcSym) {
                // We have the form  expr.methodname(args...)
                Symbol thisArg = mem.left.CodeGenRvalue(func);
                CodeGenCall(func, funcSym, thisArg);
            } else {
                // We have a call to a function pointer call
                Symbol funcPtrSym = mem.CodeGenRvalue(func);
                CodeGenIndirectCall(func, funcPtrSym);
            }
        } else {
            Symbol leftSym = left.CodeGenRvalue(func);
            if (leftSym is FunctionSymbol fs) {
                if (fs.function.methodOf != null) {
                    // We have the form  m(args...)   where m is a method
                    CodeGenCall(func, fs, func.thisSymbol);
                } else {
                    // We have a simple function call  funcname(args...)
                    CodeGenCall(func, fs, null);
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
