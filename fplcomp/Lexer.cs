using System.Text;

class Lexer (string fileName) {

    private readonly string    fileName = fileName;
    private          int       line = 1;
    private          int       column = 1;
    private          char      lookahead = '\0';
    private          bool      eof = false;
    private readonly List<int> indentStack = [1];
    private          bool      atStartOfLine = true;
    private          bool      lineContinues = true;

    private readonly StreamReader reader =  new(fileName);

    static Dictionary<string,TokenKind> predefined_tokens = new() {
        {"+" , TokenKind.Plus},
        {"-" , TokenKind.Minus},
        {"*" , TokenKind.Star},
        {"/" , TokenKind.Slash},
        {"%" , TokenKind.Percent},
        {"&" , TokenKind.Ampersand},
        {"|" , TokenKind.Bar},
        {"^" , TokenKind.Carat},
        {"=" , TokenKind.Eq},
        {"!=" , TokenKind.Neq},
        {"<" , TokenKind.Lt},
        {"<=" , TokenKind.Lte},
        {">" , TokenKind.Gt},
        {">=" , TokenKind.Gte},
        {"and" , TokenKind.And},
        {"or" , TokenKind.Or},
        {"not" , TokenKind.Not},
        {"then" , TokenKind.Then},
        {"new" , TokenKind.New},
        {"->" , TokenKind.Arrow},
        {"lsl" , TokenKind.Lsl},
        {"lsr" , TokenKind.Lsr},
        {"asr" , TokenKind.Asr},
        {"." , TokenKind.Dot},
        {"," , TokenKind.Comma},
        {":" , TokenKind.Colon},
        {"~" , TokenKind.Tilde},
        {"?" , TokenKind.Qmark},
        {"!" , TokenKind.Emark},
        {"to" , TokenKind.To},
        {"downto" , TokenKind.Downto},
        {"(" , TokenKind.OpenB},
        {"[" , TokenKind.OpenSq},
        {"{" , TokenKind.OpenCl},
        {")" , TokenKind.CloseB},
        {"]" , TokenKind.CloseSq},
        {"}" , TokenKind.CloseCl},
        {"val" , TokenKind.Val},
        {"var" , TokenKind.Var},
        {"const" , TokenKind.Const},
        {"if" , TokenKind.If},
        {"else" , TokenKind.Else},
        {"elsif" , TokenKind.Elsif},
        {"end" , TokenKind.End},
        {"fun" , TokenKind.Fun},
        {"return" , TokenKind.Return},
        {"while" , TokenKind.While},
        {"repeat" , TokenKind.Repeat},
        {"until" , TokenKind.Until},
        {"for" , TokenKind.For},
        {"Array" , TokenKind.Array}, 
        {"class" , TokenKind.Class},
        {"print" , TokenKind.Print},
        {"rc",     TokenKind.Rc},
        {"delete", TokenKind.Delete}
    };

    private char NextChar() {
        char ret = lookahead;

        int l = reader.Read();
        if (l == -1) {
            eof = true;
            lookahead = '\0';
        } else {
            lookahead = (char)l;
        }

        if (ret=='\n') {
            line++;
            column = 1;
        } else if (ret!=0)
            column++;

        return ret;
    }

    private void SkipWhiteSpaceAndComments() {
        while ((lookahead==' ' || lookahead=='\t' || lookahead == '#' || lookahead=='\r' || lookahead==0 || (lookahead=='\n' && lineContinues)) && !eof)
            if (lookahead == '#')
                while (lookahead != '\n' && !eof)
                    NextChar();
            else
                NextChar();
    }

    private string ReadWord() {
        StringBuilder ret = new();
        while (char.IsLetterOrDigit(lookahead) || lookahead == '_')
            ret.Append(NextChar());
        return ret.ToString();
    }

    private string ReadPunctuation() {
        char c = NextChar();
        if ( (c=='!' && lookahead=='=') ||
             (c=='<' && lookahead=='=') ||
             (c=='>' && lookahead=='=') ||
             (c=='-' && lookahead=='>') )
             return "" + c + NextChar();
        return "" + c;
    }

    private char ReadEscapeChar() {
        if (lookahead == '\\') {
            NextChar();
            char c = NextChar();
            return c switch {
                'n' => '\n',
                't' => '\t',
                'r' => '\r',
                '0' => '\0',
                _ => c,
            };
        }

        return NextChar();
    }

    private string ReadString(Location location) {
        StringBuilder ret = new();
        NextChar();
        while (lookahead != '"' && !eof)
            ret.Append(ReadEscapeChar());

        if (lookahead != '"')
            Log.Error(location, "Unterminated string");
        else
            NextChar();
        return ret.ToString();
    }

    private string ReadCharLit(Location location) {
        StringBuilder ret = new();
        ret.Append(NextChar());  // Skip over the opening quote

        if (lookahead == '\'')
            Log.Error(location, "Empty char literal");
        else
            ret.Append(ReadEscapeChar());

        if (lookahead != '\'')
            Log.Error(location, "Unterminated char literal");
        else
            ret.Append(NextChar());

        return ret.ToString();
    }


    public Token NextToken() {
        SkipWhiteSpaceAndComments();

        Location location = new(fileName, line, column);
        string text;
        TokenKind kind;

        if (eof) {
            if (!atStartOfLine) {
                text = "<End of Line>";
                kind = TokenKind.EndOfLine;
            } else if (indentStack.Count > 1) {
                indentStack.RemoveAt(indentStack.Count-1);
                text = "<Dedent>";
                kind = TokenKind.Dedent;
            } else {
                text = "<End of File>";
                kind = TokenKind.EndOfFile;
            }

        } else if (atStartOfLine && column>indentStack.Last()) {
            indentStack.Add(column);
            text = "<Indent>";
            kind = TokenKind.Indent;

        } else if (atStartOfLine && column<indentStack.Last()) {
            indentStack.RemoveAt(indentStack.Count-1);
            text = "<Dedent>";
            kind = TokenKind.Dedent;
            if (column > indentStack.Last())
                Log.Error(location, $"Incorrect indentation - expected column {indentStack.Last()}");

        } else if (lookahead == '\n') {
            NextChar();
            text = "<End of Line>";
            kind = TokenKind.EndOfLine;

        } else if (char.IsDigit(lookahead)) {
            text = ReadWord();
            kind = TokenKind.Integer;
        
        } else if (char.IsLetter(lookahead)) {
            text = ReadWord();
            kind = predefined_tokens.GetValueOrDefault(text, TokenKind.Identifier);

        } else if (lookahead == '"') {
            text = ReadString(location);
            kind = TokenKind.String;

        } else if (lookahead == '\'') {
            text = ReadCharLit(location);
            kind = TokenKind.CharLit;

        } else {
            text = ReadPunctuation();
            kind = predefined_tokens.GetValueOrDefault(text, TokenKind.Error);
        }

        if (kind == TokenKind.Error)    
            Log.Error(location, $"Unexpected character '{lookahead}'");

        atStartOfLine = (kind==TokenKind.EndOfLine) || (kind==TokenKind.Dedent);
        lineContinues = (kind>=TokenKind.Plus && kind<=TokenKind.OpenCl) || (kind==TokenKind.EndOfLine);
        return new Token(location, kind, text);
    }
}