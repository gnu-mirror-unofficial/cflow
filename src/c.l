/* This file is part of GNU cflow
   Copyright (C) 1997,2005 Sergey Poznyakoff

   GNU cflow is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GNU cflow is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU cflow; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

%x comment
%x string
%x stringwait
%x longline
%{
#include <cflow.h>
#include <ctype.h>
#include <parser.h>
     
struct obstack string_stk;

int line_num;
char *filename;
YYSTYPE yylval;

int ident();

#define lex_error(msg) error_at_line(0, 0, filename, line_num, "%s", msg)

%}
FILENAME [^\n*?]*
ONUMBER (0[0-7]*)
HNUMBER (0[xX][0-9a-fA-F]*)
DNUMBER ([1-9][0-9]*)
DIGITS  [0-9][0-9]*
IDENT [a-zA-Z_][a-zA-Z0-9_]*
WS    [ \t\f\r]* 
%%
     /* comments */
"/*"			BEGIN(comment);
<comment>[^*\n]*	;
<comment>[^*\n]*\n	++line_num;
<comment>"*"+[^*/\n]*	;
<comment>"*"+[^*/\n]*\n	++line_num;
<comment>"*"+"/"	BEGIN(INITIAL); 
     /* ^{WS}#{WS}line */
     /* skip any preproc */
^{WS}#.*\\\n             { BEGIN(longline); ++line_num; }
{WS}#.*\n                ++line_num;
<longline>.*\\\n         ++line_num; 
<longline>.*\n           { BEGIN(INITIAL); ++line_num; }
     /* keywords */
auto                 /* ignored */;
extern               return EXTERN; 
static               return STATIC;
typedef              return TYPEDEF;
struct               {yylval.str = "struct"; return STRUCT;}
union                {yylval.str = "union"; return STRUCT;}
enum                 {yylval.str = "enum"; return STRUCT;}
\*                   { yylval.str = "*";
		       return MODIFIER;
		   }
     /* Operators
      *
      */
"->"   |
"."                  return MEMBER_OF;
"*="   |                 
"/="   |                 
"%="   |                 
"+="   |                 
"-="   |                 
"<<="  |                 
">>="  |                 
"&="   |                 
"|="   |                 
"^="   |                 
"||"   |                 
"&&"   |                 
"=="   |                 
"!="   |                 
">="   |                 
">"    |                 
"<="   |                 
"<"    |                 
"<<"   |                 
">>"   |                 
"++"   |                 
"--"                     return OP;
'.'  |
'\\.' |
'\\[0-7]{2,3}' |
'\\[xX][0-9a-fA-F]{1,2}'  return STRING;
     /* Identifiers and constants
      *
      */
"..." |
{IDENT}			return ident();
{ONUMBER}               { obstack_grow(&string_stk, yytext, yyleng+1);
			  yylval.str = obstack_finish(&string_stk);
			  return WORD;
		      }
[+\-]?{DNUMBER}[^eE.]   { yyless(yyleng-1);
			  obstack_grow(&string_stk, yytext, yyleng+1);
			  yylval.str = obstack_finish(&string_stk);
			  return WORD;}
{HNUMBER} |               
[+\-]?{DIGITS}?\.{DIGITS}([eE][+\-]?{DIGITS})? |
[+\-]?{DIGITS}\.([eE][+\-]?{DIGITS})? {
    obstack_grow(&string_stk, yytext, yyleng+1);
    yylval.str = obstack_finish(&string_stk);
    return WORD;
}

     /* strings
      * State map:
      *
      * "<string>blahblahblah"<stringwait>
      * <stringwait>"<string>blahblahblah"<stringwait>
      * <stringwait>.<INITIAL>
      */
\"    BEGIN(string);
<string>[^\\"\n]*       ;
<string>\n              lex_error("unterminated string?"); 
<string>\\.             ;
<string>\\\n            ;
<string>\"              BEGIN(stringwait);
<stringwait>{WS}        ;
<stringwait>\n          ++line_num; 
<stringwait>\"          BEGIN(string);
<stringwait>.           {
     BEGIN(INITIAL);
     yyless(0); /* put the symbol back */
     return STRING;
}
\n                      ++line_num;
{WS}                    ;
       /*\f                      ;*/
^\{                     return LBRACE0;
^\}                     return RBRACE0;
.                       return yytext[0];
%%

static char *keywords[] = {
    "break",
    "case",
    "continue",
    "default",
    "do",
    "else",
    "for",
    "goto",
    "if",
    "return",
    "sizeof",
    "switch",
    "while"
};

static char *types[] = {
    "char",
    "const",
    "double",
    "float",
    "int",
    "long",
    "register",
    "short",
    "signed",
    "unsigned",
    "void",
    "volatile",
};

void
init_lex(int debug_level)
{
     int i;
     Symbol *sp;
    
     yy_flex_debug = debug_level;
     
     obstack_init(&string_stk);

     for (i = 0; i < NUMITEMS(keywords); i++) {
	  sp = install(keywords[i]);
	  sp->type = SymToken;
	  sp->v.type.token_type = WORD;
     }
     
     for (i = 0; i < NUMITEMS(types); i++) {
	  sp = install(types[i]);
	  sp->type = SymToken;
	  sp->v.type.token_type = TYPE;
	  sp->v.type.source = NULL;
	  sp->v.type.def_line = -1;
	  sp->v.type.ref_line = NULL;
     }	
     sp = install("...");
     sp->type = SymToken;
     sp->v.type.token_type = IDENTIFIER;
     sp->v.type.source = NULL;
     sp->v.type.def_line = -1;
     sp->v.type.ref_line = NULL;
}

int
ident()
{
     Symbol *sp;

     sp = lookup(yytext);
     if (sp && sp->type == SymToken) {
	  yylval.str = sp->name;
	  return sp->v.type.token_type;
     }
     obstack_grow(&string_stk, yytext, yyleng);
     obstack_1grow(&string_stk, 0);
     yylval.str = obstack_finish(&string_stk);
     return IDENTIFIER;
}

int
yywrap()
{
     delete_statics();
     return 1;
}

int
source(char *name)
{
     FILE *fp;
     
     fp = fopen(name, "r");
     if (!fp) {
	  error(0, errno, "cannot open `%s'", name);
	  return 1;
     }
     obstack_grow(&string_stk, name, strlen(name)+1);
     filename = obstack_finish(&string_stk);
     line_num = 1;
     yyrestart(fp);
     return 0;
}

static int
getnum(unsigned  base, int  count)
{
     int  c, n;
     unsigned i;

     for (n = 0; count; count--) {
	  if (isdigit(c = input()))
	       i = c - '0';
	  else
	       i = toupper(c) - 'A' + 10;
	  if (i > base) {
	       unput(c);
	       break;
	  }
	  n = n * base + i;
     }
     return n;
}

int
backslash()
{                                  
     int c;
     
     switch (c = input()) {                        
     case 'a':   return '\a';                  
     case 'b':   return '\b';                  
     case 'f':   return '\f';                  
     case 'n':   return '\n';                  
     case 'r':   return '\r';                  
     case 't':   return '\t';                  
     case 'x':   return getnum(16,2);          
     case '0':   return getnum(8,3);           
     }                                             
     return c;                                         
}                                                     


    
