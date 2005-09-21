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

   You should have received a copy of the GNU General Public
   License along with GNU cflow; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include <cflow.h>
#include <argp.h>
#include <stdarg.h>
#include <parser.h>

const char *argp_program_version = "cflow (" PACKAGE_NAME ") " VERSION;
const char *argp_program_bug_address = "<" PACKAGE_BUGREPORT ">";
static char doc[] = N_("generate a program flowgraph");

#define OPT_DEFINES       256
#define OPT_LEVEL_INDENT  257
#define OPT_DEBUG         258
#define OPT_PREPROCESS    259
#define OPT_NO_PREPROCESS 260

static struct argp_option options[] = {
#define GROUP_ID 0
     { NULL, 0, NULL, 0,
       N_("General options:"), GROUP_ID },
     { "depth", 'd', N_("NUMBER"), 0,
       N_("Set the depth at which the flowgraph is cut off"), GROUP_ID+1 },
     { "include", 'i', N_("SPEC"), 0,
       N_("Control the number of included symbols. SPEC is a string consisting of characters, specifying what class of symbols to include in the output (see below)."), GROUP_ID+1 },


     { "format", 'f', N_("NAME"), 0,
       N_("Use given output format NAME. Valid names are gnu (default) and posix"),
       GROUP_ID+1 },
     { "reverse", 'r', NULL, 0,
       N_("Print reverse call tree"), GROUP_ID+1 },
     { "xref", 'x', NULL, 0,
       N_("Produce cross-reference listing only"), GROUP_ID+1 },
     { "print", 'P', N_("OPT"), 0,
       N_("Set printing option to OPT. Valid OPT values are: xref (or cross-ref), tree. Any unambiguous abbreviation of the above is also accepted"),
       GROUP_ID+1 },
     { "output", 'o', N_("FILE"), 0,
       N_("Set output file name (default -, meaning stdout)"),
       GROUP_ID+1 },

     { NULL, 0, NULL, 0,
       N_("Valid SPEC symbols for --include option:"), GROUP_ID+2 },
     {"  +", 0, NULL, OPTION_DOC|OPTION_NO_TRANS,
      N_("Include symbols denoted by the following letters (default)"),
      GROUP_ID+3 },
     {"  - or ^", 0, NULL, OPTION_DOC|OPTION_NO_TRANS,
      N_("Exclude symbols denoted by the following letters (default)"),
      GROUP_ID+3 },
     {"  x", 0, NULL, OPTION_DOC|OPTION_NO_TRANS,
      N_("all data symbols, both external and static"), GROUP_ID+3 },
     {"  _",  0, NULL, OPTION_DOC|OPTION_NO_TRANS,
      N_("symbols whose names begin with an underscore"), GROUP_ID+3 },
     {"  s",  0, NULL, OPTION_DOC|OPTION_NO_TRANS,
      N_("static symbols"), GROUP_ID+3 },
     {"  t",  0, NULL, OPTION_DOC|OPTION_NO_TRANS,
      N_("typedefs (for cross-references only)"), GROUP_ID+3 },
     
#undef GROUP_ID
#define GROUP_ID 10     
     { NULL, 0, NULL, 0,
       N_("Parser control:"), GROUP_ID },
     { "use-indentation", 'S', N_("BOOL"), OPTION_ARG_OPTIONAL,
       N_("Rely on indentation"), GROUP_ID+1 },
     { "ansi", 'a', NULL, 0,
       N_("Assume input to be written in ANSI C"), GROUP_ID+1 },
     { "pushdown", 'p', N_("NUMBER"), 0,
       N_("Set initial token stack size to NUMBER"), GROUP_ID+1 },
     { "symbol", 's', N_("SYM:TYPE"), 0,
       N_("Make cflow believe the symbol SYM is of type TYPE. Valid types are: keyword (or kw), modifier, identifier, type, wrapper. Any unambiguous abbreviation of the above is also accepted"), GROUP_ID+1 },
     { "main", 'm', N_("NAME"), 0,
       N_("Assume main function to be called NAME"), GROUP_ID+1 },
     { "define", 'D', N_("NAME[=DEFN]"), 0,
       N_("Predefine NAME as a macro"), GROUP_ID+1 },
     { "undefine", 'U', N_("NAME"), 0,
       N_("Cancel any previous definition of NAME"), GROUP_ID+1 },
     { "include-dir", 'I', N_("DIR"), 0,
       N_("Add the directory DIR to the list of directories to be searched for header files."), GROUP_ID+1 },
     { "preprocess", OPT_PREPROCESS, N_("COMMAND"), OPTION_ARG_OPTIONAL,
       N_("Run the specified preprocessor command"), GROUP_ID+1 },
     { "cpp", 0, NULL, OPTION_ALIAS, NULL, GROUP_ID+1 },
     { "no-preprocess", OPT_NO_PREPROCESS, NULL, 0,
       N_("Do not preprocess the sources"), GROUP_ID+1 },
     { "no-cpp", 0, NULL, OPTION_ALIAS, NULL, GROUP_ID+1 },
#undef GROUP_ID
#define GROUP_ID 20          
     { NULL, 0, NULL, 0,
       N_("Output control:"), GROUP_ID },
     { "number", 'n', N_("BOOL"), OPTION_ARG_OPTIONAL,
       N_("Print line numbers"), GROUP_ID+1 },
     { "print-level", 'l', NULL, 0,
       N_("Print nesting level along with the call tree"), GROUP_ID+1 },
     { "level-indent", OPT_LEVEL_INDENT, "STRING", 0,
       N_("Use STRING when indenting to each new level"), GROUP_ID+1 },
     { "tree", 'T', NULL, 0,
       N_("Draw tree"), GROUP_ID+1 },
     { "brief", 'b', N_("BOOL"), OPTION_ARG_OPTIONAL,
       N_("Brief output"), GROUP_ID+1 },
#undef GROUP_ID
#define GROUP_ID 30                 
     { NULL, 0, NULL, 0,
       N_("Informational options:"), GROUP_ID },
     { "verbose", 'v', NULL, 0,
       N_("Be verbose on output"), GROUP_ID+1 },
     { "license", 'L', 0, 0,
       N_("Print license and exit"), GROUP_ID+1 },
     { "debug", OPT_DEBUG, "NUMBER", OPTION_ARG_OPTIONAL,
       N_("Set debugging level"), GROUP_ID+1 },
#undef GROUP_ID     
     { 0, }
};

char *cflow_license_text = N_(
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
"   along with GNU cflow; if not, write to the Free Software Foundation,\n"
"   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n"
"\n"
"\n"
);

/* Structure representing various arguments of command line options */
struct option_type {
    char *str;           /* optarg value */
    int min_match;       /* minimal number of characters to match */
    int type;            /* data associated with the arg */
};

int debug;              /* debug level */
char *outname = "-";    /* default output file name */
int print_option = 0;   /* what to print. */
int verbose;            /* be verbose on output */
int use_indentation;    /* Rely on indentation,
			 * i.e. suppose the function body
                         * is necessarily surrounded by the curly braces
			 * in the first column
                         */
int record_defines;     /* Record macro definitions */
int strict_ansi;        /* Assume sources to be written in ANSI C */
int print_line_numbers; /* Print line numbers */
int print_levels;       /* Print level number near every branch */
int print_as_tree;      /* Print as tree */
int brief_listing;      /* Produce short listing */
int reverse_tree;       /* Generate reverse tree */
int max_depth;          /* The depth at which the flowgraph is cut off */

#define SM_FUNCTIONS   0x0001
#define SM_DATA        0x0002
#define SM_STATIC      0x0004
#define SM_UNDERSCORE  0x0008
#define SM_TYPEDEF     0x0010

#define CHAR_TO_SM(c) ((c)=='x' ? SM_DATA : \
                        (c)=='_' ? SM_UNDERSCORE : \
                         (c)=='s' ? SM_STATIC : \
                          (c)=='t' ? SM_TYPEDEF : 0)
#define SYMBOL_INCLUDE(c) (symbol_map |= CHAR_TO_SM(c))
#define SYMBOL_EXCLUDE(c) (symbol_map &= ~CHAR_TO_SM(c))
int symbol_map;  /* A bitmap of symbols included in the graph. */

char *level_indent[] = { NULL, NULL };
char *level_end[] = { "", "" };
char *level_begin = "";

int preprocess_option = 0; /* Do they want to preprocess sources? */

char *start_name = "main"; /* Name of start symbol */

Consptr arglist;        /* List of command line arguments */

#define boolean_value(arg) ((arg) ? ((arg)[0] == 'y' || (arg)[0] == 'Y') : 1)

/* Given the option_type array and (possibly abbreviated) option argument
 * find the type corresponding to that argument.
 * Return 0 if the argument does not match any one of OPTYPE entries
 */
static int
find_option_type(struct option_type *optype, const char *str, int len)
{
     if (len == 0)
	  len = strlen(str);
     for ( ; optype->str; optype++) {
	  if (len >= optype->min_match &&
	      memcmp(str, optype->str, len) == 0) {
	       return optype->type;
	  }
     }
     return 0;
}

/* Args for --symbol option */
static struct option_type symbol_optype[] = {
     { "keyword", 2, WORD },
     { "kw", 2, WORD },
     { "modifier", 1, MODIFIER },
     { "identifier", 1, IDENTIFIER },
     { "type", 1, TYPE },
     { "wrapper", 1, PARM_WRAPPER },
     { 0 },
};

/* Parse the string STR and store the symbol in the temporary symbol table.
 * STR is the string of form: NAME:TYPE
 * NAME means symbol name, TYPE means symbol type (possibly abbreviated)
 */
static void
symbol_override(const char *str)
{
     int type;
     const char *ptr;
     char *name;
     Symbol *sp;
     
     ptr = strchr(str, ':');
     if (*ptr == ':') {
	  type = find_option_type(symbol_optype, ptr+1, 0);
	  if (type == 0) {
	       error(0, 0, _("unknown symbol type: %s"), ptr+1);
	       return;
	  }
     } else
	  type = IDENTIFIER;
     name = strndup(str, ptr - str);
     sp = install(name);
     sp->type = SymToken;
     sp->token_type = type;
     sp->source = NULL;
     sp->def_line = -1;
     sp->ref_line = NULL;
}

/* Args for --print option */
static struct option_type print_optype[] = {
     { "xref", 1, PRINT_XREF },
     { "cross-ref", 1, PRINT_XREF },
     { "tree", 1, PRINT_TREE },
     { 0 },
};

static void
set_print_option(char *str)
{
     int opt;
     
     opt = find_option_type(print_optype, str, 0);
     if (opt == 0) {
	  error(0, 0, _("unknown print option: %s"), str);
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
number(const char **str_ptr, int base, int count)
{
     int  c, n;
     unsigned i;
     const char *str = *str_ptr;
     
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
 *    --level NUMBER
 * or
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

static struct option_type level_indent_optype[] = {
     { "begin", 1, LEVEL_BEGIN },
     { "0", 1, LEVEL_INDENT0 },
     { "1", 1, LEVEL_INDENT1 },
     { "end0", 4, LEVEL_END0 },
     { "end1", 4, LEVEL_END1 },
};

static void
parse_level_string(const char *str, char **return_ptr)
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
			 error(1, 0, _("level indent string is too long"));
			 return;
		    }
	       }
	       break;
	  default:
	  copy:
	       *p++ = *str++;
	       if (*p == 0) {
		    error(1, 0, _("level indent string is too long"));
		    return;
	       }
	  }
     }
     *p = 0;
     *return_ptr = strdup(text);
}

static void
set_level_indent(const char *str)
{
     long n;
     const char *p;
     char *q;
     
     n = strtol(str, &q, 0);
     if (*q == 0 && n > 0) {
	  char *s = xmalloc(n+1);
	  memset(s, ' ', n-1);
	  s[n-1] = 0;
	  level_indent[0] = level_indent[1] = s;
	  return;
     }
     
     p = str;
     while (*p != '=') {
	  if (*p == 0) {
	       error(1, 0, _("level-indent syntax"));
	       return;
	  }
	  p++;
     }
     ++p;
    
     switch (find_option_type(level_indent_optype, str, p - str - 1)) {
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
	  error(1, 0, _("unknown level indent option: %s"), str);
     }
}

static void
add_name(const char *name)
{
     append_to_list(&arglist, (void*) name);
}

static void
add_preproc_option(int key, const char *arg)
{
     char *opt = xmalloc(3 + strlen(arg));
     sprintf(opt, "-%c%s", key, arg);
     add_name(opt);
     preprocess_option = 1;
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
     int num;
     
     switch (key) {
     case 'a':
	  strict_ansi = 1;
	  break;
     case OPT_DEBUG:
	  debug = arg ? atoi(arg) : 1;
	  break;
     case 'L':
	  printf(_("License for %s:\n\n"), argp_program_version);
	  printf("%s", cflow_license_text);
	  exit(0);
     case 'P':
	  set_print_option(arg);
	  break;
     case 'S':
	  use_indentation = boolean_value(arg);
	  break;
     case 'T':
	  print_as_tree = 1;
	  set_level_indent("0=  "); /* two spaces */
	  set_level_indent("1=| ");
	  set_level_indent("end0=+-");
	  set_level_indent("end1=\\\\-");
	  break;
     case 'b':
	  brief_listing = boolean_value(arg);
	  break;
     case 'd':
	  max_depth = atoi(arg);
	  if (max_depth < 0)
	       max_depth = 0;
	  break;
     case OPT_DEFINES:
	  record_defines = 1;
	  break;
     case 'f':
	  if (select_output_driver(arg))
	       argp_error(state, _("%s: No such output driver"), optarg);
	  output_init();
	  break;
     case OPT_LEVEL_INDENT:
	  set_level_indent(arg);
	  break;
     case 'i':
	  num = 1;
	  for (; *arg; arg++) 
	       switch (*arg) {
	       case '-':
	       case '^':
		    num = 0;
		    break;
	       case '+':
		    num = 1;
		    break;
	       case 'x':
	       case '_':
	       case 's':
	       case 't':
		    if (num)
			 SYMBOL_INCLUDE(*arg);
		    else
			 SYMBOL_EXCLUDE(*arg);
		    break;
	       default:
		    argp_error(state, _("Unknown symbol class: %c"), *arg);
	       }
	  break;
     case 'l':
	  print_levels = 1;
	  break;	
     case 'm':
	  start_name = strdup(arg);
	  break;
     case 'n':
	  print_line_numbers = boolean_value(arg);
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
     case 'v':
	  verbose = 1;
	  break;
     case 'x':
	  print_option = PRINT_XREF;
	  break;
     case OPT_PREPROCESS:
	  preprocess_option = 1;
	  set_preprocessor(arg ? arg : CFLOW_PREPROC);
	  break;
     case OPT_NO_PREPROCESS:
	  preprocess_option = 0;
	  break;
     case ARGP_KEY_ARG:
	  add_name(arg);
	  break;
     case 'I':
     case 'D':
     case 'U':
	  add_preproc_option(key, arg);
	  break;
     default:
	  return ARGP_ERR_UNKNOWN;
     }
     return 0;
}

static struct argp argp = {
     options,
     parse_opt,
     N_("[FILE]..."),
     doc,
     NULL,
     NULL,
     NULL
};

int
globals_only()
{
     return !(symbol_map & SM_STATIC);
}

int
include_symbol(Symbol *sym)
{
     int type = 0;
     
     if (!sym)
	  return 0;
     
     if (sym->type == SymIdentifier) {
	  if (sym->name[0] == '_' && !(symbol_map & SM_UNDERSCORE))
	       return 0;

	  if (sym->storage == StaticStorage)
	       type |= SM_STATIC;
	  if (sym->arity == -1 && sym->storage != AutoStorage)
	       type |= SM_DATA;
	  else if (sym->arity >= 0)
	       type |= SM_FUNCTIONS;
     } else if (sym->type == SymToken) {
	  if (sym->token_type == TYPE && sym->source)
	       type |= SM_TYPEDEF;
	  else
	       return 0;
     }
     return (symbol_map & type) == type;
}

void
xalloc_die(void)
{
     error(1, ENOMEM, _("Exiting"));
}

void
init()
{
     if (level_indent[0] == NULL) 
	  level_indent[0] = "    "; /* 4 spaces */
     if (level_indent[1] == NULL)
	  level_indent[1] = level_indent[0];
     if (level_end[0] == NULL)
	  level_end[0] = "";
     if (level_end[1] == NULL)
	  level_end[1] = "";
     
     init_lex(debug > 1);
     init_parse();
}

int
main(int argc, char **argv)
{
     int index;

     setlocale(LC_ALL, "");
     bindtextdomain(PACKAGE, LOCALEDIR);
     textdomain(PACKAGE);
     
     register_output("gnu", gnu_output_handler, NULL);
     register_output("posix", posix_output_handler, NULL);

     symbol_map = SM_FUNCTIONS|SM_STATIC;

     if (getenv ("POSIXLY_CORRECT")) {
	  if (select_output_driver("posix"))
	       error(0, _("%s: No such output driver"), "posix");
	  output_init();
     }
     
     sourcerc(&argc, &argv);
     if (argp_parse(&argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
	  exit(1);

     if (print_option == 0)
	  print_option = PRINT_TREE;

     init();

     if (arglist) 
	  /* See comment to cleanup_processor */
	  for (arglist = CAR(arglist); arglist; arglist = CDR(arglist)) {
	       char *s = (char*)CAR(arglist);
	       if (s[0] == '-')
		    pp_option(s);
	       else if (source(s) == 0)
		    yyparse();
	  }

     argc -= index;
     argv += index;

     while (argc--) {
	  if (source(*argv++) == 0)
	       yyparse();
     }

     if (input_file_count == 0)
	     error(1, 0, _("no input files"));

     cleanup();

     output();
     return 0;
}







