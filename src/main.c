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
#include "version.h"

void say_and_die(char *);

#define OPTSTR "hVL"

#ifdef GNU_STYLE_OPTIONS
typedef struct option LONGOPT;
/* Note: Internal variables go first */
LONGOPT longopts[] = {
    "help",    no_argument, 0, 'h',
    "version", no_argument, 0, 'V',
    "licence", no_argument, 0, 'L',
    0
}
#else
#define getopt_long(argc, argv, optstr, long_opts, iptr) \
 getopt(argc, argv, optstr)
#endif


int
main(argc, argv)
    int argc;
    char **argv;
{
    int c;
    
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
	    
	}
    }

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
