class AstClass : AstFunction {
    public ClassType classType;

    public AstClass(Location location, string name, List<AstParameter> astParams, AstBlock parent)
    : base(location, name, astParams, null, parent) {
        classType = new ClassType(name, this);
        TypeSymbol symbol = new TypeSymbol(name, classType);
        parent.AddSymbol(location, symbol);
        thisSymbol = new VariableSymbol("this", classType, false, false);
        AddSymbol(location, thisSymbol);
    }

    public override void TypeCheck(AstBlock scope) {
        allFunctions.Add(this);

        // Generate the parameter symbols
        foreach(AstParameter param in astParameters) {
            Symbol sym = param.GenerateFieldSymbol(scope);
            parameters.Add(sym);
            AddSymbol(param.location, sym);
            if (sym is FieldSymbol field)
                classType.AddField(field);
        }

        // Type check the body
        foreach(AstStatement stmt in statements) {
            stmt.TypeCheck(this);
        }
    }


    public override void CodeGen(AstFunction func) {
        // Copy the parameters into the local variables
        Add(new InstrStart()); 
        labels.Add(endLabel);
        Add (new InstrMov(thisSymbol!, RegisterSymbol.registers[1]) );

        for(int i=0; i<parameters.Count; i++)
            if (parameters[i] is FieldSymbol field)
                Add (new InstrStoreField(field.type.GetSize(), RegisterSymbol.registers[i+2], thisSymbol!, field));
            else
                Add (new InstrMov(parameters[i], RegisterSymbol.registers[i+2]) );

        // Generate the code for the body of the function
        foreach(AstStatement stmt in statements)
            if (stmt is not AstFunction)
                stmt.CodeGen(this); 

        Add(new InstrLabel(endLabel) );
        Add(new InstrEnd(returnType) );
    }

}