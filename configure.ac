AC_INIT([hello_world], [0.1])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

AM_SILENT_RULES([yes])

LT_INIT

AC_PROG_CC

PKG_CHECK_MODULES([BACNET],[libbacnet])
PKG_CHECK_MODULES([MODBUS], [libmodbus])

AC_SEARCH_LIBS([pthread_create], [pthread], [],
    AC_MSG_ERROR([*** POSIX threads support required]))

AC_CONFIG_FILES([
Makefile
src/Makefile
scripts/Makefile
])

AC_OUTPUT
