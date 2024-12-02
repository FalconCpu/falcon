class AstMember(Location location, AstExpression left, AstIdentifier identifier) : AstExpression(location) {
    public AstExpression left = left;
    public AstIdentifier identifier = identifier;
    public Symbol        field = FieldSymbol.undefined;


    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "MEMBER");
        left.Print(indent + 1);
        identifier.Print(indent + 1);
    }

    private FieldSymbol UndefinedField() {
        Log.Error(location, $"No field named {identifier.name} in class {left.type}");
        return new FieldSymbol(identifier.name, ErrorType.Instance, true);
    }

    public override void TypeCheckRvalue(AstBlock scope) {
        left.TypeCheckRvalue(scope);
        if (left.type.IsErrorType()) {
            SetError();
            return;
        }
        // For now - allow access through nullables - later we will need to check for null
        Type leftType = left.type is NullableType nt ? nt.elementType : left.type;


        if (leftType is ClassType classType) {
            field = classType.GetField(identifier.name) ?? UndefinedField();
            SetType(field.type);
        } else if ((leftType is StringType || leftType is ArrayType) && identifier.name=="length") {
            field = StdLib.lengthField;
            SetType(IntType.Instance);
        } else {
            SetError(location, $"Left side of member access must be a class, not {left.type}");
        }
    }

    public override void TypeCheckLvalue(AstBlock scope) {
        TypeCheckRvalue(scope);
        if (type.IsErrorType()) return;
        if (field is FieldSymbol ff) {
            if (!ff.isMutable)
                Log.Error(location, $"Field {field.name} is not mutable");
        } else {
            Log.Error(location, $"Field {field.name} is not mutable");
        }
    }

    public override Symbol CodeGenRvalue(AstFunction func) {
        if (field is FieldSymbol ff) {
            Symbol addr = left.CodeGenRvalue(func);
            Symbol ret = func.NewTemp(type);
            func.code.Add(new InstrLoadField(type.GetSize(), ret, addr, ff));
            return ret;
        } else 
            throw new NotImplementedException("AstMember.CodeGenRvalue for non-field");
    }

    public override void CodeGenLvalue(AstFunction func, AluOp op, Symbol value) {
        if (field is FieldSymbol ff) {
            Symbol addr = left.CodeGenRvalue(func);
            if (op == AluOp.UNDEFINED)
                func.code.Add(new InstrStoreField(type.GetSize(), value, addr, ff));
            else {
                Symbol temp1 = func.NewTemp(type);
                Symbol temp2 = func.NewTemp(type);
                func.code.Add(new InstrLoadField(type.GetSize(), temp1, addr, ff));
                func.code.Add(new InstrAlu(temp2, op, temp1, value));
                func.code.Add(new InstrStoreField(type.GetSize(), temp2, addr, ff));
            }
        } else 
            throw new NotImplementedException("AstMember.CodeGenRvalue for non-field");
    }
}