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

static void
newline()
{
    fprintf(outfile, "\n");
    out_line++;
}

void
text_begin()
{
}

void
text_end()
{
}

void
text_separator()
{
    newline();
}

void
text_print_level(lev)
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
text_print_function_name(sym, has_subtree)
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
text_set_active(sym)
    Symbol *sym;
{
    sym->active = out_line;
}


void
text_header(tree)
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


