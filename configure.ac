#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([itools], [1.0], [https://github.com/arabeyes-org/ITL-programs/issues/new])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([ipraytime.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
#AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([itl], [getNorthQibla])
AC_CHECK_LIB([m], [pow])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_CHECK_FUNCS([strcasecmp])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
