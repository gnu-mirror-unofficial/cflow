/* $Id$ 
 *  cflow
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

char usage[]= "\
usage: cflow [options] file [files]\n\
\n\
The options are:\n\
    -v, --verbose               be verbose on output\n\
    -S, -ignore-indentation     don't rely on indentation\n\
    -d, --defines               record defines\n\
    -x, --xref                  produce cross-reference listing only\n\
    -t, --typedefs              record typedefs\n\
    -H VALUE,\n\
    --hashsize VALUE            set the symbol hash table size to VALUE\n\
                                (default is 509)\n\
    -o FILENAME,\n\
    --output FILENAME           set output file name (default stdout)\n\
    -p VALUE,\n\
    --pushdown VALUE            set initial token stack size to VALUE\n\
    -s SYM:TYPE,\n\
    --symbol SYM:TYPE           make cflow believe the symbol SYM is\n\
                                of type TYPE. Valid types are:\n\
                                    keyword (or kw),\n\
                                    modifier,\n\
                                    identifier,\n\
                                    type,\n\
                                    wrapper\n\
                                Any unambiguous abbreviation of the above is\n\
                                also accepted\n\
    -a, --ansi                  Assume input to be written in ANSI C\n\
    -g, --globals-only          Record only global symbols\n\
    -l, --print-level           Print nesting level along with the call tree\n\
    -m NAME,\n\
    --main NAME                 Assume main function to be called NAME.\n\
    -T, --tree                  Draw tree\n\
    -i STRING,\n\
    --level-indent STRING       Use STRING when indenting to each new level\n\
    -P OPT,\n\
    --print OPT                 Set printing option to OPT. Valid OPT\n\
                                values are:\n\
                                    xref (or cross-ref),\n\
                                    tree\n\
                                Any unambiguous abbreviation of the above is\n\
                                also accepted\n\
\n\
   When given any one of the following options cflow prints corresponding\n\
information and exits successfully ignoring any other command line arguments:\n\
    -h                          Print this usage summary\n\
    -V                          Print version information\n\
    -L                          Print licence information\n\
";

char version[] =
"cflow $Revision$($Date$)\n"
"Compilation options:\n"
#ifdef DEBUG
"    DEBUG\n"
#endif
#ifdef GNU_STYLE_OPTIONS
"    GNU_STYLE_OPTIONS\n"
#endif
"\n";

char licence[] = "\
cflow $Revision$($Date$)\n\
    Copyright (C) 1997 Gray\n\
\n\
    This program is free software; you can redistribute it and/or modify\n\
    it under the terms of the GNU General Public License as published by\n\
    the Free Software Foundation; either version 2 of the License, or\n\
    (at your option) any later version.\n\
\n\
    This program is distributed in the hope that it will be useful,\n\
    but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
    GNU General Public License for more details.\n\
\n\
    You should have received a copy of the GNU General Public License\n\
    along with this program; if not, write to the Free Software\n\
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\
"; 
