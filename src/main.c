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
#include <string.h>
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
void set_print_option(char*);
void set_level_indent(char*);

#ifdef DEBUG
#define DEBUG_OPT "D"
#endif
#define OPTSTR "hVLvSCdxtH:p:s:glTi:P:o:m:" DEBUG_OPT


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
    "xref", no_argument, 0, 'x',
    "typedefs", no_argument, 0, 't',
    "hashsize", required_argument, 0, 'H',
    "pushdown", required_argument, 0, 'p',
    "symbol", required_argument, 0, 's',
    "ansi", no_argument, 0, 'a',
    "globals-only", no_argument, 0, 'g',
    "print-level", no_argument, 0, 'l',
    "tree", no_argument, 0, 'T',
    "level-indent", required_argument, 0, 'i',
    "print", required_argument, 0, 'P',
    "output", required_argument, 0, 'o',
    "main", required_argument, 0, 'm', 
    0,
};
#else
#define getopt_long(argc, argv, optstr, long_opts, iptr) \
 getopt(argc, argv, optstr)
#endif

/* Structure representing various arguments of command line options */
struct option_type {
    char *str;           /* optarg value */
    int min_match;       /* minimal number of characters to match */
    int type;            /* data associated with the arg */
};

static int find_option_type(struct option_type *, char *);

/* Args for --print option */
struct option_type print_optype[] = {
    "xref", 1, PRINT_XREF,
    "cross-ref", 1, PRINT_XREF,
    "tree", 1, PRINT_TREE,
    0
};
/* Args for --symbol option */
struct option_type symbol_optype[] = {
    "keyword", 2, WORD,
    "kw", 2, WORD,
    "modifier", 1, MODIFIER,
    "identifier", 1, IDENTIFIER,
    "type", 1, TYPE,
    "wrapper", 1, PARM_WRAPPER,
    0
};

char *progname;         /* program name */
#ifdef DEBUG
int debug;              /* debug mode on */
#endif
char *outname = "a.cflow"; /* default output file name */
int print_option = 0;   /* what to print. */
int verbose;            /* be verbose on output */
int ignore_indentation; /* Don't rely on indentation,
			 * i.e. don't suppose the function body
                         * is necessarily surrounded by the curly braces
			 * in the first column
                         */
int assume_cplusplus;   /* Assume C++ input always */
int record_defines;     /* Record C preproc definitions */
int record_typedefs;    /* Record typedefs */
int strict_ansi;        /* Assume sources to be written in ANSI C */
int globals_only;       /* List only global symbols */
int print_levels;       /* Print level number near every branch */
int print_as_tree;      /* Print as tree */

char level_indent[80] = "    "; /* string used to indent each successive
				 * nesting level.
				 */

char *start_name = "main"; /* Name of start symbol */

/* Structs of this type are used to temporarily hold user-provided
 * symbol information */
struct symbol_holder {
    char *name;
    int type;
};

static struct obstack temp_symbol_stack;
/* Actually it is the stack of struct symbol_holder elements.
 * This stack is used to temporarily hold symbol information obtained
 * from the command line. It is necessary since the symbol tables are not
 * initialized at the time of command line processing.
 */
int temp_symbol_count; /* number of symbols stored in temp_symbol_stack */

int
main(argc, argv)
    int argc;
    char **argv;
{
    int c, i, num;

    progname = argv[0];
    obstack_init(&temp_symbol_stack);
    sourcerc(&argc, &argv);
    
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
	    print_option = PRINT_XREF;
	    break;
	case 'P':
	    set_print_option(optarg);
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
	case 'T':
	    print_as_tree = 1;
	    break;
	case 'i':
	    set_level_indent(optarg);
	    break;
	case 'o':
	    outname = strdup(optarg);
	    break;
	case 'm':
	    start_name = strdup(optarg);
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
    if (print_option == 0)
	print_option = PRINT_TREE;
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

    /* Now that we have symbol tables, pass there all user-provided symbols
     */
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


/* Given the option_type array and (possibly abbreviated) option argument
 * find the type corresponding to that argument.
 * Return 0 if the argument does not match any one of OPTYPE entries
 */
static int
find_option_type(optype, str)
    struct option_type *optype;
    char *str;
{
    int i;
    int len = strlen(str);
    
    for ( ; optype->str; optype++) {
	if (len >= optype->min_match &&
	    strncmp(str, optype->str, len) == 0) {
	    return optype->type;
	}
    }
    return 0;
}

/* Parse the string STR and store the symbol in the temporary symbol table.
 * STR is the string of form: NAME:TYPE
 * NAME means symbol name, TYPE means symbol type (possibly abbreviated)
 */
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
	sym.type = find_option_type(symbol_optype, ptr);
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

void
set_print_option(str)
    char *str;
{
    int opt;
    
    opt = find_option_type(print_optype, str);
    if (opt == 0) {
	error(0, "unknown print option: %s", str);
	return;
    }
    print_option |= opt;
}

/* Convert first COUNT bytes of the string pointed to by STR_PTR
 * to integer using BASE. Move STR_PTR to the point where the
 * conversion stopped.
 * Return the number obtained.
 */
static int
number(str_ptr, base, count)
    char **str_ptr;
    int base;
    int count;
{
    int  c, n;
    unsigned i;
    char *str = *str_ptr;

    for (n = 0; *str && count; count--) {
	c = *str++;
	if (isdigit(c))
	    i = c - '0';
	else
	    i = toupper(c) - 'A' + 10;
	if (i > base) {
	    break;
	}
	n = n * base + i;
    }
    *str_ptr = str - 1;
    return n;
}

/* Parse and set new value for level_indent.
 * STR can contain usual C escape sequences, plus \e meaning '\033'.
 * Apart from this any character followed by xN suffix (where N is
 * a decimal number) is expanded to the sequence of N such characters.
 * 'x' looses its special meaning at the start of the string.
 */
void
set_level_indent(str)
    char *str;
{
    int i, num, c;
    char text[sizeof(level_indent)];
    char *p;

    p = text;
    while (*str) {
	switch (*str) {
	case '\\':
	    switch (*++str) {
	    case 'a':
		*p++ = '\a';
		break;
	    case 'b':
		*p++ = '\b';
		break;
	    case 'e':
		*p++ = '\033';
		break;
	    case 'f':
		*p++ = '\f';
		break;
	    case 'n':
		*p++ = '\n';
		break;
	    case 'r':
		*p++ = '\r';
		break;
	    case 't':
		*p++ = '\t';
		break;
	    case 'x':
	    case 'X':
		++str;
		*p++ = number(&str,16,2);
		break;
	    case '0':
		++str;
		*p++ = number(&str,8,3);
		break;
	    default:
		*p++ = *str;
	    }
	    ++str;
	    break;
	case 'x':
	    if (p == text) {
		goto copy;
	    }
	    num = strtol(str+1, &str, 10);
	    if (num >= sizeof(level_indent) - (p - text)) {
		error(0, "level indent string is too long. (max %d symbols)",
		      sizeof(level_indent)-1);
		return;
	    }
	    c = p[-1];
	    for (i = 1; i < num; i++)
		*p++ = c;
	    break;
	default:
	copy:
	    *p++ = *str++;
	    if (p >= text + sizeof(text) - 1) {
		error(0, "level indent string is too long. (max %d symbols)",
		      sizeof(level_indent)-1);
		return;
	    }
	}
    }
    *p = 0;
    strcpy(level_indent, text);
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


/* malloc() with error reporting
 */
void *
emalloc(size)
    int size;
{
    void *p = malloc(size);
    if (!p) 
	error(FATAL(2), "not enough core");
    return p;
}

/* just for symmetry with emalloc() */
void
efree(ptr)
    void *ptr;
{
    free(ptr);
}

/* Report the error condition.
 * STAT is the message status bitmask:
 *    SYSTEM_ERROR bits mean do perror() after printing the message
 *    FATAL_ERROR bits mean exit().
 */
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





