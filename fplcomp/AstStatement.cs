
abstract class AstStatement(Location location) : Ast(location) {
    
    abstract public PathContext TypeCheck(AstBlock scope, PathContext pathContext);

    abstract public void CodeGen(AstFunction func);
}