/* $Id$ */
/*  cflow:
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
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include "cflow.h"
#include "parser.h"
#include "output.h"

#ifndef LOCAL_RC
#define LOCAL_RC ".cflowrc"
#endif

static int xargc;
static char **xargv;
static void parse_rc(char*);
static void expand_args(char*);

/* Process the value of the environment variable CFLOW_OPTIONS
 * and of the rc file.
 * Split the value into words and add them between (*ARGV_PTR)[0] and
 * (*ARGV_PTR[1]) modifying *ARGC_PTR accordingly.
 * NOTE: Since sourcerc() is not meant to take all SH command line processing
 *       burden, only word splitting is performed and no kind of expansion
 *       takes place. 
 */
void
sourcerc(argc_ptr, argv_ptr)
    int *argc_ptr;
    char ***argv_ptr;
{
    char *env, *home;
    
    xargc = 1;
    xargv = malloc(2*sizeof(xargv[0]));

    env = getenv("CFLOW_OPTIONS");
    if (env) {
	expand_args(strdup(env));
    }
    
    home = getenv("HOME");
    if (home) {
	int len = strlen(home);
	char *buf = malloc(len + sizeof(LOCAL_RC)
			   + (home[len-1] != '/') );
	if (!buf)
	    return;
	strcpy(buf, home);
	if (home[len-1] != '/')
	    buf[len++] = '/';
	strcpy(buf+len, LOCAL_RC);
	parse_rc(buf);
	free(buf);
    }
    
    if (xargc > 1) {
	int i;
	char **xp = realloc(xargv, (xargc + *argc_ptr + 1)*(sizeof(xargv[0])));
	if (!xp) {
	    error(0, "not enough core to process rc");
	    free(xargv);
	    return;
	}
	xargv = xp;
	xargv[0] = (*argv_ptr)[0];
	xargc--;
	for (i = 1; i <= *argc_ptr; i++) {
	    xargv[xargc+i] = (*argv_ptr)[i];
	}
	*argc_ptr += xargc;
	*argv_ptr = xargv;
    }
}

/* Parse rc file
 */
void
parse_rc(name)
    char *name;
{
    struct stat st;
    FILE *rcfile;
    int size;
    char *buf;
    
    if (stat(name, &st))
	return;
    buf = malloc(st.st_size+1);
    if (!buf) {
	error(0, "not enough memory to process rc file");
	return;
    }
    rcfile = fopen(name, "r");
    if (!rcfile) {
	error(SYSTEM_ERROR, "cannot open %s", name);
	return;
    }
    size = fread(buf, 1, st.st_size, rcfile);
    buf[size] = 0;
    fclose(rcfile);
    expand_args(buf);
}


void
expand_args(buf)
    char *buf;
{
    char *p, *start;
    char **xp;
    
    p = buf;
    while (*p) {
	while (*p && isspace(*p))
	    p++;
	if (!*p)
	    break;
	start = p;
	while (*p && !isspace(*p))
	    p++;
	if (*p)
	    *p++ = 0;
	xargv[xargc] = start;
	xp = realloc(xargv, (xargc+2)*sizeof(xargv[0]));
	if (!xp) {
	    error(0, "not enough core to process rc");
	    return;
	}
	xargv = xp;
	xargc++;
    }
}
	
