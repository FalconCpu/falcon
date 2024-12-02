
using System.Xml.Serialization;

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

    private Token Expect(TokenKind kind1, TokenKind kind2) {
        if (lookahead.kind == kind1 || lookahead.kind == kind2)
            return NextToken();
        throw new ParseError(lookahead.location, $"Expected {kind1} or {kind2} but got {lookahead.text}");
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

    private AstCharLit ParseCharLit() {
        Token tok = Expect(TokenKind.CharLit);
        return new(lookahead.location, tok.text[1]);
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
        if (CanTake(TokenKind.Colon)) {
            AstType type = ParseType();
            Expect(TokenKind.CloseB);
            return new AstTypecast(lookahead.location, exp, type);
        } else {
        Expect(TokenKind.CloseB);
        return exp;
        }
    }

    private AstNew ParseNew() {
        Expect(TokenKind.New);
        AstType type = ParseType();
        bool hasInitializer;
        List<AstExpression> args = [];

        if (CanTake(TokenKind.OpenB)) {
            hasInitializer = false;
            args = ParseExpressionList();
            Expect(TokenKind.CloseB);
        } else if (CanTake(TokenKind.OpenCl)) {
            hasInitializer = true;
            args = ParseExpressionList();
            Expect(TokenKind.CloseCl);
        } else {
            throw new ParseError(lookahead.location, $"Expected '(' or '{{' but got '{lookahead.text}'");
        }
        return new AstNew(lookahead.location, type, args, hasInitializer);
    }

    private AstExpression ParsePrimary() {
        switch(lookahead.kind) {
            case TokenKind.Identifier:  return ParseIdentifier();
            case TokenKind.Integer:     return ParseIntLit();
            case TokenKind.String:      return ParseStringLit();
            case TokenKind.RealLit:     return ParseRealLit();
            case TokenKind.CharLit:     return ParseCharLit();
            case TokenKind.New:         return ParseNew();
            case TokenKind.OpenB:       return ParseBracket();
            default:                    throw new ParseError(lookahead.location, $"Got '{lookahead.text}' when expecting primary exppression");;
        }
    }   

    private AstFuncCall ParseFuncCall(AstExpression left) {
        Token loc = Expect(TokenKind.OpenB);
        List<AstExpression> args = ParseExpressionList();
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
        if (lookahead.kind == TokenKind.Minus || lookahead.kind == TokenKind.Tilde || lookahead.kind == TokenKind.Not) {
            Token op = NextToken();
            AstExpression right = ParsePrefix();
            return new AstUnary(op.location, op.kind, right);
        } else if (lookahead.kind==TokenKind.Rc) {
            Token op = NextToken();
            AstExpression right = ParsePrefix();
            return new AstRc(op.location, right);
        }
            return ParsePostfix();
    }

    private AstExpression ParseMul() {
        AstExpression ret = ParsePrefix();
        while (lookahead.kind == TokenKind.Star || lookahead.kind == TokenKind.Slash || 
               lookahead.kind == TokenKind.Percent || lookahead.kind == TokenKind.Ampersand||
               lookahead.kind == TokenKind.Lsl || lookahead.kind == TokenKind.Lsr ||
               lookahead.kind == TokenKind.Asr ) {
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

    private List<AstExpression> ParseExpressionList() {
        List<AstExpression> ret = [];
        if (lookahead.kind==TokenKind.CloseB || lookahead.kind==TokenKind.CloseCl)
            return ret;
        
        ret.Add(ParseExpression());
        while (CanTake(TokenKind.Comma))
            ret.Add(ParseExpression());
        return ret;
    }

    private AstExpression? ParseOptExpression() {
        if (CanTake(TokenKind.Eq))
            return ParseExpression();
        else
            return null;
    }

    private List<AstType> ParseTypeArgs() {
        List<AstType> ret = [];
        if (lookahead.kind!=TokenKind.Lt)   
            return ret;
        Expect(TokenKind.Lt);

        if (lookahead.kind!=TokenKind.Gt)
            do {
                ret.Add(ParseType());
            } while (CanTake(TokenKind.Comma));
    
        Expect(TokenKind.Gt);
    
        return ret;
    }

    private AstTypeIdentifier ParseTypeIdentifier() {
        Token loc = Expect(TokenKind.Identifier);
        List<AstType> typeArgs = ParseTypeArgs();
        return new AstTypeIdentifier(loc.location, loc.text, typeArgs);
    }

    private AstTypeArray ParseTypeArray() {
        Token loc = Expect(TokenKind.Array);
        AstType? elementType = null;
        if (CanTake(TokenKind.Lt)) {
            elementType = ParseType();
            Expect(TokenKind.Gt);
        }
        return new AstTypeArray(loc.location, elementType);
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
        bool isVariadic = CanTake(TokenKind.DotDotDot);
        return new AstParameter(name.location, TokenKind.Val, name, type, isVariadic);
    }

    private List<AstParameter> ParseParameterList() {
        List<AstParameter> parameters = [];
        Expect(TokenKind.OpenB);
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
        return new AstParameter(name.location, kind, name, type, false);
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

    private List<AstIdentifier> ParseTypeParameterList() {
        List<AstIdentifier> parameters = [];
        Expect(TokenKind.Lt);
        if (lookahead.kind != TokenKind.Gt)
            do {
                parameters.Add(ParseIdentifier());
             } while (CanTake(TokenKind.Comma));
        Expect(TokenKind.Gt);
        return parameters;
    }

    private void ParseFunction(AstBlock block) {
        Token loc = Expect(TokenKind.Fun);
        Token name = Expect(TokenKind.Identifier, TokenKind.Delete);
        List<AstParameter> parameters = ParseParameterList();
        AstType? astReturnType = ParseOptReturnType();
        ExpectEol();
        AstFunction ret = new(loc.location, name.text, parameters, astReturnType, block);
        block.Add(ret);
        ParseIndentedBlock(ret);
        CheckEnd(TokenKind.Fun);
    }

    private void ParseClass(AstBlock block) {
        Token loc = Expect(TokenKind.Class);
        AstIdentifier name = ParseIdentifier();
        List<AstIdentifier> typeParameters = lookahead.kind==TokenKind.Lt ? ParseTypeParameterList() : [];
        List<AstParameter> parameters = lookahead.kind==TokenKind.OpenB ? ParseClassParameterList() : [];
        ExpectEol();
        AstClass ret = new(loc.location, name.name, typeParameters, parameters, block);
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

    private void ParseConst(AstBlock block) {
        Expect(TokenKind.Const);
        AstIdentifier name = ParseIdentifier();
        AstType? astType = ParseOptType();
        Expect(TokenKind.Eq);
        AstExpression expr = ParseExpression();
        ExpectEol();
        block.Add(new AstConst(name.location, name, astType, expr));
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

    private void ParseFor(AstBlock block) {
        Token loc = Expect(TokenKind.For);
        AstIdentifier name = ParseIdentifier();
        AstBlock ret;
        if (CanTake(TokenKind.Eq)) {
            AstExpression start = ParseExpression();
            Expect(TokenKind.To);
            TokenKind op;
            if (lookahead.kind==TokenKind.Gt || lookahead.kind==TokenKind.Lt || lookahead.kind==TokenKind.Gte || lookahead.kind==TokenKind.Lte)
                op = NextToken().kind;
            else
                op = TokenKind.Lte;
            AstExpression end = ParseExpression();
            ExpectEol();
            ret = new AstFor(loc.location, name, start, end, op, block);
        } else if (CanTake(TokenKind.In)) {
            AstExpression list = ParseExpression();
            ExpectEol();
            ret = new AstForIn(loc.location, name, list, block);
        } else {
            Log.Error(lookahead.location, "Expected 'for' or 'for in'");
            return;
        }
        block.Add(ret);
        ParseIndentedBlock(ret);
        CheckEnd(TokenKind.For);
    }
    
    private void parseAssign(AstBlock block) {
        AstExpression lhs = ParsePostfix();
        if (lookahead.kind == TokenKind.Eq || lookahead.kind == TokenKind.PlusEq || lookahead.kind == TokenKind.MinusEq || lookahead.kind == TokenKind.StarEq || lookahead.kind == TokenKind.SlashEq) {
            Token op = NextToken();
            AstExpression rhs = ParseExpression();
            ExpectEol();
            block.Add(new AstAssign(op.location, op.kind, lhs, rhs));
        } else if (lhs is AstFuncCall) {
            ExpectEol();
            block.Add(new AstFuncCallStatement(lhs.location, lhs));
        } else {
            ExpectEol();
            Log.Error(lhs.location, "Expression has no effect");
        }
    }

    private void parsePrint(AstBlock block) {
        Token loc = Expect(TokenKind.Print);
        Expect(TokenKind.OpenB);
        List<AstExpression> args = ParseExpressionList();
        Expect(TokenKind.CloseB);
        ExpectEol();

        block.Add(new AstPrint(loc.location, args));
    }

    private void ParseDelete(AstBlock block) {
        Token loc = Expect(TokenKind.Delete);
        AstExpression expr = ParseExpression();
        ExpectEol();
        block.Add(new AstDelete(loc.location, expr));
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

    private AstWhenClause ParseWhenClause(AstBlock block) {
        List<AstExpression> cond = [];
        if (lookahead.kind == TokenKind.Else) {
            NextToken();
        } else {
            do {
                cond.Add(ParseExpression());
            } while(CanTake(TokenKind.Comma));
        }
        Token loc = Expect(TokenKind.Arrow);
        AstWhenClause ret = new(loc.location, cond, block);
        if (lookahead.kind == TokenKind.EndOfLine) {
            ExpectEol();
            ParseIndentedBlock(ret);
        } else {
            ParseStatement(ret);
        }
        return ret;
    }

    private void ParseWhen(AstBlock block) {
        Token loc = Expect(TokenKind.When);
        AstExpression expr = ParseExpression();
        ExpectEol();
        Expect(TokenKind.Indent);
        List<AstWhenClause> clauses = [];
        while(lookahead.kind!=TokenKind.Dedent && lookahead.kind!=TokenKind.EndOfFile)
            clauses.Add( ParseWhenClause(block) );
        Expect(TokenKind.Dedent);
        CheckEnd(TokenKind.When);
        block.Add(new AstWhen(loc.location, expr, clauses));
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
                case TokenKind.For: ParseFor(block); break;
                case TokenKind.Print: parsePrint(block); break;
                case TokenKind.Class: ParseClass(block); break;
                case TokenKind.Const: ParseConst(block); break;
                case TokenKind.Delete: ParseDelete(block); break;
                case TokenKind.When: ParseWhen(block); break;
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