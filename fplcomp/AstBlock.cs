
abstract class AstBlock(Location location, AstBlock? parent) : AstStatement(location) {
    public AstBlock? parent = parent;
    public List<AstStatement> statements = [];
    private readonly Dictionary<string, Symbol> symbols = [];

    public void Add(AstStatement statement) {
        statements.Add(statement);
    }

    // Look up a symbol in the current block, or recursively in parent blocks
    public Symbol? GetSymbol(string name) {
        Symbol? symbol = symbols.GetValueOrDefault(name);
        if (symbol == null && parent != null)
            symbol = parent.GetSymbol(name);
        return symbol;
    }

    // Add a symbol to the current scope (block)
    public void AddSymbol(Location location, Symbol symbol) {
        if (symbols.ContainsKey(symbol.name))
            Log.Error(location, $"Duplicate symbol {symbol.name}");
        symbols[symbol.name] = symbol;
    }

    // Find the enclosing function
    public AstFunction? FindEnclosingFunction() {
        AstBlock? block = this;
        while (block != null) {
            if (block is AstFunction func)
                return func;
            block = block.parent;
        }
        return null;
    }
}