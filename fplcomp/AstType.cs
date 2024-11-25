
abstract class AstType(Location location) : Ast(location) {
    public abstract Type ResolveAsType(AstBlock scope);
}