
abstract class Type (string name) {
    public string name = name;
    public static readonly AstBlock predefindedScope = new AstTop().AddPredefinedSymbols();
    public static readonly Type undefined = UndefinedType.Instance;
    public bool descriptorUsed = false;

    override public string ToString() {
        return name;
    }

    public bool IsAssignableFrom(Type other) {
        return this == other || this == ErrorType.Instance || other == ErrorType.Instance ||
        (this==PointerType.Instance && (other is ArrayType || other is ClassType || other is NullableType || other is NullType || other is StringType)) ||
        (this is NullableType && other is NullType) ||
        (this is AnyType) || 
        (this is NullableType nt && nt.elementType.IsAssignableFrom(other))
        ;
    }

    public bool IsAssignableFrom(AstExpression expr) {
        // Special case -> allow integer expressions to be assigned to char/short, if can be proved within range at compile time.
        // Bit messy as I currently allow for both signed and unsigned bytes. 
        if (this is CharType && expr.HasKnownIntValue() && expr.GetKnownIntValue() is >=-128 and <=255 )
            return true;
        if (this is ShortType && expr.HasKnownIntValue() && expr.GetKnownIntValue() is >=-0x8000 and <=0x8000)
            return true;
        return IsAssignableFrom(expr.type);
    }

    public void CheckAssignableFrom(AstExpression expr) {
        if (!IsAssignableFrom(expr))
            Log.Error(expr.location, $"Type {expr.type} is not assignable to {this}");
    }

    public void CheckAssignableFrom(AstExpression expr, TokenKind op) {
        if (!IsAssignableFrom(expr))
            Log.Error(expr.location, $"At operator {op} Got type {expr.type} when expecting {this}");
    }


    public bool IsErrorType() {
        return this == ErrorType.Instance;
    }

    public int GetSize() {
        return this switch {
            BoolType => 1,
            CharType => 1,
            ShortType => 2,
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

class ShortType : Type {
    public static readonly ShortType Instance = new();
    private ShortType() : base("Short") {}
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

class AnyType : Type {
    public static readonly AnyType Instance = new();
    private AnyType() : base("Any") {}
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
    public readonly static HashSet<ArrayType> allArrayTypes = [];
    public readonly TypeSymbol typeSymbol;
    
    private ArrayType(Type elementType) : base($"Array<{elementType}>") {
        this.elementType = elementType;
        typeSymbol = new TypeSymbol(name,this);
    }

    public static Type MakeArrayType(Type elementType) {
        if (elementType == ErrorType.Instance)
            return ErrorType.Instance;
        foreach (ArrayType t in allArrayTypes)
            if (t.elementType == elementType)
                return t;
        ArrayType r = new(elementType);
        allArrayTypes.Add(r);
        return r;
    }
}

class VariadicType : Type {
    public Type elementType;
    private readonly static List<VariadicType> allClassTypes = [];
    
    private VariadicType(Type elementType) : base($"{elementType}...") {
        this.elementType = elementType;
    }

    public static Type MakeVariadicType(Type elementType) {
        if (elementType == ErrorType.Instance)
            return ErrorType.Instance;
        foreach (VariadicType t in allClassTypes)
            if (t.elementType == elementType)
                return t;
        VariadicType r = new(elementType);
        allClassTypes.Add(r);
        return r;
    }
}



class FunctionType : Type {
    public Type returnType;
    public List<Type> parameterTypes;

    static readonly List<FunctionType> allClassTypes = [];

    // TODO - the code for generating names of variadic functions is not quite right -> is needs to be element of for the last element
    private FunctionType(Type returnType, List<Type> parameterTypes) 
    : base($"({string.Join(", ",parameterTypes)}) -> {returnType}>") {
        this.returnType = returnType;
        this.parameterTypes = parameterTypes;
    }

    public static FunctionType MakeFunctionType(List<Type> parameterTypes, Type returnType) {
        foreach (FunctionType t in allClassTypes)
            if (t.returnType == returnType  && t.parameterTypes.SequenceEqual(parameterTypes))
                return t;
        return new FunctionType(returnType, parameterTypes);
    }
}

class NullableType : Type {
    public Type elementType;
    private readonly static List<NullableType> allClassTypes = [];

    private NullableType(Type elementType) : base($"{elementType}?") {
        this.elementType = elementType;
    }

    public static Type MakeNullableType(Type elementType) {
        if (elementType == ErrorType.Instance)
            return ErrorType.Instance;
        foreach (NullableType t in allClassTypes)
            if (t.elementType == elementType)
                return t;
        NullableType r = new(elementType);
        allClassTypes.Add(r);
        return r;
    }
}

// Used to represent type parameters (e.g. T in List<T>)
class TypeParameterType(string name) : Type(name) {
}

class GenericClassType: Type {
    public readonly AstClass constructor;
    public readonly List<FieldSymbol> fields = [];
    public readonly List<FunctionSymbol> methods = [];
    public readonly List<FunctionSymbol> virtualMethods = [];
    public readonly List<TypeParameterType> typeParameters;
    public bool isExternal = false;

    public int size;
    
    private readonly static List<GenericClassType> allClasses = [];

    public GenericClassType(string name, AstClass constructor, List<TypeParameterType> typeParameters) : base(name) {
        this.constructor = constructor;
        this.typeParameters = typeParameters;
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
        if (func.function.qualifiers.Contains(TokenKind.Virtual)) {
            virtualMethods.Add(func);
            func.function.virtualMethodNumber = virtualMethods.Count;
        }
    }
}

class ClassType : Type {
    public  readonly GenericClassType generic;
    private readonly List<Type> typeArguments;
    public  readonly Dictionary<TypeParameterType,Type> typeMap = [];

    public readonly List<FieldSymbol> fields = [];
    public readonly List<FunctionSymbol> methods = [];
    public readonly List<FunctionSymbol> virtualMethods = [];
    public readonly TypeSymbol typeSymbol;
    public bool isExternal = false;

    public readonly static List<ClassType> allClassTypes = [];

    private ClassType(GenericClassType generic, List<Type> typeArguments) 
    : base(typeArguments.Count==0 ? $"{generic.name}" : $"{generic.name}<{string.Join(", ", typeArguments)}>") 
    {
        this.generic = generic;
        this.typeArguments = typeArguments;
        isExternal = generic.isExternal;
        typeSymbol = new TypeSymbol(name, this);
        allClassTypes.Add(this);

        // Build the map to map the generic class types into concrete types
        if (typeArguments.Count != generic.typeParameters.Count)
            throw new Exception($"Invalid number of type arguments for class {generic.name}");  
        for(int i=0; i<typeArguments.Count; i++)
            typeMap[generic.typeParameters[i]] = typeArguments[i];

    }

    public static ClassType MakeClassType(GenericClassType generic, List<Type> typeArguments) {
        foreach (ClassType t in allClassTypes)
            if (t.generic == generic && t.typeArguments.SequenceEqual(typeArguments))
                return t;
        return new ClassType(generic, typeArguments);
    }

    public Type MapType(Type type) {
        if (type is TypeParameterType tpt)
            return typeMap[tpt];
        else if (type is NullableType nt)
            return NullableType.MakeNullableType(MapType(nt.elementType));
        else if (type is ArrayType at)  
            return ArrayType.MakeArrayType(MapType(at.elementType));
        else if (type is VariadicType vt)  
            return VariadicType.MakeVariadicType(MapType(vt.elementType));
        else if (type is FunctionType ft) {
            List<Type> mappedParameters = ft.parameterTypes.Select(MapType).ToList();
            return FunctionType.MakeFunctionType(mappedParameters, MapType(ft.returnType));
        } else if (type is ClassType cit)
            return MakeClassType(cit.generic, cit.typeArguments.Select(MapType).ToList());
        else
        return type;
    }

    public List<Type> GetConstrutorParameters() {
        List<Type> ret = [];
        foreach(Symbol param in generic.constructor.parameters)
            ret.Add(MapType(param.type));
        return ret;
    }

    public int GetInstanceSize() {
        return generic.size;
    }

    public void UpdateFieldsAndMethods() {
        // It is possible that some fields or methods have been added to the generic class
        // since the class type was created. So we need to update the fields and methods
        if (generic.fields.Count == fields.Count && generic.methods.Count == methods.Count) 
            return;

        // Add fields
        for(int index=fields.Count; index<generic.fields.Count; index++) {
            FieldSymbol genericField = generic.fields[index];
            FieldSymbol mappedField = new FieldSymbol(genericField.name, MapType(genericField.type), genericField.isMutable);
            mappedField.offset = genericField.offset;
            fields.Add(mappedField);
        }

        // Add methods
        for(int index=methods.Count; index<generic.methods.Count; index++) {
            FunctionSymbol genericMethod = generic.methods[index];
            FunctionSymbol mappedMethod = new FunctionSymbol(genericMethod.name, MapType(genericMethod.type), genericMethod.function);
            methods.Add(mappedMethod);
            if (genericMethod.function.qualifiers.Contains(TokenKind.Virtual)) {
                mappedMethod.function.virtualMethodNumber = genericMethod.function.virtualMethodNumber;
                virtualMethods.Add(mappedMethod);
            }
        }

    }

    public Symbol? GetField(string name) {
        UpdateFieldsAndMethods();

        foreach (FieldSymbol field in fields)
            if (field.name == name)
                return field;
    
        foreach (FunctionSymbol method in methods)
            if (method.name == name)
                return method;
    
        return null;
    }
}