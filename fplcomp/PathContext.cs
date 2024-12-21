
class PathContext() {
    public HashSet<Symbol>          definitelyUninitialized = [];
    public HashSet<Symbol>          possiblyUninitialized = [];
    public Dictionary<Symbol,Type>  refinedTypes = [];
    public bool                     unreachable = false;

    public void AddUninitialized(Symbol symbol) {
        definitelyUninitialized.Add(symbol);
        possiblyUninitialized.Add(symbol);
    }

    public void Initialize(Symbol symbol) {
        definitelyUninitialized.Remove(symbol);
        possiblyUninitialized.Remove(symbol);
    }

    public void Refine(Symbol? symbol, Type type) {
        if (symbol!=null) {
            // Console.WriteLine($"Refining symbol {symbol.name} to type {type}");
            refinedTypes[symbol] = type;
        }
    }

    public void Unrefine(Symbol symbol) {
        refinedTypes.Remove(symbol);

        // If we invalidate a symbol 'c' then also unrefine any member accesses to 'c'
        var toRemove = refinedTypes.Keys.Where(it => it is SmartcastMemberAccessSymbol sms && sms.lhs==symbol);
        foreach (var key in toRemove)
            refinedTypes.Remove(key);
    }

    public void SetUnreachable() {
        unreachable = true;
    }

    public Type LookupType(Symbol symbol) {
        Type ret;
        if (refinedTypes.ContainsKey(symbol)) 
            ret = refinedTypes[symbol];
        else
            ret = symbol.type;
        // Console.WriteLine($"Looking up type for symbol {symbol.name} {ret}");
        return ret;
    }

    // Static method to merge multiple PathContexts into one
    public static PathContext Merge(List<PathContext> contexts) {
        // Filter out unreachable contexts
        var reachableContexts = contexts.Where(ctx => !ctx.unreachable).ToList();

        // If all contexts are unreachable, return a single unreachable context
        if (reachableContexts.Count==0)
            return new PathContext { unreachable = true };

        // Initialize a new PathContext for the result
        PathContext merged = new();

        // Definitely uninitialized: Intersection of all reachable contexts
        var definitelyUninitialized = reachableContexts
            .Select(ctx => ctx.definitelyUninitialized)
            .Aggregate((a, b) => {
                var result = new HashSet<Symbol>(a);
                result.IntersectWith(b);
                return result;
            });

        // Possibly uninitialized: Union of all reachable contexts
        var possiblyUninitialized = reachableContexts
            .Select(ctx => ctx.possiblyUninitialized)
            .Aggregate((a, b) => {
                var result = new HashSet<Symbol>(a);
                result.UnionWith(b);
                return result;
            });

        // Refined types: Keep only refinements consistent across all reachable contexts
        var refinedTypes = reachableContexts
            .Select(ctx => ctx.refinedTypes)
            .Aggregate((a, b) => {
                var result = new Dictionary<Symbol, Type>();
                foreach (var pair in a) {
                    if (b.TryGetValue(pair.Key, out var typeB) && pair.Value.Equals(typeB))
                        result[pair.Key] = pair.Value;
                }
                return result;
            });

        return new PathContext {
            definitelyUninitialized = definitelyUninitialized,
            possiblyUninitialized = possiblyUninitialized,
            refinedTypes = refinedTypes,
            unreachable = false
        };
    }

    public PathContext Clone() {
        return new PathContext {
            definitelyUninitialized = new HashSet<Symbol>(definitelyUninitialized),
            possiblyUninitialized = new HashSet<Symbol>(possiblyUninitialized),
            refinedTypes = new Dictionary<Symbol, Type>(refinedTypes),
            unreachable = unreachable
    };
}

}
