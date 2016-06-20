#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([dategrep], [0.01], [mario@domgoergen.com])
AC_CONFIG_SRCDIR([dategrep.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99

# Checks for libraries.

AC_CHECK_LIB([m],[pow])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])

# Checks for library functions.
AC_FUNC_FSEEKO
AC_FUNC_MKTIME
AC_CHECK_FUNCS([strerror])

AM_INIT_AUTOMAKE([foreign])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
