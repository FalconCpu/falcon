
public static class Debug {
    // static readonly bool debug = true;
    static readonly StreamWriter log = new("debug.log");

    public static void Print(string s) {
        log.Write(s);
    }

    public static void Print(char c) {
        log.Write(c);
    }

    public static void PrintLn(string? s) {
        if (s!=null) 
            log.Write(s);
        log.Write("\n");
        log.Flush();
    }

    public static void Close() {
        log.Close();
    }
}