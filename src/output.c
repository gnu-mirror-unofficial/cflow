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
#include "output.h"

void xref_output();
void print_refs(char*,Consptr);
void print_function(Symbol*);
void print_type(Symbol *);
void direct_tree(int, Symbol *);
void reverse_tree(int, Symbol *);
void tree_output();

void (*print_level_fun[])(int) = {
    text_print_level,
    html_print_level
};
void (*print_function_name_fun[])(Symbol*) = {
    text_print_function_name,
    html_print_function_name
};
void (*set_active_fun[])(Symbol*) = {
    text_set_active,
    html_set_active
};
void (*header_fun[])(char*) = {
    text_header,
    html_header,
};


#define print_level print_level_fun[output_mode]
#define print_function_name print_function_name_fun[output_mode]
#define set_active set_active_fun[output_mode]
#define header header_fun[output_mode]

void
output()
{
    if (cross_ref) {
	xref_output();
    } else {
	tree_output();
    }
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
tree_output()
{
    Symbol **symbols, *symp;
    int i, num;
    Ref *refptr;
    Cons *cons;
    
    num = collect_symbols(&symbols, is_fun);
    qsort(symbols, num, sizeof(*symbols), compare);

    header("Direct tree");
    for (i = 0; i < num; i++) {
	if (symbols[i]->v.func.callee == NULL)
	    continue;
	direct_tree(0, symbols[i]);
	newline();
    }

    header("Reverse tree");
    for (i = 0; i < num; i++) {
	reverse_tree(0, symbols[i]);
	newline();
    }
    
    free(symbols);
}

void
direct_tree(lev, sym)
    int lev;
    Symbol *sym;
{
    Consptr cons;

    print_level(lev);
    print_function_name(sym);
    set_active(sym);
    for (cons = sym->v.func.callee; cons; cons = CDR(cons)) 
	direct_tree(lev+1, (Symbol*)CAR(cons));
    clear_active(sym);
}

void
reverse_tree(lev, sym)
    int lev;
    Symbol *sym;
{
    Consptr cons;

    print_level(lev);
    print_function_name(sym);
    set_active(sym);
    for (cons = sym->v.func.caller; cons; cons = CDR(cons)) 
	reverse_tree(lev+1, (Symbol*)CAR(cons));
    clear_active(sym);
}

void
clear_active(sym)
    Symbol *sym;
{
    sym->active = 0;
}
