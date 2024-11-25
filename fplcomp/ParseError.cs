
class ParseError(Location location, string message) : Exception(message) {
    public Location location = location;
}