/* $Id$
 */

char usage[]= "\
usage: cflow \n\
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
