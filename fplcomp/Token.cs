
class Token (Location location, TokenKind kind, string text) {
    public Location  location = location;
    public TokenKind kind = kind;
    public string    text = text;

    public override string ToString() {
        return $"{location}: {kind} {text}";
    }
}

enum TokenKind {
    EndOfFile,
    EndOfLine,
    Indent,
    Dedent,
    Identifier,
    Integer,
    String,
    RealLit,
    Plus,   // Tokens after this point are line continuation tokens
    Minus,
    Star,
    Slash,
    Percent,
    Ampersand,
    Bar,
    Carat,
    Eq,
    Neq,
    Lt,
    Lte,
    Gte,
    And,
    Or,
    Not,
    Dot,
    Comma,
    Colon,
    Tilde,
    Arrow,
    Then,
    New,
    OpenB,
    OpenSq,
    OpenCl,     // Tokens before this point are line continuation tokens
    CloseB,
    CloseSq,
    CloseCl,
    Gt,         // Cannot allow > to be a line continuation as its use in generics
    Qmark,
    Emark,
    Array,
    Val,
    Var,
    If,
    Else,
    Elsif,
    End,
    Fun,
    Return,
    While,
    Repeat,
    Until,
    Class,
    Print,
    Error
}