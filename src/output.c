/*  $Id$
 *  cflow:
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
#include <stdlib.h>
#include "cflow.h"
#include "parser.h"

void xref_output();
void print_refs(char*,Consptr);
void print_function(Symbol*);
void print_type(Symbol *);
void direct_tree(int, Symbol *);
void direct_tree_output();

static int line;

void
output()
{
    line = 1;
    
    if (cross_ref) {
	xref_output();
    } else {
	direct_tree_output();
    }
}

void
newline()
{
    printf("\n");
    line++;
}

int
compare(a, b)
    Symbol **a, **b;
{
    return strcmp((*a)->name, (*b)->name);
}

int
is_var(symp)
    Symbol *symp;
{
    if (record_typedefs &&
	symp->type == SymToken &&
	symp->v.type.token_type == TYPE &&
	symp->v.type.source)
	return 1;
    return symp->type == SymFunction &&
	(symp->v.func.storage == ExternStorage ||
	 symp->v.func.storage == StaticStorage);
}

int
is_fun(symp)
    Symbol *symp;
{
    return symp->type == SymFunction && symp->v.func.argc >= 0;
}


void
xref_output()
{
    Symbol **symbols, *symp;
    int i, num;
    Ref *refptr;
    Cons *cons;
    
    num = collect_symbols(&symbols, is_var);
    qsort(symbols, num, sizeof(*symbols), compare);

    /* produce xref output */
    for (i = 0; i < num; i++) {
	symp = symbols[i];
	switch (symp->type) {
	case SymFunction:
	    print_function(symp);
	    break;
	case SymToken:
	    print_type(symp);
	    break;
	}
    }
    free(symbols);
}

void
direct_tree_output()
{
    Symbol **symbols, *symp;
    int i, num;
    Ref *refptr;
    Cons *cons;
    
    num = collect_symbols(&symbols, is_fun);
    qsort(symbols, num, sizeof(*symbols), compare);

    for (i = 0; i < num; i++) {
	direct_tree(0, symbols[i]);
	newline();
    }
    free(symbols);
}

int print_tree=1;

void
direct_tree(lev, sym)
    int lev;
    Symbol *sym;
{
    int i;
    Consptr cons;

    if (print_level)
	printf("%4d ", lev);
    if (print_tree) {
	if (lev) {
	    printf("  ");
	    for (i = 1; i < lev; i++)
		printf("| ");
	}
	printf("+-");
    } else {
	for (i = 0; i < lev; i++)
	    printf("%s", level_indent);
    }
    printf("%s()", sym->name);
    if (sym->v.func.type)
	printf(" <%s at %s:%d>",
	       sym->v.func.type,
	       sym->v.func.source,
	       sym->v.func.def_line);
    if (sym->active) {
	printf("(recursive: see %d)", sym->active-1);
	newline();
	return;
    } else
	printf(":");
    newline();
    sym->active = line;
    for (cons = sym->v.func.callee; cons; cons = CDR(cons)) 
	direct_tree(lev+1, (Symbol*)CAR(cons));
    sym->active = 0;
}

void
print_refs(name, cons)
    char *name;
    Consptr cons;
{
    Ref *refptr;
    
    for ( ; cons; cons = CDR(cons)) {
	refptr = (Ref*)CAR(cons);
	printf("%s   %s:%d",
	       name,
	       refptr->source,
	       refptr->line);
	newline();
    }
}


void
print_function(symp)
    Symbol *symp;
{
    if (symp->v.func.source) {
	printf("%s * %s:%d %s",
	       symp->name,
	       symp->v.func.source,
	       symp->v.func.def_line,
	       symp->v.func.type);
	newline();
    }
    print_refs(symp->name, symp->v.func.ref_line);
}


void
print_type(symp)
    Symbol *symp;
{
    printf("%s t %s:%d",
	   symp->name,
	   symp->v.type.source,
	   symp->v.type.def_line,
	   symp->v.func.type);
    newline();
}

