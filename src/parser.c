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
    enum storage storage;
} Ident;

void func_def(Ident*);
void func_body();
void declaration(Ident*);
int dcl(Ident*);
int dirdcl(Ident*);
void process_dcl(Ident*);
void process_knr_dcl(Ident*);
void process_comma_dcl(Ident*);
void process_typedef(Ident*);
void process_struct(Ident*);

void call();
void reference();

#define MAXTOKENLEN 256

int level;
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
    case TYPE:
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
    case EXTERN: /* starage class specifiers are already taken care of */
    case STATIC:
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
    obstack_1grow(&text_stk, 0);
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
    Ident identifier;

    level = 0;
    clearstack();
    while (nexttoken()) {
	identifier.storage = ExternStorage;
	switch (tok.type) {
	case 0:
	    return 0;
	case TYPEDEF:
	    process_typedef(&identifier);
	    break;
	case STRUCT:
	    process_struct(&identifier);
	    break;
	case STATIC:
	    identifier.storage = StaticStorage;
	    /* fall through */
	default:
	    identifier.type_end = -1;
	    process_knr_dcl(&identifier);

	    switch (tok.type) {
	    default:
		if (verbose) 
		    file_error("expected ';'", 1);
		/* should putback() here */
		/* fall through */
	    case ';':
		declaration(&identifier);
		break;
	    case ',':
		declaration(&identifier);
		process_comma_dcl(&identifier);
		break;
	    case '=':
		declaration(&identifier);
		skip_to(';'); /* should handle ',' */
		break;
	    case LBRACE0:
	    case LBRACE:
		func_def(&identifier);
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
process_knr_dcl(ident)
    Ident *ident;
{
    ident->type_end = -1;
    process_dcl(ident);
    if (strict_ansi)
	return;
    switch (tok.type) {
    case IDENTIFIER:
    case TYPE:
	if (ident->parmcnt >= 0) {
	    /* maybe K&R function definition */
	    int i, parmcnt, sp, stop, new_tos;
	    Ident id;
	    
	    sp = mark();
	    parmcnt = 0;
	    
	    for (stop = 0; !stop && parmcnt < ident->parmcnt;
		 nexttoken()) {
		id.type_end = -1;
		switch (tok.type) {
		case '{':
		    putback();
		    stop = 1;
		    break;
		case TYPE:
		case IDENTIFIER:
		    new_tos = mark();
		    if (dcl(&id) == 0) {
 			parmcnt++;
			if (tok.type == ',') {
			    do {
				tos = id.type_end;
				curs = new_tos;
				dcl(&id);
			    } while (tok.type == ',');
			}
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
process_typedef(ident)
    Ident *ident;
{
    dcl(NULL);
}

void
process_struct(ident)
    Ident *ident;
{
}

void
process_comma_dcl(ident)
    Ident *ident;
{
    do {
	tos = ident->type_end;
	curs = 1;
	process_dcl(ident);
	declaration(ident);
    } while (tok.type == ',');
}
	

void
process_dcl(ident)
    Ident *ident;
{
    ident->parmcnt = -1;
    putback();
    dcl(ident);
    save_stack();
}

int
dcl(idptr)
    Ident *idptr;
{
    int type;
    
    while (nexttoken() != 0 && tok.type != '(') {
	if (tok.type == MODIFIER) {
	    if (idptr && idptr->type_end == -1)
		idptr->type_end = curs-1;
	}
	if (tok.type == IDENTIFIER) {
	    while (tok.type == IDENTIFIER)
		nexttoken();
	    type = tok.type;
	    putback();
	    if (type != MODIFIER) 
		break;
	}
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
func_def(ident)
    Ident *ident;
{
    declaration(ident);
}

void
func_body()
{
    char *name;
    Ident ident;
    
    level++;
    while (level) {
	clearstack();
	nexttoken();
	switch (tok.type) {
	case TYPE:
	    ident.storage = AutoStorage;
	    process_dcl(&ident);
	    declaration(&ident);
	    break;
	case IDENTIFIER:
	    nexttoken();
	    if (tok.type == '(')
		call();
	    else {
		reference();
		if (tok.type == MEMBER_OF) {
		    while (tok.type == MEMBER_OF)
			nexttoken();
		}
	    }
	    break;
	case '(':
	    /* typecast */
	    skip_to(')');
	    break;
	case LBRACE0:
	case '{':
	    level++;
	    break;
	case RBRACE0:
	    if (!ignore_indentation) {
		if (verbose && level != 1)
		    file_error("forced function body close", 0);
		for ( ; level; level--) {
		    delete_autos(level);
		}
		break;
	    }
	    /* else: fall thru */
	case '}':
	    delete_autos(level);
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
declaration(ident)
    Ident *ident;
{
    Symbol *sp;
    
    finish_save();
    sp = install(ident->name);
    sp->type = SymFunction;
    sp->v.func.argc = ident->parmcnt;
    sp->v.func.storage = ident->storage;
    sp->v.func.type = declstr;
    sp->v.func.source = filename;
    sp->v.func.def_line = ident->line;
    sp->v.func.level = level;
#ifdef DEBUG
    if (debug)
	printf("%s:%d: %s/%d defined to %s\n",
	       filename,
	       line_num,
	       ident->name, ident->parmcnt,
	       declstr);
#endif
}

Symbol *
get_symbol(name)
    char *name;
{
    Symbol *sp;

    if (sp = lookup(name)) {
	while (sp->type != SymFunction) 
	    sp = sp->next;
	if (sp)
	    return sp;
    }
    sp = install(name);
    sp->type = SymFunction;
    return sp;
}

void
call()
{
    Symbol *sp = get_symbol(tok.token);
    Ref *refptr;
    Cons *cons;

    if (sp->v.func.storage == AutoStorage)
	return;
    refptr = emalloc(sizeof(*refptr));
    cons = alloc_cons();
    refptr->source = filename;
    refptr->line = line_num;
    (Ref*)CAR(cons) = refptr;
    CDR(cons) = sp->v.func.ref_line;
    sp->v.func.ref_line = cons;
}

void
reference()
{
    Symbol *sp = get_symbol(tok.token);
    Ref *refptr;
    Cons *cons;

    if (sp->v.func.storage == AutoStorage)
	return;
    refptr = emalloc(sizeof(*refptr));
    cons = alloc_cons();
    refptr->source = filename;
    refptr->line = line_num;
    (Ref*)CAR(cons) = refptr;
    CDR(cons) = sp->v.func.ref_line;
    sp->v.func.ref_line = cons;
}

void
print_token(tokptr)
    TOKSTK *tokptr;
{
    switch (tokptr->type) {
    case IDENTIFIER:
    case TYPE:
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
