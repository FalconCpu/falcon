
class Location (string fileName, int line, int column) {
    public string fileName = fileName;
    public int line = line;
    public int column = column;

    public static Location unknown = new Location("",0,0);

    public override string ToString() {
        return $"{fileName}:{line}:{column}";
    }
}