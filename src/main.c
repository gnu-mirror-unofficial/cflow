/*  $Id$
 *  cflow main module
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
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <varargs.h>
#include <errno.h>
#include "cflow.h"
#include "parser.h"
#include "version.h"
#include "obstack1.h"

void say_and_die(char *);
void symbol_override();
void init();

#ifdef DEBUG
#define DEBUG_OPT "D"
#endif
#define OPTSTR "hVLvSCdxtH:p:s:glmTi:" DEBUG_OPT


#ifdef GNU_STYLE_OPTIONS
typedef struct option LONGOPT;
LONGOPT longopts[] = {
    "help",    no_argument, 0, 'h',
    "version", no_argument, 0, 'V',
    "licence", no_argument, 0, 'L',
    "verbose", no_argument, 0, 'v',
    "ignore-indentation", no_argument, 0, 'S',
    "c++", no_argument, 0, 'C',
    "defines" , no_argument, 0, 'd',
    "cxref", no_argument, 0, 'x',
    "typedefs", no_argument, 0, 't',
    "hashsize", required_argument, 0, 'H',
    "pushdown", required_argument, 0, 'p',
    "symbol", required_argument, 0, 's',
    "ansi", no_argument, 0, 'a',
    "globals-only", no_argument, 0, 'g',
    "print-level", no_argument, 0, 'l',
    "html", no_argument, 0, 'm',
    "tree", no_argument, 0, 'T',
    "level-indent", required_argument, 0, 'i',
    0,
};
#else
#define getopt_long(argc, argv, optstr, long_opts, iptr) \
 getopt(argc, argv, optstr)
#endif

char *progname;
#ifdef DEBUG
int debug;
#endif
int output_mode = OUT_TEXT;
int verbose;            /* be verbose on output */
int ignore_indentation; /* Don't rely on indentation,
			 * i.e. don't suppose the function body
                         * is necessarily surrounded by the curly braces
			 * in the first column
                         */
int assume_cplusplus;   /* Assume C++ input always */
int record_defines;     /* Record C preproc definitions */
int record_typedefs;    /* Record typedefs */
int cross_ref;          /* Generate cross-reference */
int strict_ansi;        /* Assume sources to be written in ANSI C */
int globals_only;       /* List only global symbols */
int print_levels;       /* Print level number near every branch */
int print_as_tree;      /* Print as tree */

char level_indent[80] = "    ";

struct symbol_holder {
    char *name;
    int type;
};
static struct obstack temp_symbol_stack;
int temp_symbol_count;

int
main(argc, argv)
    int argc;
    char **argv;
{
    int c, i, num;

    progname = argv[0];
    obstack_init(&temp_symbol_stack);
    while ((c = getopt_long(argc, argv, OPTSTR, longopts, &i)) != EOF) {
	switch (c) {
	case 'h':
	    say_and_die(usage);
	    return 0;
	case 'V':
	    say_and_die(version);
	    return 0;
	case 'L':
	    say_and_die(licence);
	    return 0;
	case 'v':
	    verbose = 1;
	    break;
	case 'S':
	    ignore_indentation = 1;
	    break;
	case 'C':
	    assume_cplusplus = 1;
	    break;
	case 'd':
	    record_defines = 1;
	    break;
	case 't':
	    record_typedefs = 1;
	    break;
	case 'x':
	    cross_ref = 1;
	    break;
	case 'a':
	    strict_ansi = 1;
	    break;
	case 'H':
	    set_hash_size(atoi(optarg));
	    break;
	case 'p':
	    num = atoi(optarg);
	    if (num > 0)
		token_stack_length = num;
	    break;
	case 's':
	    symbol_override(optarg);
	    break;
	case 'g':
	    globals_only = 1;
	    break;
	case 'l':
	    print_levels = 1;
	    break;
	case 'm':
	    output_mode = OUT_HTML;
	    break;
	case 'T':
	    print_as_tree = 1;
	    break;
	case 'i':
	    if (strlen(level_indent) >= sizeof(level_indent))
		error(0, "level indent string is too long. (max %d symbols)",
		      sizeof(level_indent)-1);
	    else
		strcpy(level_indent, optarg);
	    break;
#ifdef DEBUG
	case 'D':
	    debug = 1;
	    break;
#endif
	}
    }
    if (argv[optind] == NULL)
	error(FATAL(1), "No input files");
    init();
    collect(argc-optind, argv+optind);
    output();
    return 0;
}

void
init()
{
    struct symbol_holder *hold;
    int i;
    Symbol *sp;
    
    init_hash();
    init_lex();
    init_parse();

    if (temp_symbol_count) {
	hold = obstack_finish(&temp_symbol_stack);
	for (i = 0; i < temp_symbol_count; i++, hold++) {
	    sp = install(hold->name);
	    sp->type = SymToken;
	    sp->v.type.token_type = hold->type;
	    sp->v.type.source = NULL;
	    sp->v.type.def_line = -1;
	    sp->v.type.ref_line = NULL;
	}
    }
    obstack_free(&temp_symbol_stack, NULL);
}

struct option_type {
    char *str;
    int min_match;
    int type;
} optype[] = {
    "keyword", 2, WORD,
    "kw", 2, WORD,
    "modifier", 1, MODIFIER,
    "identifier", 1, IDENTIFIER,
    "type", 1, TYPE,
    "wrapper", 1, PARM_WRAPPER,
};

static int
find_option_type(str)
    char *str;
{
    int i;
    int len = strlen(str);
    
    for (i = 0; i < NUMITEMS(optype); i++) {
	if (len >= optype[i].min_match &&
	    strncmp(str, optype[i].str, len) == 0) {
	    return optype[i].type;
	}
    }
    return 0;
}

void
symbol_override(str)
    char *str;
{
    struct symbol_holder sym;
    char *ptr;
    
    ptr = str;
    while (*ptr && *ptr != ':') 
	ptr++;
    if (*ptr == ':') {
	*ptr++ = 0;
	sym.type = find_option_type(ptr);
	if (sym.type == 0) {
	    error(0, "unknown symbol type: %s", ptr);
	    return;
	}
    } else
	sym.type = IDENTIFIER;
    sym.name = str;
    obstack_grow(&temp_symbol_stack, &sym, sizeof(sym));
    temp_symbol_count++;
}

/* Print text on console and exit. Before printing scan text for
 * RCS keywords ("\$[a-zA-Z]+:\(.*\)\$") and replace them with the keyword's 
 * walue ("\1").
 * NOTE: The function does not check the whole regexp above, it simply
 * assumes that every occurence of '$' in the text is a start of RCS keyword.
 * It suffices for all used messages, but if you add any messages containing
 * '$' on it's own, you have to modify helptext() accordingly.
 */
void
say_and_die(text)
    char *text;
{
    char *p;

    p = text;
    while (*p) {
	if (*p == '$') {
	    *p = 0;
	    printf("%s", text);
	    *p = '$';
	    while (*p++ != ':')
		;
	    text = p;
	    while (*p != '$')
		p++;
	    *p = 0;
	    printf("%s", text);
	    *p++ = '$';
	    text = p;
	} else
	    p++;
    }
    if (p > text)
	printf("%s", text);
    exit(0);
}


void *
emalloc(size)
    int size;
{
    void *p = malloc(size);
    if (!p) 
	error(FATAL(2), "not enough core");
    return p;
}

void
efree(ptr)
    void *ptr;
{
    free(ptr);
}

error(stat, fmt, va_alist)
    char *fmt;
    va_dcl
{
    va_list ap;
    va_start(ap);
    fprintf(stderr, "%s: ", progname);
    vfprintf(stderr, fmt, ap);
    if (stat & SYSTEM_ERROR)
	fprintf(stderr, ": %s", strerror(errno));
    fprintf(stderr, "\n");
    va_end(ap);
    if (stat & FATAL_ERROR)
	exit(stat & 255);
}
