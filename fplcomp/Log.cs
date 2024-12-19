
static class Log {
    public static int numErrors = 0;

    public static void Error(Location location, string message) {
        Console.WriteLine($"{location}: {message}");
        numErrors++;
    }

    public static void Error(string message) {
        Console.WriteLine(message);
        numErrors++;
    }


}