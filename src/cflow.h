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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free
#include <obstack.h>
#include <error.h>
#include <xalloc.h>

#define GNU_STYLE_OPTIONS 1

#define SYSTEM_ERROR 0x0100
#define FATAL_ERROR  0x0200
#define FATAL(n) FATAL_ERROR|(n)

#define NUMITEMS(a) sizeof(a)/sizeof((a)[0])

typedef struct cons *Consptr;
typedef struct cons Cons;
struct cons {
     Consptr car;
     Consptr cdr;
};

#define CAR(a) (a)->car
#define CDR(a) (a)->cdr

enum symtype {
     SymUndefined,
     SymToken,
     SymFunction
};

enum storage {
     ExternStorage,
     ExplicitExternStorage,
     StaticStorage,
     AutoStorage,
     AnyStorage
};

typedef struct {
     int line;
     char *source;
} Ref;

typedef struct symbol Symbol;

struct symbol {
     Symbol *next;
     enum symtype type;
     char *name;
     int active;
     int expand_line;
     union {
	  struct {
	       int token_type;
	       char *source;
	       int def_line;
	       Consptr ref_line;
	  } type;
	  struct {
	       int token_type;
	       char *source;
	       int def_line;
	       Consptr ref_line;
	       char *type;
	       enum storage storage;
	       int argc;
	       char *args;
	       int recursive;      /* for functions only */
	       int level;          /* for local vars only */
	       Consptr caller;
	       Consptr callee;
	  } func;
     } v;
};

/* Output flags */
#define PRINT_XREF 0x01
#define PRINT_TREE 0x02

#define MAX_OUTPUT_DRIVERS 8

extern char *level_mark;
extern FILE *outfile;
extern char *outname;

extern int verbose;
extern int print_option;
extern int ignore_indentation;
extern int assume_cplusplus;
extern int record_defines;
extern int strict_ansi;
extern char *level_indent[];
extern char *level_end[];
extern char *level_begin;
extern int print_levels;
extern int print_as_tree;
extern int brief_listing;
extern int reverse_tree;
extern int out_line;
extern char *start_name;
extern int max_depth;
extern int debug;

extern int token_stack_length;
extern int token_stack_increase;

extern int symbol_count;

Symbol *lookup(char*);
Symbol *install(char*);
int source(char *name);
int yyparse(void);
void cleanup(void);
void output(void);
void init_lex(int debug_level);
void init_parse(void);
void delete_autos(int level);
void delete_statics(void);
int globals_only(void);
int include_symbol(Symbol *sym);

int collect_symbols(Symbol ***, int (*sel)());
Consptr append_to_list(Consptr *, void *);
int symbol_in_list(Symbol *sym, Consptr list);
void sourcerc(int *, char ***);

typedef enum {
     cflow_output_begin,
     cflow_output_end,
     cflow_output_newline,
     cflow_output_separator,
     cflow_output_symbol,
     cflow_output_text
} cflow_output_command;

struct output_symbol {
     int direct;
     int level;
     int last;
     Symbol *sym;
};

int register_output(const char *name,
		    int (*handler) (cflow_output_command cmd,
				    FILE *outfile, int line,
				    void *data, void *handler_data),
		    void *handler_data);
int select_output_driver (const char *name);

int gnu_output_handler(cflow_output_command cmd,
		       FILE *outfile, int line,
		       void *data, void *handler_data);
int posix_output_handler(cflow_output_command cmd,
			 FILE *outfile, int line,
			 void *data, void *handler_data);

