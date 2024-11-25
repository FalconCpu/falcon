
using System.Runtime.InteropServices;

class Parser(Lexer lexer)
{
    private readonly Lexer lexer = lexer;
    private          Token lookahead = lexer.NextToken();

    private Token NextToken() {
        Token ret = lookahead;
        lookahead = lexer.NextToken();
        return ret;
    }

    private bool CanTake(TokenKind kind) {
        if (lookahead.kind==kind) {
            NextToken();
            return true;
        } else
            return false;
    }

    private Token Expect(TokenKind kind) {
        if (lookahead.kind == kind)
            return NextToken();
        throw new ParseError(lookahead.location, $"Expected {kind} but got {lookahead.text}");
    }

    private void SkipToEndOfLine() {
        while (lookahead.kind != TokenKind.EndOfLine && lookahead.kind != TokenKind.EndOfFile)
            NextToken();
        NextToken();
    }   

    private void ExpectEol() {
        if (lookahead.kind == TokenKind.EndOfLine)
            NextToken();
        else {
            Log.Error(lookahead.location, $"Got '{lookahead.text}' when expecting end of line");
            SkipToEndOfLine();
        }
    }

    private AstIdentifier ParseIdentifier() {
        Token tok = Expect(TokenKind.Identifier);
        return new(tok.location, tok.text);
    }

    private AstIntLit ParseIntLit() {
        Token tok = Expect(TokenKind.Integer);
        int value;
        try {
            if (tok.text.StartsWith("0x"))
                value = int.Parse(tok.text[2..], System.Globalization.NumberStyles.HexNumber);
            else
                value = int.Parse(tok.text);
        } catch (Exception) {
            Log.Error(tok.location, $"Malformed integer '{tok.text}'");
            value = 0;
        }

        return new(lookahead.location, value);
    }

    private AstStringLit ParseStringLit() {
        Token tok = Expect(TokenKind.String);
        return new(lookahead.location, tok.text);
    }

    private AstRealLit ParseRealLit() {
        Token tok = Expect(TokenKind.RealLit);

        double value;
        try {
            value = double.Parse(tok.text);
        } catch (Exception) {
            Log.Error(tok.location, $"Malformed real '{tok.text}'");
            value = 0.0;
        }

        return new(lookahead.location, value);
    }

    private AstExpression ParseBracket() {
        Expect(TokenKind.OpenB);
        AstExpression exp = ParseExpression();
        Expect(TokenKind.CloseB);
        return exp;
    }

    private AstExpression ParseNew() {
        Expect(TokenKind.New);
        AstType type = ParseType();
        Expect(TokenKind.OpenB);
        List<AstExpression> args = [];
        if (lookahead.kind != TokenKind.CloseB) {
            do {
                args.Add(ParseExpression());
            } while(CanTake(TokenKind.Comma));
        }
        Expect(TokenKind.CloseB);
        return new AstNew(lookahead.location, type, args);
    }

    private AstExpression ParsePrimary() {
        switch(lookahead.kind) {
            case TokenKind.Identifier:  return ParseIdentifier();
            case TokenKind.Integer:     return ParseIntLit();
            case TokenKind.String:      return ParseStringLit();
            case TokenKind.RealLit:     return ParseRealLit();
            case TokenKind.New:         return ParseNew();
            case TokenKind.OpenB:       return ParseBracket();
            default:                    throw new ParseError(lookahead.location, $"Got '{lookahead.text}' when expecting primary exppression");;
        }
    }   

    private AstFuncCall ParseFuncCall(AstExpression left) {
        Token loc = Expect(TokenKind.OpenB);
        List<AstExpression> args = [];
        if (lookahead.kind != TokenKind.CloseB) {
            do {
                args.Add(ParseExpression());
            } while(CanTake(TokenKind.Comma));
        }
        Expect(TokenKind.CloseB);
        return new AstFuncCall(loc.location, left, args);
    }

    private AstIndex ParseIndex(AstExpression left) {
        Token loc = Expect(TokenKind.OpenSq);
        AstExpression index = ParseExpression();
        Expect(TokenKind.CloseSq);
        return new AstIndex(loc.location, left, index);
    }

    private AstMember ParseMember(AstExpression left) {
        Expect(TokenKind.Dot);
        AstIdentifier member = ParseIdentifier();
        return new AstMember(left.location, left, member);
    }

    private AstExpression ParsePostfix() {
        AstExpression ret = ParsePrimary();
        while(true) 
            switch(lookahead.kind) {
                case TokenKind.OpenB: ret = ParseFuncCall(ret); break;
                case TokenKind.OpenSq: ret = ParseIndex(ret); break;
                case TokenKind.Dot: ret = ParseMember(ret); break;
                default: return ret;
            }   
    }

    private AstExpression ParsePrefix() {
        if (lookahead.kind == TokenKind.Minus || lookahead.kind == TokenKind.Tilde) {
            Token op = NextToken();
            AstExpression right = ParsePrefix();
            return new AstUnary(op.location, op.kind, right);
        } else
            return ParsePostfix();
    }

    private AstExpression ParseMul() {
        AstExpression ret = ParsePrefix();
        while (lookahead.kind == TokenKind.Star || lookahead.kind == TokenKind.Slash || 
               lookahead.kind == TokenKind.Percent || lookahead.kind == TokenKind.Ampersand) {
            Token op = NextToken();
            AstExpression right = ParsePrefix();
            ret = new AstBinop(op.location, op.kind, ret, right);
        }
        return ret;
    }

    private AstExpression ParseAdd() {
        AstExpression ret = ParseMul();
        while (lookahead.kind == TokenKind.Plus || lookahead.kind == TokenKind.Minus || 
               lookahead.kind == TokenKind.Bar || lookahead.kind == TokenKind.Carat) {
            Token op = NextToken();
            AstExpression right = ParseMul();
            ret = new AstBinop(op.location, op.kind, ret, right);
        }
        return ret;
    }

    private AstExpression ParseComp() {
        AstExpression ret = ParseAdd();
        while (lookahead.kind == TokenKind.Eq || lookahead.kind == TokenKind.Neq ||
               lookahead.kind == TokenKind.Lt || lookahead.kind == TokenKind.Lte ||
               lookahead.kind == TokenKind.Gt || lookahead.kind == TokenKind.Gte) {
            Token op = NextToken();
            AstExpression right = ParseAdd();
            ret = new AstBinop(op.location, op.kind, ret, right);
        }
        return ret;
    }

    private AstExpression ParseAnd() {
        AstExpression ret = ParseComp();
        while (lookahead.kind == TokenKind.And) {
            Token op = NextToken();
            AstExpression right = ParseComp();
            ret = new AstBinop(op.location, op.kind, ret, right);
        }
        return ret;
    }

    private AstExpression ParseOr() {
        AstExpression ret = ParseAnd();
        while (lookahead.kind == TokenKind.Or) {
            Token op = NextToken();
            AstExpression right = ParseAnd();
            ret = new AstBinop(op.location, op.kind, ret, right);
        }
        return ret;
    }

    private AstExpression ParseExpression() {
        if (lookahead.kind == TokenKind.If) {
            Token loc = NextToken();
            AstExpression cond = ParseExpression();
            Expect(TokenKind.Then);
            AstExpression then = ParseExpression();
            Expect(TokenKind.Else);
            AstExpression els = ParseExpression();
            return new AstIfExpr(loc.location, cond, then, els);
        } else 
            return ParseOr();
    }

    private AstExpression? ParseOptExpression() {
        if (CanTake(TokenKind.Eq))
            return ParseExpression();
        else
            return null;
    }

    private AstTypeIdentifier ParseTypeIdentifier() {
        Token loc = Expect(TokenKind.Identifier);
        return new AstTypeIdentifier(loc.location, loc.text);
    }

    private AstTypeArray ParseTypeArray() {
        Token loc = Expect(TokenKind.Array);
        Expect(TokenKind.Lt);
        AstType type = ParseType();
        Expect(TokenKind.Gt);
        return new AstTypeArray(loc.location, type);
    }

    private AstType ParseType() {
        AstType ret = lookahead.kind switch {
            TokenKind.Identifier => ParseTypeIdentifier(),
            TokenKind.Array => ParseTypeArray(),
            _ => throw new Exception($"Got '{lookahead.kind}' when expecting type"),
        };

        if (lookahead.kind==TokenKind.Qmark)
            ret = new AstTypeNullable(NextToken().location, ret);

        return ret;
    }

    private AstType? ParseOptType() {
        if (CanTake(TokenKind.Colon))
            return ParseType();
        else
            return null;
    }

    private AstType? ParseOptReturnType() {
        if (CanTake(TokenKind.Arrow))
            return ParseType();
        else
            return null;
    }


    private AstParameter ParseParameter() {
        AstIdentifier name = ParseIdentifier();
        Expect(TokenKind.Colon);
        AstType type = ParseType();
        return new AstParameter(name.location, TokenKind.Val, name, type);
    }

    private List<AstParameter> ParseParameterList() {
        Expect(TokenKind.OpenB);
        List<AstParameter> parameters = [];
        if (lookahead.kind != TokenKind.CloseB)
            do {
                parameters.Add(ParseParameter());
             } while (CanTake(TokenKind.Comma));
        Expect(TokenKind.CloseB);
        return parameters;
    }

    private AstParameter ParseClassParameter() {
        TokenKind kind = TokenKind.EndOfLine;
        if (lookahead.kind == TokenKind.Val || lookahead.kind == TokenKind.Var)
            kind = NextToken().kind;
        AstIdentifier name = ParseIdentifier();
        Expect(TokenKind.Colon);
        AstType type = ParseType();
        return new AstParameter(name.location, kind, name, type);
    }

    private List<AstParameter> ParseClassParameterList() {
        Expect(TokenKind.OpenB);
        List<AstParameter> parameters = [];
        if (lookahead.kind != TokenKind.CloseB)
            do {
                parameters.Add(ParseClassParameter());
             } while (CanTake(TokenKind.Comma));
        Expect(TokenKind.CloseB);
        return parameters;
    }



    private void ParseFunction(AstBlock block) {
        Token loc = Expect(TokenKind.Fun);
        AstIdentifier name = ParseIdentifier();
        List<AstParameter> parameters = ParseParameterList();
        AstType? astReturnType = ParseOptReturnType();
        ExpectEol();
        AstFunction ret = new(loc.location, name.name, parameters, astReturnType, block);
        block.Add(ret);
        ParseIndentedBlock(ret);
        CheckEnd(TokenKind.Fun);
        
    }

    private void ParseClass(AstBlock block) {
        Token loc = Expect(TokenKind.Class);
        AstIdentifier name = ParseIdentifier();
        List<AstParameter> parameters = lookahead.kind==TokenKind.OpenB ? ParseClassParameterList() : [];
        ExpectEol();
        AstClass ret = new(loc.location, name.name, parameters, block);
        block.Add(ret);
        if (lookahead.kind == TokenKind.Indent)
            ParseIndentedBlock(ret);
        CheckEnd(TokenKind.Fun);
        
    }

    private void ParseDeclaration(AstBlock block) {
        Token kind = NextToken();
        AstIdentifier name = ParseIdentifier();
        AstType? astType = ParseOptType();
        AstExpression? expr = ParseOptExpression();
        ExpectEol();
        block.Add(new AstDeclaration(name.location, kind.kind, name, astType, expr));
    }

    private void ParseIndentedBlock(AstBlock block) {
        if (lookahead.kind != TokenKind.Indent) {
            Log.Error(lookahead.location, "Expected indented block");
            return;
        }

        Expect(TokenKind.Indent);
        while(lookahead.kind != TokenKind.Dedent && lookahead.kind != TokenKind.EndOfFile)
            ParseStatement(block);
        Expect(TokenKind.Dedent);
    }

    private void ParseIndentedBlockClass(AstBlock block) {
        if (lookahead.kind != TokenKind.Indent) {
            Log.Error(lookahead.location, "Expected indented block");
            return;
        }

        Expect(TokenKind.Indent);
        while(lookahead.kind != TokenKind.Dedent && lookahead.kind != TokenKind.EndOfFile)
            ParseStatementClass(block);
        Expect(TokenKind.Dedent);
    }


    private void CheckEnd(TokenKind kind) {
        if (CanTake(TokenKind.End)) {
            if (lookahead.kind == TokenKind.EndOfLine) {
                NextToken();
                return;
            } 
            
            if (lookahead.kind != kind)
                Log.Error(lookahead.location, $"Expected 'end {kind}' but got 'end {lookahead.text}'");
            NextToken();
            ExpectEol();
        }
    }

    private void ParseWhile(AstBlock block) {
        Token loc = Expect(TokenKind.While);
        AstExpression cond = ParseExpression();
        ExpectEol();
        AstWhile ret = new(loc.location, cond, block);
        block.Add(ret);
        ParseIndentedBlock(ret);
        CheckEnd(TokenKind.While);
    }

    private void ParseRepeat(AstBlock block) {
        Token loc = Expect(TokenKind.Repeat);
        ExpectEol();
        AstRepeat ret = new(loc.location, block);
        block.Add(ret);
        ParseIndentedBlock(ret);
        Expect(TokenKind.Until);
        AstExpression cond = ParseExpression();
        ExpectEol();
        ret.SetCond(cond);
    }

    private void parseAssign(AstBlock block) {
        AstExpression lhs = ParsePostfix();
        if (lookahead.kind == TokenKind.Eq) {
            Token op = NextToken();
            AstExpression rhs = ParseExpression();
            ExpectEol();
            block.Add(new AstAssign(op.location, lhs, rhs));
        } else if (lhs is AstFuncCall) {
            ExpectEol();
            block.Add(new AstFuncCallStatement(lhs.location, lhs));
        } else
            Log.Error(lhs.location, "Expression has no effect");
    }

    private void parsePrint(AstBlock block) {
        Token loc = Expect(TokenKind.Print);
        Expect(TokenKind.OpenB);
        List<AstExpression> args = [];
        do {
            args.Add(ParseExpression());
        } while (CanTake(TokenKind.Comma));
        Expect(TokenKind.CloseB);
        ExpectEol();

        block.Add(new AstPrint(loc.location, args));
    }

    private void ParseUnexpectedIndent(AstBlock block) {
        Expect(TokenKind.Indent);
        Log.Error(lookahead.location, "Unexpected indent");
        while(lookahead.kind != TokenKind.Dedent && lookahead.kind != TokenKind.EndOfFile)
            ParseStatement(block);
        Expect(TokenKind.Dedent);
    }

    private AstIfClause ParseIfClause(AstBlock block) {
        Token loc = NextToken();
        AstExpression cond = ParseExpression();
        ExpectEol();
        AstIfClause ret = new(loc.location, cond, block);
        ParseIndentedBlock(ret);
        return ret;
    }

    private AstIfClause ParseElseClause(AstBlock block) {
        Token loc = Expect(TokenKind.Else);
        ExpectEol();
        AstIfClause ret = new(loc.location, null, block);
        ParseIndentedBlock(ret);
        return ret;
    }

    private void ParseIf(AstBlock block) {
        List<AstIfClause> clauses = [];
        Location loc = lookahead.location;
        clauses.Add(ParseIfClause(block));
        while(lookahead.kind==TokenKind.Elsif)
            clauses.Add(ParseIfClause(block));
        if (lookahead.kind == TokenKind.Else)
            clauses.Add(ParseElseClause(block));
        block.Add(new AstIf(loc, clauses));
    }

    private void ParseReturn(AstBlock block) {
        Token loc = Expect(TokenKind.Return);
        if (lookahead.kind == TokenKind.EndOfLine) {
            NextToken();
            block.Add(new AstReturn(loc.location, null));
        } else {
            AstExpression expr = ParseExpression();
            ExpectEol();
            block.Add(new AstReturn(loc.location, expr));
        }
    }

    private void ParseStatement(AstBlock block) {
        try {
            switch(lookahead.kind) {
                case TokenKind.Val: ParseDeclaration(block); break;
                case TokenKind.Var: ParseDeclaration(block); break;
                case TokenKind.While: ParseWhile(block); break;
                case TokenKind.Repeat: ParseRepeat(block); break;
                case TokenKind.Fun: ParseFunction(block); break;
                case TokenKind.Indent: ParseUnexpectedIndent(block); break;
                case TokenKind.If: ParseIf(block); break;
                case TokenKind.Return: ParseReturn(block); break;
                case TokenKind.Print: parsePrint(block); break;
                case TokenKind.Class: ParseClass(block); break;
                case TokenKind.Identifier:
                case TokenKind.OpenB: parseAssign(block); break;
                default: throw new ParseError(lookahead.location, $"Got {lookahead.text} when expecting a statement");
            }
        } catch (ParseError e) {
            Log.Error(e.location, e.Message);
            SkipToEndOfLine();
        }
    }

    private void ParseStatementClass(AstBlock block) {
        try {
            switch(lookahead.kind) {
                case TokenKind.Val: ParseDeclaration(block); break;
                case TokenKind.Var: ParseDeclaration(block); break;
                case TokenKind.Indent: ParseUnexpectedIndent(block); break;
                case TokenKind.Fun: ParseFunction(block); break;
                case TokenKind.While: 
                case TokenKind.Repeat: 
                case TokenKind.If: 
                case TokenKind.Return: 
                case TokenKind.Print: 
                case TokenKind.Identifier:
                case TokenKind.OpenB: throw  new ParseError(lookahead.location, $"Statement {lookahead.text} not allowed inside class");
                default: throw new ParseError(lookahead.location, $"Got {lookahead.text} when expecting a statement");
            }
        } catch (ParseError e) {
            Log.Error(e.location, e.Message);
            SkipToEndOfLine();
        }
    }



    public void Parse(AstTop top) {
        while(lookahead.kind != TokenKind.EndOfFile)    
            ParseStatement(top);
    }
}