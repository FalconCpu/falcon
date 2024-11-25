
class AstIdentifier (Location location,string name) : AstExpression(location) {
    public string name = name;
    private Symbol symbol = Symbol.undefined;

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

    public override void TypeCheckRvalue(AstBlock scope) {
        symbol =  Type.predefindedScope.GetSymbol(name) ??
                      scope.GetSymbol(name) ??
                      new VariableSymbol(name, ErrorType.MakeErrorType(location, $"Unknown symbol '{name}'"), false, false);
        if (symbol is TypeSymbol)
            Log.Error(location, $"Cannot use type '{symbol}' as a variable");
        SetType(symbol.type);
    }


    public override void TypeCheckLvalue(AstBlock scope) {
        TypeCheckRvalue(scope);
        if (symbol is VariableSymbol variableSymbol) {
            if (!variableSymbol.isMutable)
                Log.Error(location, $"Symbol '{symbol}' is immutable");
            return;
        } else {
            Log.Error(location, $"Symbol '{symbol}' is not an lvalue");
        }
    }

    private static void CheckThisHas(Symbol thisSymbol, FieldSymbol field) {
        // Sanity check that the symbol we have encountered really does 
        // come from 'this'. If so something has gone badly wrong in the 
        // type checking up to this point -> throw an exception.

        if (thisSymbol.type is ClassType classType) {
            if (! classType.fields.Contains(field))
                throw new ArgumentException($"Field '{field}' is not a member of class '{classType}'");
        } else
            throw new ArgumentException($"Symbol '{thisSymbol}' is not a class type");
    }


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

            default:
                throw new NotImplementedException();
        }
    }


    public override void CodeGenLvalue(AstFunction func, Symbol value) {
        switch (symbol) {
            case VariableSymbol variableSymbol:
                if (variableSymbol.isGlobal)
                    throw new NotImplementedException();
                func.Add(new InstrMov(variableSymbol, value));
                break;

            case FieldSymbol fieldSymbol:
                if (func.thisSymbol==null) throw new ArgumentException("Cannot find 'this' symbol");
                CheckThisHas(func.thisSymbol, fieldSymbol);
                func.Add(new InstrStoreField(type.GetSize(), value, func.thisSymbol, fieldSymbol));
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