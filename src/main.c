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
#include "version.h"

void say_and_die(char *);
void init();

#define OPTSTR "hVLvSCdxtH:p:"

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
    0,
};
#else
#define getopt_long(argc, argv, optstr, long_opts, iptr) \
 getopt(argc, argv, optstr)
#endif

char *progname;
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

int
main(argc, argv)
    int argc;
    char **argv;
{
    int c, i, num;

    progname = argv[0];
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
	case 'H':
	    set_hash_size(atoi(optarg));
	case 'p':
	    num = atoi(optarg);
	    if (num > 0)
		token_stack_length = num;
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
    init_lex();
    init_parse();
    init_hash();
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
	return stat & 255;
}
