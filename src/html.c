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
    fprintf(outfile, "\n<BR>");
    out_line++;
}

void
html_begin()
{
    format(HEADER, NULL);
}

void
html_end()
{
    format(FOOTER, NULL);
}

void
html_print_level(lev)
    int lev;
{
    int i;
    
    if (print_levels)
	fprintf(outfile, "%4d ", lev);
    if (print_as_tree) {
	for (i = 0; i < lev; i++) {
	    if (level_mark[i])
		format(BRANCH, NULL);
	    else
		format(NOBRANCH, NULL);
	}
	format(LEAF, NULL);
    } else {
	for (i = 0; i < lev; i++)
	    format(LEVEL_INDENT, NULL);
    }
}

void
html_print_function_name(sym, has_subtree)
    Symbol *sym;
    int has_subtree;
{
    if (sym->active) {
	format(RECURSIVE_BOTTOM, sym);
	fprintf(outfile, " ");
	if (sym->v.func.source) 
	    format(DESCRIPTION, sym);
	format(RECURSIVE_REF, sym);
    } else {
	if (sym->v.func.recursive)
	    format(RECURSIVE_CALL, sym);
	else
	    format(CALL, sym);
	if (sym->v.func.source) 
	    format(DESCRIPTION, sym);
	if (!print_as_tree && has_subtree)
	    fprintf(outfile, ":");
    }
    newline();
}

void
html_set_active(sym)
    Symbol *sym;
{
    sym->active = out_line;
}

void
html_header(tree)
    enum tree_type tree;
{
    newline();
    switch (tree) {
    case DirectTree:
	format(DIRECT_TREE, NULL);
	break;
    case ReverseTree:
	format(REVERSE_TREE, NULL);
	break;
    }
    newline();
}

void
html_separator()
{
    newline();
}

