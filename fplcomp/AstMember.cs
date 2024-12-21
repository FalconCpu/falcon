class AstMember(Location location, AstExpression left, AstIdentifier identifier) : AstExpression(location) {
    public AstExpression left = left;
    public AstIdentifier identifier = identifier;
    public Symbol        field = FieldSymbol.undefined;

    // For smartcasts create a symbol to track the type of the member
    public SmartcastMemberAccessSymbol? smartcastMemberAccessSymbol = null;


    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "MEMBER");
        left.Print(indent + 1);
        identifier.Print(indent + 1);
    }

    private FieldSymbol UndefinedField(ClassType classType) {
        Log.Error(location, $"No field named {identifier.name} in class {classType}");
        string s = string.Join(",",classType.fields.Select(f=>f.name));
        Log.Error(location, "Available fields are:"+s);
        return new FieldSymbol(identifier.name, ErrorType.Instance, true);
    }

    private static List<SmartcastMemberAccessSymbol> smartcastMemberAccessSymbols = [];

    private void SetSmartcastMemberAccessSymbol() {
        Symbol? lhs = left.GetSymbol();
        if (lhs==null)
            return;
        foreach(SmartcastMemberAccessSymbol s in smartcastMemberAccessSymbols) {
            if (s.lhs == lhs && s.rhs == field) {
                smartcastMemberAccessSymbol = s;
                return;
            }
        }
        SmartcastMemberAccessSymbol n = new(lhs, field);
        smartcastMemberAccessSymbols.Add(n);
        smartcastMemberAccessSymbol = n;
    }

    public override Symbol? GetSymbol() {
        return smartcastMemberAccessSymbol;
    } 

    public override void TypeCheckRvalue(AstBlock scope, PathContext pathContext) {
        left.TypeCheckRvalue(scope, pathContext);
        if (left.type.IsErrorType()) {
            SetError();
            return;
        }

        if (left.type is NullableType)
            Log.Error(location, $"Cannot access member {identifier.name} of nullable type {left.type}");

        Type leftType = left.type is NullableType nt ? nt.elementType : left.type;


        if (leftType is ClassType classType) {
            field = classType.GetField(identifier.name) ?? UndefinedField(classType);
            SetSmartcastMemberAccessSymbol();
            if (smartcastMemberAccessSymbol!=null)
                SetType(pathContext.LookupType(smartcastMemberAccessSymbol));
            else
                SetType(field.type);
        } else if ((leftType is StringType || leftType is ArrayType) && identifier.name=="length") {
            field = StdLib.lengthField;
            SetType(IntType.Instance);
            SetSmartcastMemberAccessSymbol();
        } else {
            SetError(location, $"Left side of member access must be a class, not {left.type}");
        }

    }

    public override void TypeCheckLvalue(AstBlock scope, PathContext pathContext) {
        TypeCheckRvalue(scope, pathContext);
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