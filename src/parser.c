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
    int type_end;
    int parmcnt;
    int line;
} Ident;

void func_def();
void func_body();
void declaration();
int dcl(Ident*);
int dirdcl(Ident*);
void process_dcl();
void process_knr_dcl();
void process_comma_dcl();
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
    int line;
} TOKSTK;

TOKSTK tok;
TOKSTK *token_stack;
int tos;
int curs;
int token_stack_length = 64;
int token_stack_increase = 32;
static int need_space;

#define mark() tos
#define restore(sp) tos=sp

void
tokpush(type, line, token)
    int type;
    int line;
    char *token;
{
    token_stack[tos].type = type;
    token_stack[tos].token = token;
    token_stack[tos].line = line;
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
	tokpush(type, line_num, yylval.str);
    }
    tok = token_stack[curs];
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
    case MODIFIER:
	if (need_space) 
	    obstack_1grow(&text_stk, ' ');
	if (tokptr->token[0] == '*') 
	    need_space = 0;
	else
	    need_space = 1;
	obstack_grow(&text_stk, tokptr->token, strlen(tokptr->token));
	break;
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
    
    level = 0;
    clearstack();
    while (nexttoken()) {
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
	    identifier.type_end = -1;
	    process_knr_dcl();

	    switch (tok.type) {
	    default:
		if (verbose) 
		    file_error("expected ';'", 1);
		/* should putback() here */
		/* fall through */
	    case ';':
		declaration();
		break;
	    case ',':
		declaration();
		process_comma_dcl();
		break;
	    case '=':
		declaration();
		skip_to(';'); /* should handle ',' */
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
	    break;
	}
	clearstack();
    }
    /*NOTREACHED*/
}

void
process_knr_dcl()
{
    identifier.type_end = -1;
    process_dcl();
    if (strict_ansi)
	return;
    switch (tok.type) {
    case IDENTIFIER:
    case WORD:
	if (identifier.parmcnt >= 0) {
	    /* maybe K&R function definition */
	    int i, parmcnt, sp, stop;
		    
	    sp = mark();
	    parmcnt = 0;
	    for (stop = 0; !stop && parmcnt < identifier.parmcnt;
		 nexttoken()) {
		switch (tok.type) {
		case '{':
		    putback();
		    stop = 1;
		    break;
		case WORD:
		case IDENTIFIER:
		    if (dcl(NULL) == 0) {
			parmcnt++;
			break;
		    }
		    /* else fall through */
		default:
		    restore(sp);
		    return;
		}
	    }
	}
    }
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
process_comma_dcl()
{
    do {
	tos = identifier.type_end;
	curs = 1;
	process_dcl();
	declaration();
    } while (tok.type == ',');
}
	

void
process_dcl()
{
    identifier.parmcnt = -1;
    putback();
    dcl(&identifier);
    save_stack();
}

int
dcl(idptr)
    Ident *idptr;
{
    while (nexttoken() != 0 && tok.type != IDENTIFIER && tok.type != '(') {
	if (tok.type == MODIFIER) {
	    if (idptr && idptr->type_end == -1)
		idptr->type_end = curs-1;
	}
    }
    if (tok.type == IDENTIFIER) {
	while (tok.type == IDENTIFIER)
	    nexttoken();
/*	if (tok.type != '(')*/
	    putback();
    }
    if (idptr && idptr->type_end == -1)
	idptr->type_end = curs-1;
    return dirdcl(idptr);
}

int
dirdcl(idptr)
    Ident *idptr;
{
    int *parm_ptr = NULL;
    
    if (tok.type == '(') {
	dcl(idptr);
	if (tok.type != ')' && verbose) {
	    file_error("expected ')'", 1);
	    return 1;
	}
    } else if (tok.type == IDENTIFIER) {
	if (idptr) {
	    idptr->name = tok.token;
	    idptr->line = tok.line;
	    parm_ptr = &idptr->parmcnt;
	}
    }
    while (nexttoken() == '[' || tok.type == '(') {
	if (tok.type == '[') 
	    skip_to(']');
	else {
	    maybe_parm_list(parm_ptr);
	    if (tok.type != ')' && verbose) {
		file_error("expected ')'", 1);
		return 1;
	    }
	}
    }
    return 0;
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
    Symbol *sp;
    
    finish_save();
    sp = install(identifier.name);
    if (identifier.parmcnt >= 0) 
	sp->type = SymFunction;
    else
	sp->type = SymVariable;
    sp->v.func.argc = identifier.parmcnt;
    sp->v.func.type = declstr;
    sp->v.func.source = filename;
    sp->v.func.def_line = identifier.line;
#ifdef DEBUG
    if (debug)
	printf("%s:%d: %s/%d defined to %s\n",
	       filename,
	       line_num,
	       identifier.name, identifier.parmcnt,
	       declstr);
#endif
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

void
print_token(tokptr)
    TOKSTK *tokptr;
{
    switch (tokptr->type) {
    case IDENTIFIER:
    case WORD:
    case MODIFIER:
	fprintf(stderr, "`%s'", tokptr->token);
	break;
    case LBRACE0:
    case LBRACE:
	fprintf(stderr, "`{'");
	break;
    case RBRACE0:
    case RBRACE:
	fprintf(stderr, "`}'");
	break;
    case EXTERN:
	fprintf(stderr, "`extern'");
	break;
    case STATIC:
	fprintf(stderr, "`static'");
	break;
    case TYPEDEF:
	fprintf(stderr, "`typedef'");
	break;
    case STRUCT:
	fprintf(stderr, "`struct'");
	break;
    case UNION:
	fprintf(stderr, "`union'");
	break;
    case ENUM:
	fprintf(stderr, "`enum'");
	break;
    case OP:
	fprintf(stderr, "OP"); /* ouch!!! */
	break;
    default:
	fprintf(stderr, "`%c'", tokptr->type);
    }
}

file_error(msg, near)
    char *msg;
    int near;
{
    fprintf(stderr, "%s:%d: %s", filename, tok.line, msg);
    if (near) {
	fprintf(stderr, " near ");
	print_token(&tok);
    }
    fprintf(stderr, "\n");
}
