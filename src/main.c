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
#include <argp.h>
#include <stdarg.h>
#include <parser.h>

void symbol_override();
void init();
void set_print_option(char*);
static void set_level_indent(char*);
static void parse_level_string(char*, char**);

const char *argp_program_version = "cflow (" PACKAGE_NAME ") " VERSION;
const char *argp_program_bug_address = "<" PACKAGE_BUGREPORT ">";
static char doc[] = "";

static struct argp_option options[] = {
     { "verbose", 'v', NULL, 0,
       "be verbose on output", 0 },
     { "ignore-indentation", 'S', NULL, 0,
       "do not rely on indentation", 0 },
     { "c++", 'C', NULL, 0,
       "expect C++ input", 0 },
     { "defines" , 'd', NULL, 0,
       "record defines", 0 },
     { "xref", 'x', NULL, 0,
       "produce cross-reference listing only" },
     { "typedefs", 't', NULL, 0,
       "record typedefs" },
     { "pushdown", 'p', "VALUE", 0,
       "set initial token stack size to VALUE", 0 },
     { "symbol", 's', "SYM:TYPE", 0,
       "make cflow believe the symbol SYM is of type TYPE. Valid types are: keyword (or kw), modifier, identifier, type, wrapper. Any unambiguous abbreviation of the above is also accepted", 0 },
     { "ansi", 'a', NULL, 0,
       "Assume input to be written in ANSI C", 0 },
     { "globals-only", 'g', NULL, 0,
       "Record only global symbols" },
     { "print-level", 'l', NULL, 0,
       "Print nesting level along with the call tree", 0 },
     { "tree", 'T', NULL, 0,
       "Draw tree", 0 },
     { "level-indent", 'i', "STRING", 0,
       "Use STRING when indenting to each new level", 0 },
     { "print", 'P', "OPT", 0,
       "Set printing option to OPT. Valid OPT values are: xref (or cross-ref), tree. Any unambiguous abbreviation of the above is also accepted" },
     { "output", 'o', "FILE", 0,
       "set output file name (default -, meaning stdout)" },
     { "main", 'm', "NAME", 0,
       "Assume main function to be called NAME", 0 },
     { "brief", 'b', NULL, 0,
       "brief output" },
     { "reverse", 'r', NULL, 0,
       "Print reverse call tree", 0 },
     { "format", 'f', "NAME", 0,
       "use given output format NAME. Valid names are gnu (default) and posix", 0},
     { "license", 'L', 0, 0,
       "Print license and exit", 0 },
     { 0, }
};

char *cflow_license_text =
"   GNU cflow is free software; you can redistribute it and/or modify\n"
"   it under the terms of the GNU General Public License as published by\n"
"   the Free Software Foundation; either version 2 of the License, or\n"
"   (at your option) any later version.\n"
"\n"
"   GNU cflow is distributed in the hope that it will be useful,\n"
"   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"   GNU General Public License for more details.\n"
"\n"
"   You should have received a copy of the GNU General Public License\n"
"   along with GNU cflow; if not, write to the Free Software\n"
"   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA\n\n";

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
     int num;
     
     switch (key) {
     case 'a':
	  strict_ansi = 1;
	  break;
     case 'C':
	  assume_cplusplus = 1;
	  break;
     case 'D':
	  debug = 1;
	  break;
     case 'L':
	  printf("License for %s:\n\n", argp_program_version);
	  printf("%s", cflow_license_text);
	  exit(0);
     case 'P':
	  set_print_option(arg);
	  break;
     case 'S':
	  ignore_indentation = 1;
	  break;
     case 'T':
	  print_as_tree = 1;
	  level_indent[0] = "  "; /* two spaces */
	  level_indent[1] = "| ";
	  level_end[0] = "+-";
	  level_end[1] = "\\-";
	  break;
     case 'b':
	  brief_listing = 1;
	  break;
     case 'd':
	  record_defines = 1;
	  break;
     case 'f':
	  if (select_output_driver(arg))
	       argp_error(state, "%s: No such output driver", optarg);
	  break;
     case 'g':
	  globals_only = 1;
	  break;
     case 'i':
	  set_level_indent(arg);
	  break;
     case 'l':
	  print_levels = 1;
	  break;	
     case 'm':
	  start_name = strdup(arg);
	  break;
     case 'o':
	  outname = strdup(arg);
	  break;
     case 'p':
	  num = atoi(arg);
	  if (num > 0)
	       token_stack_length = num;
	  break;
     case 'r':
	  reverse_tree = 1;
	  break;
     case 's':
	  symbol_override(arg);
	  break;
     case 't':
	  record_typedefs = 1;
	  break;
     case 'v':
	  verbose = 1;
	  break;
     case 'x':
	  print_option = PRINT_XREF;
	  break;
     default:
	  return ARGP_ERR_UNKNOWN;
     }
     return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  "[FILE]...",
  doc,
  NULL,
  NULL,
  NULL
};

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
int debug;              /* debug mode on */
char *outname = "-";    /* default output file name */
int print_option = 0;   /* what to print. */
int verbose;            /* be verbose on output */
int ignore_indentation; /* Don't rely on indentation,
			 * i.e. don't suppose the function body
                         * is necessarily surrounded by the curly braces
			 * in the first column
                         */
int assume_cplusplus;   /* Assume C++ input always */
int record_defines;     /* Record macro definitions */
int record_typedefs;    /* Record typedefs */
int strict_ansi;        /* Assume sources to be written in ANSI C */
int globals_only;       /* List only global symbols */
int print_levels;       /* Print level number near every branch */
int print_as_tree;      /* Print as tree */
int brief_listing;      /* Produce short listing */
int reverse_tree;       /* Generate reverse tree */
			   
char *level_indent[] = { NULL, NULL };
char *level_end[] = { "", "" };
char *level_begin = "";

char *start_name = "main"; /* Name of start symbol */

void
xalloc_die(void)
{
     error(1, ENOMEM, "");
}

int
main(int argc, char **argv)
{
     int i;
     int index;

     progname = argv[0];
     register_output("gnu", gnu_output_handler, NULL);
     register_output("posix", posix_output_handler, NULL);
     sourcerc(&argc, &argv);

     if (argp_parse (&argp, argc, argv, 0, &index, NULL))
	  exit (1);
     
     if (argv[optind] == NULL)
	  error(1, 0, "No input files");
     if (print_option == 0)
	  print_option = PRINT_TREE;

     init();

     argc -= index;
     argv += index;
     while (argc--) {
	  if (source(*argv++) == 0)
	       yyparse();
     }
     cleanup();

     output();
     return 0;
}

void
init()
{
     int i;
     
     if (level_indent[0] == NULL) {
	  level_indent[0] = level_indent[1] = "    "; /* 4 spaces */
	  level_end[0] = level_end[1] = "";
     }
     
     init_lex();
     init_parse();
}


/* Given the option_type array and (possibly abbreviated) option argument
 * find the type corresponding to that argument.
 * Return 0 if the argument does not match any one of OPTYPE entries
 */
static int
find_option_type(struct option_type *optype, char *str)
{
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
symbol_override(char *str)
{
     int type;
     char *ptr;
     Symbol *sp;
     
     ptr = str;
     while (*ptr && *ptr != ':') 
	  ptr++;
     if (*ptr == ':') {
	  *ptr++ = 0;
	  type = find_option_type(symbol_optype, ptr);
	  if (type == 0) {
	       error(0, 0, "unknown symbol type: %s", ptr);
	       return;
	  }
     } else
	  type = IDENTIFIER;
     sp = install(str);
     sp->type = SymToken;
     sp->v.type.token_type = type;
     sp->v.type.source = NULL;
     sp->v.type.def_line = -1;
     sp->v.type.ref_line = NULL;
}

void
set_print_option(char *str)
{
     int opt;
     
     opt = find_option_type(print_optype, str);
     if (opt == 0) {
	  error(0, 0, "unknown print option: %s", str);
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
number(char **str_ptr, int base, int count)
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

/* Processing for --level option
 * The option syntax is
 *    --level KEYWORD=STR
 * where
 *    KEYWORD is one of "begin", "0", ", "1", "end0", "end1",
 *    or an abbreviation thereof,
 *    STR is the value to be assigned to the parameter.
 *
 * STR can contain usual C escape sequences plus \e meaning '\033'.
 * Apart from this any character followed by xN suffix (where N is
 * a decimal number) is expanded to the sequence of N such characters.
 * 'x' looses its special meaning at the start of the string.
 */
#define MAXLEVELINDENT 216
#define LEVEL_BEGIN 1
#define LEVEL_INDENT0 2
#define LEVEL_INDENT1 3
#define LEVEL_END0 4
#define LEVEL_END1 5

struct option_type level_indent_optype[] = {
     "begin", 1, LEVEL_BEGIN,
     "0", 1, LEVEL_INDENT0,
     "1", 1, LEVEL_INDENT1,
     "end0", 4, LEVEL_END0,
     "end1", 4, LEVEL_END1,
};

void
set_level_indent(char *str)
{
     char *p;

     p = str;
     while (*p != '=') {
	  if (*p == 0) {
	       error(0, 0, "level-indent syntax");
	       return;
	  }
	  p++;
     }
     *p++ = 0;
    
     switch (find_option_type(level_indent_optype, str)) {
     case LEVEL_BEGIN:
	  parse_level_string(p, &level_begin);
	  break;
     case LEVEL_INDENT0:
	  parse_level_string(p, &level_indent[0]);
	  break;
     case LEVEL_INDENT1:
	  parse_level_string(p, &level_indent[1]);
	  break;
     case LEVEL_END0:
	  parse_level_string(p, &level_end[0]);
	  break;
     case LEVEL_END1:
	  parse_level_string(p, &level_end[1]);
	  break;
     default:
	  error(0, 0, "unknown level indent option: %s", str);
     }
}

void
parse_level_string(char *str, char **return_ptr)
{
     static char text[MAXLEVELINDENT];
     char *p;
     int i, c, num;
    
     p = text;
     memset(text, ' ', sizeof(text));
     text[sizeof(text)-1] = 0;
     
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
	       c = p[-1];
	       for (i = 1; i < num; i++) {
		    *p++ = c;
		    if (*p == 0) {
			 error(0, 0, "level indent string too long");
			 return;
		    }
	       }
	       break;
	  default:
	  copy:
	       *p++ = *str++;
	       if (*p == 0) {
		    error(0, 0, "level indent string is too long");
		    return;
	       }
	  }
     }
     *p = 0;
     *return_ptr = strdup(text);
}







