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
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([rabbitmq], [amqp_queue_declare])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gethostname inet_ntoa memset select socket])

AC_CONFIG_FILES([Makefile
				 src/AuthServer/Makefile
                 src/Makefile])
AC_OUTPUT
