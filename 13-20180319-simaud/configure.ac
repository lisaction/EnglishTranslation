#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Simau], [1.0], [wanglin1993@iie.ac.cn])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
# set dir for m4
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE
# for libtool
LT_PREREQ([2.4.6])
LT_INIT()
AC_SUBST([LIBTOOL_DEPS])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
