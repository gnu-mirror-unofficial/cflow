# This file is part of GNU cflow
# Copyright (C) 2005 Free Software Foundation, Inc.
#
# Written by Sergey Poznyakoff
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

INCLUDES = -I$(top_srcdir)/lib -I../ -I../lib

bin_PROGRAMS = cflow
cflow_SOURCES = \
 main.c\
 rc.c\
 parser.c\
 c.l\
 output.c\
 symbol.c\
 cflow.h\
 parser.h\
 gnu.c\
 posix.c

cflow_LDADD=../lib/libcflow.a
AM_LFLAGS=-dvp
EXTRA_DIST=cflow.rc

FLOWCHART_FILES=\
 main.c\
 rc.c\
 parser.c\
 c.c\
 output.c\
 symbol.c\
 gnu.c\
 posix.c

chart.cflow: cflow $(FLOWCHART_FILES) cflow.rc Makefile
	CFLOWRC=$(top_srcdir)/src/cflow.rc \
	 cflow -o$@ -i^s $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	            $(CPPFLAGS) $(FLOWCHART_FILES)