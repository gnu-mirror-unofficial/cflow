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
#include "cflow.h"
#include "parser.h"
#include "obstack1.h"
#include "output.h"

#ifndef LOCAL_RC
#define LOCAL_RC ".cflowrc"
#endif

struct obstack rc_stk;
char *format_str[NUMFORMATS];
FILE *rcfile;
char *rcname;
int line;
char linebuf[512];

static int get_format();
static int nextline();
static void parse_rc(char*);
static void section();
static void find_end();

struct keyword {
    char *name;
    int code;
};

struct keyword kw[] = {
    "call", CALL,
    "direct_tree", DIRECT_TREE,
    "footer", FOOTER,
    "header", HEADER,
    "level_indent", LEVEL_INDENT,
    "recursive_call", RECURSIVE_CALL,
    "recursive_ref", RECURSIVE_REF,
    "reverse_tree", REVERSE_TREE,
    "recursive_bottom", RECURSIVE_BOTTOM,
    "description", DESCRIPTION,
    "branch", BRANCH,
    "nobranch", NOBRANCH,
    "leaf", LEAF,
    0,
};


static int
find_keyword(kwp, str, len)
    struct keyword *kwp;
    char *str;
    int *len;
{
    int i;

    for ( ; kwp->name; kwp++) {
	if (strncmp(str, kwp->name, strlen(kwp->name)) == 0) {
	    if (len)
		*len = strlen(kwp->name);
	    return kwp->code;
	}
    }
    return 0;
}

int
get_format()
{
    return find_keyword(kw, linebuf+1, NULL);
}

int
nextline()
{
    if (fgets(linebuf, sizeof(linebuf), rcfile)) {
	line++;
	return 1;
    }
    return 0;
}

void
sourcerc()
{
    char *home;

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
}

void
parse_rc(name)
    char *name;
{
    rcfile = fopen(name, "r");
    if (!rcfile) {
	return;
    }

    line = 1;
    rcname = name;
    
    obstack_init(&rc_stk);
    while (nextline()) {
	int len = strlen(linebuf);
	if (len == 1)
	    continue;
	linebuf[len-1] = 0;
	if (linebuf[0] == '$') 
	    section();
	else
	    error(0, "%s:%d expected section start", rcname, line);
    }
    fclose(rcfile);
}

enum fmt_type {
    TextFmt=1,
    CurLineFmt,
    NameFmt,
    FileFmt,
    LineFmt,
    RefFmt
};

struct keyword format_var[] = {
    ".", CurLineFmt,
    "name", NameFmt,
    "file", FileFmt,
    "line", LineFmt,
    "ref", RefFmt,
    0
};

int
get_var(ptr, endptr)
    char *ptr;
    char **endptr;
{
    int len;
    int n = find_keyword(format_var, ptr, &len);
    *endptr = ptr;
    if (n) 
	*endptr += len;
    return n;
}

void
section()
{
    int num, var;
    char *ptr, *start;
    
    if ((num = get_format()) == 0) {
	error(0, "%s:%d unknown section name: %s", rcname, line, linebuf+1);
	find_end();
	return;
    }

    while (nextline() && strncmp(linebuf, "$end", 4)) {
	for (ptr = start = linebuf; *ptr; ) {
	    if (*ptr == '$') {
		if (ptr[1] == '$') {
		    obstack_1grow(&rc_stk, '$');
		    ptr += 2;
		} else {
		    if (ptr > start) {
			obstack_1grow(&rc_stk, TextFmt);
			obstack_grow(&rc_stk, start, ptr-start);
		    }
		    obstack_1grow(&rc_stk, 0);
		    var = get_var(ptr+1, &start);
		    obstack_1grow(&rc_stk, var);
		    ptr = start;
		}
	    } else
		ptr++;
	}
	obstack_1grow(&rc_stk, TextFmt);
	obstack_grow(&rc_stk, start, ptr-start);
	obstack_1grow(&rc_stk, 0);
    }
    obstack_1grow(&rc_stk, 0);
    format_str[num] = obstack_finish(&rc_stk);
}
	
void
find_end()
{
    while (nextline() && strncmp(linebuf, "$end", 4))
	;
}
   
void
format(num, sp)
    int num;
    Symbol *sp;
{
    char *p;

    p = format_str[num];
    while (*p) {
	switch (*p++) {
	case TextFmt:
	    printf("%s", p);
	    while (*p++) ;
	    break;
	case CurLineFmt:
	    printf("%d", out_line);
	    break;
	case NameFmt:
	    printf("%s", sp->name);
	    break;
	case FileFmt:
	    printf("%s", sp->v.func.source);
	    break;
	case LineFmt:
	    printf("%d", sp->v.func.def_line);
	    break;
	case RefFmt:
	    printf("%d", sp->active-1);
	    break;
	default:
	    abort();
	}
    }
}
