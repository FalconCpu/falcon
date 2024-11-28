
abstract class Type (string name) {
    public string name = name;
    public static readonly AstBlock predefindedScope = new AstTop().AddPredefinedSymbols();
    public static readonly Type undefined = UndefinedType.Instance;

    override public string ToString() {
        return name;
    }

    public bool IsAssignableFrom(Type other) {
        return this == other || this == ErrorType.Instance || other == ErrorType.Instance ||
        (this==PointerType.Instance && (other is ArrayType || other is ClassType || other is NullableType || other is NullType || other is StringType)) ||
        (this is NullableType && other is NullType) ||
        (this is NullableType nt && nt.elementType.IsAssignableFrom(other))
        ;
    }

    public void CheckAssignableFrom(AstExpression expr) {
        // Special case -> allow integer expressions to be assigned to char, if can be proved within range at compile time.
        // Bit messy as I currently allow for both signed and unsigned bytes. 
        if (this is CharType && expr.HasKnownIntValue() && expr.GetKnownIntValue() is >=-128 and <=255 )
            return;

        if (!IsAssignableFrom(expr.type))
            Log.Error(expr.location, $"Type {expr.type} is not assignable to {this}");
    }

    public bool IsErrorType() {
        return this == ErrorType.Instance;
    }

    public int GetSize() {
        return this switch {
            BoolType => 1,
            CharType => 1,
            IntType => 4,
            StringType => 4,
            RealType => 8,
            _ => 4,
        };
    }
}

class UndefinedType : Type {
    public static readonly UndefinedType Instance = new();
    private UndefinedType() : base("Undefined") {}
}


class IntType : Type {
    public static readonly IntType Instance = new();
    private IntType() : base("Int") {}
}

class BoolType : Type {
    public static readonly BoolType Instance = new();
    private BoolType() : base("Bool") {}
}

class CharType : Type {
    public static readonly CharType Instance = new();
    private CharType() : base("Char") {}
}

class StringType : Type {
    public static readonly StringType Instance = new();
    private StringType() : base("String") {}
}

class RealType : Type {
    public static readonly RealType Instance = new();
    private RealType() : base("Real") {}
}

class NullType : Type {
    public static readonly NullType Instance = new();
    private NullType() : base("Null") {}
}

class UnitType : Type {
    public static readonly UnitType Instance = new();
    private UnitType() : base("Unit") {}
}

class PointerType : Type {
    public static readonly PointerType Instance = new();
    private PointerType() : base("Pointer") {}
}


class ErrorType : Type {
    public static readonly ErrorType Instance = new();
    private ErrorType() : base("<Error>") {}
    
    public static ErrorType MakeErrorType(Location location, string message) {
        Log.Error(location, message);
        return Instance;
    }
}

class ArrayType : Type {
    public Type elementType;
    private readonly static List<ArrayType> cache = [];
    
    private ArrayType(Type elementType) : base($"Array<{elementType}>") {
        this.elementType = elementType;
    }

    public static Type MakeArrayType(Type elementType) {
        if (elementType == ErrorType.Instance)
            return ErrorType.Instance;
        foreach (ArrayType t in cache)
            if (t.elementType == elementType)
                return t;
        ArrayType r = new(elementType);
        cache.Add(r);
        return r;
    }
}

class FunctionType : Type {
    public Type returnType;
    public List<Type> parameterTypes;

    static readonly List<FunctionType> cache = [];

    private FunctionType(Type returnType, List<Type> parameterTypes) 
    : base($"({string.Join(", ",parameterTypes)}) -> {returnType}>") {
        this.returnType = returnType;
        this.parameterTypes = parameterTypes;
    }

    public static FunctionType MakeFunctionType(List<Type> parameterTypes, Type returnType) {
        foreach (FunctionType t in cache)
            if (t.returnType == returnType && t.parameterTypes == parameterTypes)
                return t;
        return new FunctionType(returnType, parameterTypes);
    }
}

class NullableType : Type {
    public Type elementType;
    private readonly static List<NullableType> cache = [];

    private NullableType(Type elementType) : base($"{elementType}?") {
        this.elementType = elementType;
    }

    public static Type MakeNullableType(Type elementType) {
        if (elementType == ErrorType.Instance)
            return ErrorType.Instance;
        foreach (NullableType t in cache)
            if (t.elementType == elementType)
                return t;
        NullableType r = new(elementType);
        cache.Add(r);
        return r;
    }
}

class ClassType: Type {
    public readonly AstClass constructor;
    public readonly List<FieldSymbol> fields = [];
    public readonly List<FunctionSymbol> methods = [];
    public int size;
    
    private readonly static List<ClassType> allClasses = [];

    public ClassType(string name, AstClass constructor) : base(name) {
        this.constructor = constructor;
        allClasses.Add(this);
    }

    public void AddField(FieldSymbol field) {
        fields.Add(field);
        int fieldSize = field.type.GetSize();
        
        // Add padding if necessary
        if (fieldSize>=4)
            size = (size+3)&-4;
        field.offset = size;
        size += fieldSize;
    }

    public void AddMethod(FunctionSymbol func) {
        methods.Add(func);
    }

    public Symbol? GetField(string name) {
        foreach (FieldSymbol field in fields)
            if (field.name == name)
                return field;
        foreach (FunctionSymbol method in methods)
            if (method.name == name)
                return method;
        return null;
    }

}