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

static int line=1;

void
newline()
{
    printf("\n");
    line++;
}

void
text_print_level(lev)
    int lev;
{
    int i;
    
    if (print_levels)
	printf("%4d ", lev);
    if (print_as_tree) {
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
}

void
text_print_function_name(sym)
    Symbol *sym;
{
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
}

void
text_set_active(sym)
    Symbol *sym;
{
    sym->active = line;
}


void
text_header(text)
    char *text;
{
    newline();
    printf("%s:", text);
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

