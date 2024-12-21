
class AstClass : AstFunction {
    public GenericClassType classType;
    public ClassType instanceClassType;

    public AstClass(Location location, string name, List<AstIdentifier> astTypeParameters, List<AstParameter> astParams, AstBlock parent)
    : base(location, name, astParams, null, [], parent) {

        // Build the type parameters
        List<TypeParameterType> typeParameters = [];
        foreach(AstIdentifier id in astTypeParameters) {
            TypeParameterType typeParameter = new TypeParameterType($"{name}.{id.name}");
            TypeSymbol ts = new(id.name, typeParameter);
            AddSymbol(id.location, ts);  // This implicitly checks for duplicates
            typeParameters.Add(typeParameter);
        }

        classType = new GenericClassType(name, this, typeParameters);
        instanceClassType = ClassType.MakeClassType(classType, typeParameters.Select(it => it as Type).ToList());
        TypeSymbol symbol = new TypeSymbol(name, classType);
        parent.AddSymbol(location, symbol);

        thisSymbol = new VariableSymbol("this", instanceClassType, false, false);
        AddSymbol(location, thisSymbol);
    }

    public override void IdentifyFunctions(AstBlock scope) {
        allFunctions.Add(this);

        // Generate the parameter symbols
        foreach(AstParameter param in astParameters) {
            Symbol sym = param.GenerateFieldSymbol(this);
            parameters.Add(sym);
            AddSymbol(param.location, sym);
            if (sym is FieldSymbol field)
                classType.AddField(field);
        }
        
        foreach(AstStatement stmt in statements)
            if (stmt is AstFunction func)
                func.IdentifyFunctions(this);
    }

    public override PathContext TypeCheck(AstBlock scope, PathContext pathContext) {
        // Type check the body
        foreach(AstStatement stmt in statements) {
            pathContext = stmt.TypeCheck(this, pathContext);
        }
        return pathContext;
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