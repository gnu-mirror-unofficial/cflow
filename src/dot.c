/* This file is part of GNU cflow
   Copyright (C) 2021 Sergey Poznyakoff
 
   GNU cflow is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
 
   GNU cflow is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */
#include <cflow.h>

static void
dot_begin(FILE *fp)
{
     fprintf(fp, "digraph cflow {\n");
     fprintf(fp, "    node [shape=\"box\"]\n");     
}

static void
declare_node(FILE *fp, Symbol *sym)
{
     fprintf(fp, "    %s [label=\"", sym->name);
     if (sym->decl)
	  fprintf(fp, "%s\n%s:%d",
		  sym->decl, sym->source, sym->def_line);
     else
	  fprintf(fp, "%s()", sym->name);
     fprintf(fp, "\"]\n");
}
     

static void
dot_print_symbol(FILE *fp, int line, struct output_symbol *s)
{
     struct linked_list *lp;
     struct linked_list_entry *ep;

     if (s->sym->active)
	  return;
     if (s->sym->expand_line)
	  return;
     else {
	  declare_node(fp, s->sym);
	  s->sym->expand_line = line;
     }
     lp = s->direct ? s->sym->callee : s->sym->caller;
     for (ep = linked_list_head(lp); ep; ep = ep->next) {
	  Symbol *sym = ep->data;
	  if (include_symbol(sym))
	       fprintf(fp, "    %s -> %s\n", s->sym->name, sym->name);
     }
}

int
dot_output_handler(cflow_output_command cmd,
		   FILE *outfile, int line,
		   void *data, void *handler_data)
{
     switch (cmd) {
     case cflow_output_begin:
	  dot_begin(outfile);
	  break;
     case cflow_output_end:
	  fprintf(outfile, "}\n");
	  break;
     case cflow_output_symbol:
	  dot_print_symbol(outfile, line, data);
     default:
	  break;
     }
     return 0;
}
