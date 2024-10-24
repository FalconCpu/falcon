
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EOL = 1,
     INDENT = 2,
     OUTDENT = 3,
     IDENTIFIER = 4,
     INTLIT = 5,
     REALLIT = 6,
     STRINGLIT = 7,
     CHARLIT = 8,
     PLUS = 9,
     MINUS = 10,
     STAR = 11,
     SLASH = 12,
     PERCENT = 13,
     AMP = 14,
     BAR = 15,
     CARAT = 16,
     EQ = 17,
     NEQ = 18,
     LT = 19,
     LTE = 20,
     GT = 21,
     GTE = 22,
     NOT = 23,
     AND = 24,
     OR = 25,
     DOT = 26,
     ARROW = 27,
     COMMA = 28,
     COLON = 29,
     OPENB = 30,
     OPENSQ = 31,
     OPENCL = 32,
     CLOSEB = 33,
     CLOSESQ = 34,
     CLOSECL = 35,
     VAL = 36,
     VAR = 37,
     IF = 38,
     ELSE = 39,
     END = 40,
     WHILE = 41,
     REPEAT = 42,
     UNTIL = 43,
     CLASS = 44,
     ENUM = 45,
     RETURN = 46,
     FUN = 47
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 82 "C:/Users/simon/falcon/compiler/src/parser.y"

    String string;
    AST    ast;
    Block  block;
    AST_list list;



/* Line 1676 of yacc.c  */
#line 110 "C:/Users/simon/falcon/compiler/src/build/parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE yylloc;

