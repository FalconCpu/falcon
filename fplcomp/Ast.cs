abstract class Ast(Location location) {
    public Location location = location;
    
    abstract public void Print(int indent);
}