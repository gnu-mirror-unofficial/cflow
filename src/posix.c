/* This file is part of GNU cflow
   Copyright (C) 1997,2005 Sergey Poznyakoff
 
   GNU cflow is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   GNU cflow is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU cflow; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include <cflow.h>

static void
print_symbol(FILE *outfile, int line, struct output_symbol *s)
{
     if (s->direct) {
	  fprintf(outfile, "%d %*s%s: ", \
		  line,
		  3*s->level, "",
		  s->sym->name);

	  if (s->sym->v.func.type) {
	       char *p = s->sym->v.func.type;
	       
	       while (*p) {
		    char *q = p;

		    /* Skip whitespace */
		    while (*q && isspace (*q))
			 q++;

		    if (*q == '(') {
			 /* Do not print function name */
			 p = q;
			 break;
		    }

		    /* Skip identifier */
		    while (*q && !isspace (*q))
			 q++;
		    
		    for (; p < q; p++)
			 fputc(*p, outfile);
	       }

	       fprintf(outfile, "%s, <%s %d>",
		       p,
		       s->sym->v.func.source,
		       s->sym->v.func.def_line);
	  } else
	       fprintf(outfile, "<>");
     }
}

void
posix_output_handler(cflow_output_command cmd,
		     FILE *outfile, int line,
		     void *data, void *handler_data)
{
     switch (cmd) {
     case cflow_output_begin:
     case cflow_output_end:
	  break;
     case cflow_output_newline:
     case cflow_output_separator:
	  fprintf(outfile, "\n");
	  break;
     case cflow_output_text:
	  fprintf(outfile, "%s\n", data);
	  break;
     case cflow_output_symbol:
	  print_symbol(outfile, line, data);
     }
}
