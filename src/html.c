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
html_print_level(lev)
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
html_print_function_name(sym)
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
html_set_active(sym)
    Symbol *sym;
{
    sym->active = line;
}

void
html_header(text)
    char *text;
{
    printf("%s:", text);
    newline();
}

