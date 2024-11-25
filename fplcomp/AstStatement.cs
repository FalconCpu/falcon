
abstract class AstStatement(Location location) : Ast(location) {
    
    abstract public void TypeCheck(AstBlock scope);

    abstract public void CodeGen(AstFunction func);
}