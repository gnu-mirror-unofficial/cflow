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
#include <stdio.h>
#include "cflow.h"
#include "parser.h"
#include "output.h"

void xref_output();
void print_refs(char*,Consptr);
void print_function(Symbol*);
void print_type(Symbol *);
void scan_tree(int, Symbol *);
void direct_tree(int, Symbol *);
void inverted_tree(int, Symbol *);
void tree_output();
void print_level(int);
void print_function_name(Symbol*,int);
void set_active(Symbol*);
void header(enum tree_type);
void begin();
void end();
void separator();

int level_mark_size=1000;
char *level_mark;
int out_line = 1;
FILE *outfile;

void
output()
{
    if (strcmp(outname, "-") == 0) {
	outfile = stdout;
    } else {
	outfile = fopen(outname, "w");
	if (!outfile)
	    error(SYSTEM_ERROR|FATAL(2), "cannot open file `%s'", outname);
    } 
	
    level_mark = emalloc(level_mark_size);
    if (print_option & PRINT_XREF) {
	xref_output();
    }
    if (print_option & PRINT_TREE) {
	tree_output();
    }
    fclose(outfile);
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
    Symbol **symbols, *main;
    int i, num;
    Ref *refptr;
    Cons *cons;

    /* Collect and sort symbols */
    num = collect_symbols(&symbols, is_fun);
    qsort(symbols, num, sizeof(*symbols), compare);
    /* Scan and mark the recursive ones */
    for (i = 0; i < num; i++) {
	if (symbols[i]->v.func.callee)
            scan_tree(0, symbols[i]);
    }

    /* Produce output */
    begin();
    
    header(DirectTree);
    main = lookup(start_name);
    if (main) {
	direct_tree(0, main);
	separator();
    } else {
	for (i = 0; i < num; i++) {
	    if (symbols[i]->v.func.callee == NULL)
		continue;
	    direct_tree(0, symbols[i]);
	    separator();
	}
    }
    
    header(ReverseTree);
    for (i = 0; i < num; i++) {
	inverted_tree(0, symbols[i]);
	separator();
    }

    end();
    
    free(symbols);
}

void
scan_tree(lev, sym)
    int lev;
    Symbol *sym;
{
    Consptr cons;

    if (sym->active) {
	sym->v.func.recursive = 1;
	return;
    }
    sym->active = 1;
    for (cons = sym->v.func.callee; cons; cons = CDR(cons)) {
	scan_tree(lev+1, (Symbol*)CAR(cons));
    }
    sym->active = 0;
}

void
direct_tree(lev, sym)
    int lev;
    Symbol *sym;
{
    Consptr cons;

    print_level(lev);
    print_function_name(sym, sym->v.func.callee != NULL);
    if (sym->active)
	return;
    set_active(sym);
    for (cons = sym->v.func.callee; cons; cons = CDR(cons)) {
	level_mark[lev+1] = CDR(cons) != NULL;
	direct_tree(lev+1, (Symbol*)CAR(cons));
    }
    clear_active(sym);
}

void
inverted_tree(lev, sym)
    int lev;
    Symbol *sym;
{
    Consptr cons;

    print_level(lev);
    print_function_name(sym, sym->v.func.caller != NULL);
    if (sym->active)
	return;
    set_active(sym);
    for (cons = sym->v.func.caller; cons; cons = CDR(cons)) {
	level_mark[lev+1] = CDR(cons) != NULL;
	inverted_tree(lev+1, (Symbol*)CAR(cons));
    }
    clear_active(sym);
}

void
clear_active(sym)
    Symbol *sym;
{
    sym->active = 0;
}


static void
newline()
{
    fprintf(outfile, "\n");
    out_line++;
}

void
begin()
{
}

void
end()
{
}

void
separator()
{
    newline();
}

void
print_level(lev)
    int lev;
{
    int i;
    
    if (print_levels)
	fprintf(outfile, "%4d ", lev);
    if (print_as_tree) {
	for (i = 0; i < lev; i++) {
	    if (level_mark[i])
		fprintf(outfile, "| ");
	    else
		fprintf(outfile, "  ");
	}
	fprintf(outfile, "+-");
    } else {
	for (i = 0; i < lev; i++)
	    fprintf(outfile, "%s", level_indent);
    }
}

void
print_function_name(sym, has_subtree)
    Symbol *sym;
    int has_subtree;
{
    fprintf(outfile, "%s()", sym->name);
    if (sym->v.func.type)
	fprintf(outfile, " <%s at %s:%d>",
	       sym->v.func.type,
	       sym->v.func.source,
	       sym->v.func.def_line);
    if (sym->active) {
	fprintf(outfile, "(recursive: see %d)", sym->active-1);
	newline();
	return;
    }
    if (sym->v.func.recursive)
	fprintf(outfile, " (R)");
    if (!print_as_tree && has_subtree)
	fprintf(outfile, ":");
    newline();
}

void
set_active(sym)
    Symbol *sym;
{
    sym->active = out_line;
}


void
header(tree)
    enum tree_type tree;
{
    newline();
    switch (tree) {
    case DirectTree:
	fprintf(outfile, "Direct Tree:");
	break;
    case ReverseTree:
	fprintf(outfile, "Reverse Tree:");
	break;
    }
    newline();
}

void
print_refs(name, cons)
    char *name;
    Consptr cons;
{
    Ref *refptr;
    
    for ( ; cons; cons = CDR(cons)) {
	refptr = (Ref*)CAR(cons);
	fprintf(outfile, "%s   %s:%d",
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
	fprintf(outfile, "%s * %s:%d %s",
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
    fprintf(outfile, "%s t %s:%d",
	   symp->name,
	   symp->v.type.source,
	   symp->v.type.def_line,
	   symp->v.func.type);
    newline();
}


