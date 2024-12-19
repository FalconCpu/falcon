
class AstIdentifier (Location location,string name) : AstExpression(location) {
    public string name = name;
    public Symbol symbol = Symbol.undefined;

    public override void Print(int indent) {
        Console.WriteLine(new string(' ', indent * 2) + "IDENTIFIER " + name + " (" + type + ")");
    }

    public override string ToString() {
        return name;
    }

    public void SetSymbol(Symbol newSym) {
        if (symbol != Symbol.undefined)
            Log.Error(location, $"Cannot redeclare symbol '{name}'");
        symbol = newSym;
        type = newSym.type;
    }

    public override void TypeCheckRvalue(AstBlock scope, PathContext pathContext) {
        symbol =  Type.predefindedScope.GetSymbol(name) ??
                      scope.GetSymbol(name) ??
                      new VariableSymbol(name, ErrorType.MakeErrorType(location, $"Unknown symbol '{name}'"), false, false);
        
        if (pathContext.definitelyUninitialized.Contains(symbol))
            Log.Error(location, $"Symbol '{symbol}' is uninitialized");
        else if (pathContext.possiblyUninitialized.Contains(symbol))    
            Log.Error(location, $"Symbol '{symbol}' may be uninitialized");
            
        if (symbol is TypeSymbol)
            Log.Error(location, $"Cannot use type '{symbol}' as a variable");
        SetType(symbol.type);
    }


    public override void TypeCheckLvalue(AstBlock scope, PathContext pathContext) {
        symbol =  Type.predefindedScope.GetSymbol(name) ??
                      scope.GetSymbol(name) ??
                      new VariableSymbol(name, ErrorType.MakeErrorType(location, $"Unknown symbol '{name}'"), false, false);
        
        if (symbol is TypeSymbol)
            Log.Error(location, $"Cannot use type '{symbol}' as a variable");
        SetType(symbol.type);

        switch(symbol) {
            case VariableSymbol variableSymbol:
                if (variableSymbol.isGlobal)
                    throw new NotImplementedException();
                if (!variableSymbol.isMutable && !pathContext.definitelyUninitialized.Contains(symbol))
                    Log.Error(location, $"Symbol '{symbol}' is immutable");
                break;

            case FieldSymbol fieldSymbol:
                if (!fieldSymbol.isMutable)
                    Log.Error(location, $"Symbol '{symbol}' is immutable");
                break;

            default:
                Log.Error(location, $"Symbol '{symbol}' is not an lvalue");
                break;
        }
    }

    private static void CheckThisHas(Symbol thisSymbol, FieldSymbol field) {
        // Sanity check that the symbol we have encountered really does 
        // come from 'this'. If so something has gone badly wrong in the 
        // type checking up to this point -> throw an exception.

        if (thisSymbol.type is ClassType classType) {
            classType.UpdateFieldsAndMethods();

            if (! classType.generic.fields.Contains(field)) {
                // Console.Out.WriteLine("fields:-");
                // Console.Out.WriteLine(string.Join(",",classType.fields));
                throw new ArgumentException($"Field '{field}' is not a member of class '{classType}'");
            }
        } else if (thisSymbol.type is GenericClassType gClassType) {
            if (! gClassType.fields.Contains(field))
                throw new ArgumentException($"Field '{field}' is not a member of class '{gClassType}'");
        } else
            throw new ArgumentException($"Symbol '{thisSymbol}' is not a class type  {thisSymbol.type}");
    }

    public override bool HasKnownIntValue() => symbol is ConstantSymbol;
    public override int GetKnownIntValue() => (symbol is ConstantSymbol constantSymbol) ? constantSymbol.value : throw new InvalidOperationException(); 

    public override Symbol CodeGenRvalue(AstFunction func) {
        switch (symbol) {
            case VariableSymbol variableSymbol:
                if (variableSymbol.isGlobal)
                    throw new NotImplementedException();
                return variableSymbol;

            case FunctionSymbol functionSymbol:
                return functionSymbol;

            case FieldSymbol fieldSymbol:
                if (func.thisSymbol==null) throw new ArgumentException("Cannot find 'this' symbol");
                CheckThisHas(func.thisSymbol, fieldSymbol);
                Symbol ret = func.NewTemp(type);
                func.Add(new InstrLoadField(type.GetSize(), ret, func.thisSymbol, fieldSymbol));
                return ret;

            case ConstantSymbol isymbol:
                func.Add(new InstrLdi(isymbol, isymbol.value));
                return symbol;

            case ConstObjectAliasSymbol cas: 
                ret = func.NewTemp(type);
                func.Add(new InstrLea(ret, cas.alias));
                return ret;

            default:
                throw new NotImplementedException();
        }
    }


    public override void CodeGenLvalue(AstFunction func, AluOp op,Symbol value) {
        switch (symbol) {
            case VariableSymbol variableSymbol:
                if (variableSymbol.isGlobal)
                    throw new NotImplementedException();
                if (op==AluOp.UNDEFINED)
                    func.Add(new InstrMov(variableSymbol, value));
                else {
                    Symbol temp = func.NewTemp(type);
                    func.Add(new InstrAlu(temp, op, variableSymbol, value));
                    func.Add(new InstrMov(variableSymbol, temp));
                }
                break;

            case FieldSymbol fieldSymbol:
                if (func.thisSymbol==null) throw new ArgumentException("Cannot find 'this' symbol");
                CheckThisHas(func.thisSymbol, fieldSymbol);
                if (op==AluOp.UNDEFINED)
                    func.Add(new InstrStoreField(type.GetSize(), value, func.thisSymbol, fieldSymbol));
                else {
                    Symbol temp1 = func.NewTemp(type);
                    Symbol temp2 = func.NewTemp(type);
                    func.Add(new InstrLoadField(type.GetSize(), temp1, func.thisSymbol, fieldSymbol));
                    func.Add(new InstrAlu(temp2, op, temp1, value));
                    func.Add(new InstrStoreField(type.GetSize(), temp2, func.thisSymbol, fieldSymbol));
                }
            break;

            default:
                throw new NotImplementedException();
        }
    }
}

class VariableSymbol(string name, Type type, bool isGlobal, bool isMutable) : Symbol(name, type) {
    public readonly bool isGlobal = isGlobal;
    public readonly bool isMutable = isMutable;
}