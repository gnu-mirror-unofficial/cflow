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
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA.

AC_PREREQ(2.59)
AC_INIT([GNU cflow], [0.2.3], [bug-cflow@gnu.org])
AC_CONFIG_SRCDIR([src/cflow.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([config.h])
gl_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX
AC_PROG_RANLIB

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h locale.h])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([memmove memset strdup strerror strtol setlocale])

cflow_GNULIB
MU_DEBUG_MODE

AH_TEMPLATE([PROGRAM_INVOCATION_NAME_DECLARED],
            [Is program_invocation_name declared in the system headers.])

# This variable should have been set in m4/argp.m4
if test "$ac_cv_have_decl_program_invocation_name" = no; then
	AC_LIBOBJ([pin])
	AC_DEFINE([HAVE_DECL_PROGRAM_INVOCATION_NAME],1)
	AC_DEFINE_UNQUOTED([PROGRAM_INVOCATION_NAME_DECLARED],0)
else
	AC_DEFINE_UNQUOTED([PROGRAM_INVOCATION_NAME_DECLARED],1)
fi

AH_BOTTOM([
#if !PROGRAM_INVOCATION_NAME_DECLARED
extern char *program_invocation_short_name;
extern char *program_invocation_name;
#endif
/* program_name is used by lib/error.c */
#define program_name program_invocation_name
#include <errno.h>
])

### Check for Emacs site-lisp directory
AM_PATH_LISPDIR

if test "$EMACS" != "no"; then
	lisp_LISP='$(LISPSRC)'
fi
AC_SUBST(lisp_LISP)

# Gettext.
AM_GNU_GETTEXT([external], [need-ngettext])
AM_GNU_GETTEXT_VERSION(0.14.1)

### Initialize the test suite.
AC_CONFIG_TESTDIR(tests)
AC_CONFIG_FILES([tests/Makefile tests/atlocal])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_CONFIG_FILES([
 Makefile
 lib/Makefile
 src/Makefile
 elisp/Makefile
 po/Makefile.in 
 doc/Makefile])
AC_OUTPUT
