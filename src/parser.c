/* $Id$ */
/*  cflow:
 *  Copyright (C) 1997 Gray
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include "cflow.h"
#include "parser.h"
#include "obstack1.h"

typedef struct {
    char *name;
    int parmcnt;
} Ident;

void func_def();
void func_body();
void declaration();
void dcl(Ident*);
void dirdcl(Ident*);
void process_dcl();
void process_typedef();
void process_struct();

void call(char*);
void reference(char*);

#define MAXTOKENLEN 256

int level;
Ident identifier;
char *declstr;
struct obstack text_stk;

typedef struct {
    int type;
    char *token;
} TOKSTK;

TOKSTK tok;
TOKSTK *token_stack;
int tos;
int curs;
int token_stack_length = 64;
int token_stack_increase = 32;
static int need_space;

void
tokpush(type, token)
    int type;
    char *token;
{
    token_stack[tos].type = type;
    token_stack[tos].token = token;
    if (++tos == token_stack_length) {
	token_stack_length += token_stack_increase;
	token_stack = realloc(token_stack,
			      token_stack_length*sizeof(*token_stack));
	if (!token_stack)
	    error(FATAL(3), "Out of token pushdown");
    }
}

void
clearstack()
{
    tos = 0;
    curs = 0;
}

int
nexttoken()
{
    int type;
    
    if (curs == tos) {
	type = yylex();
	tokpush(type, yylval.str);
    }
    tok.type = token_stack[curs].type;
    tok.token = token_stack[curs].token;
    curs++;
    return tok.type;
}

int
putback()
{
    if (curs == 0)
	error(FATAL(10), "can't putback");
    curs--;
    if (curs > 0) {
	tok.type = token_stack[curs-1].type;
	tok.token = token_stack[curs-1].token;
    } else
	tok.type = 0;
    return tok.type;
}

void
init_parse()
{
    obstack_init(&text_stk);
    token_stack = emalloc(token_stack_length*sizeof(*token_stack));
    clearstack();
}

void
save_token(tokptr)
    TOKSTK *tokptr;
{
    switch (tokptr->type) {
    case IDENTIFIER:
    case WORD:
	if (need_space) 
	    obstack_1grow(&text_stk, ' ');
	obstack_grow(&text_stk, tokptr->token, strlen(tokptr->token));
	need_space = 1;
	break;
    case '*':
    case '(':
	if (need_space) 
	    obstack_1grow(&text_stk, ' ');
	/*fall through */
    default:
	obstack_1grow(&text_stk, tokptr->type);
	need_space = 0;
    }
}

void
save_stack()
{
    int i;

    for (i = 0; i < curs-1; i++) 
	save_token(token_stack+i);
}

void
finish_save()
{
    declstr = obstack_finish(&text_stk);
}

void
skip_to(c)
    int c;
{
    while (nexttoken()) {
	if (tok.type == c)
	    break;
    }
}
	
yyparse()
{
    int decl;
    
    level = 0;
    clearstack();
    while (nexttoken()) {
	decl = 0;
	switch (tok.type) {
	case 0:
	    return 0;
	case TYPEDEF:
	    process_typedef();
	    break;
	case STRUCT:
	    process_struct();
	    break;
	default:
	    decl = 1;
	    process_dcl();
	}
	finish_save();
	clearstack();
	
	switch (tok.type) {
	default:
	    if (decl && identifier.parmcnt >= 0) {
		/* maybe K&R function definition */
#if 0
		int i;
		
		mark();
		for (i = 0; i < ident.parmcnt; i++) {
		    parm_dcl();
		}
#endif		
	    }
	    if (verbose)
		file_error("expected ';'", 1);
	case ';':
	    declaration();
	    break;
	case '=':
	    declaration();
	    skip_to(';');
	    break;
	case LBRACE0:
	case LBRACE:
	    func_def();
	    func_body();
	    break;
	case 0:
	    if (verbose)
		file_error("unexpected eof in declaration", 0);
	    return 1;
	}
	clearstack();
    }
    /*NOTREACHED*/
}

void
process_typedef()
{
    dcl(NULL);
}

void
process_struct()
{
}

void
process_dcl()
{
    identifier.parmcnt = -1;
    putback();
    dcl(&identifier);
    save_stack();
}

void
dcl(idptr)
    Ident *idptr;
{
    while (nexttoken() != 0 && tok.type != IDENTIFIER && tok.type != '(')
	;
    if (tok.type == IDENTIFIER) {
	while (tok.type == IDENTIFIER)
	    nexttoken();
	putback();
    }
    dirdcl(idptr);
}

void
dirdcl(idptr)
    Ident *idptr;
{
    int *parm_ptr = NULL;
    
    if (tok.type == '(') {
	dcl(idptr);
	if (tok.type != ')' && verbose)
	    file_error("expected ')'", 1);
    } else if (tok.type == IDENTIFIER) {
	if (idptr) {
	    idptr->name = tok.token;
	    parm_ptr = &idptr->parmcnt;
	}
    }
    while (nexttoken() == '[' || tok.type == '(') {
	if (tok.type == '[') 
	    skip_to(']');
	else {
	    maybe_parm_list(parm_ptr);
	    if (tok.type != ')' && verbose)
		file_error("expected ')'", 1);
	}
    }
}

int
maybe_parm_list(parm_cnt_return)
    int *parm_cnt_return;
{
    int parmcnt=0;
    while (nexttoken()) {
	switch (tok.type) {
	case ')':
	    if (parm_cnt_return)
		*parm_cnt_return = parmcnt+1;
	    return;
	case ',':
	    parmcnt++;
	    break;
	default:
	    putback();
	    dcl(NULL);
	    putback();
	}
    }
    /*NOTREACHED*/
}

void
func_def()
{
    declaration();
}

void
func_body()
{
    int type;
    char *name;
    
    level++;
    while (level) {
	type = yylex();
	switch (type) {
	case IDENTIFIER:
	    name = yylval.str;
	    type = yylex();
	    if (type == '(')
		call(name);
	    else
		reference(name);
	    break;
	case LBRACE0:
	case '{':
	    level++;
	    break;
	case RBRACE0:
	    if (!ignore_indentation) {
		if (verbose && level != 1)
		    file_error("forced function body close", 0);
		level = 0;
		break;
	    }
	    /* else: fall thru */
	case '}':
	    level--;
	    break;
	case 0:
	    if (verbose)
		file_error("unexpected eof in function body", 0);
	    return;
	}
    }
}

void
declaration()
{
    printf("%s:%d: %s/%d defined to %s\n",
	   filename,
	   line_num,
	   identifier.name, identifier.parmcnt,
	   declstr);
}

void
call(name)
    char *name;
{
}

void
reference(name)
    char *name;
{
}
