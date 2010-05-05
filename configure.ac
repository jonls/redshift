#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([redshift], [1.3], [https://bugs.launchpad.net/redshift])
AC_CONFIG_SRCDIR([src/redshift.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([dist-bzip2])

# Checks for programs.
AC_PROG_CC_C99

# Checks for libraries.
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

PKG_CHECK_MODULES([X11], [x11], [have_x11=yes], [have_x11=no])
PKG_CHECK_MODULES([XF86VM], [xxf86vm], [have_xf86vm=yes], [have_xf86vm=no])
PKG_CHECK_MODULES([XCB], [xcb], [have_xcb=yes], [have_xcb=no])
PKG_CHECK_MODULES([XCB_RANDR], [xcb-randr],
	[have_xcb_randr=yes], [have_xcb_randr=no])

# Check for Python
AM_PATH_PYTHON([2.6], [have_python=yes], [have_python=no])

# Check RANDR method
AC_MSG_CHECKING([whether to enable RANDR method])
AC_ARG_ENABLE([randr], [AC_HELP_STRING([--enable-randr],
	[enable RANDR method])],
	[enable_randr=$enableval],[enable_randr=maybe])
AS_IF([test "x$enable_randr" != xno], [
	AS_IF([test $have_xcb = yes -a $have_xcb_randr = yes], [
		AC_DEFINE([ENABLE_RANDR], 1,
			[Define to 1 to enable RANDR method])
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_randr" = xyes], [
			AC_MSG_ERROR([missing dependencies for RANDR method])
		])
		enable_randr=no
	])
], [
	AC_MSG_RESULT([no])
])
AM_CONDITIONAL([ENABLE_RANDR], [test "x$enable_randr" != xno])

# Check VidMode method 
AC_MSG_CHECKING([whether to enable VidMode method])
AC_ARG_ENABLE([vidmode], [AC_HELP_STRING([--enable-vidmode],
	[enable VidMode method])],
	[enable_vidmode=$enableval],[enable_vidmode=maybe])
AS_IF([test "x$enable_vidmode" != xno], [
	AS_IF([test $have_x11 = yes -a $have_xf86vm = yes], [
		AC_DEFINE([ENABLE_VIDMODE], 1,
			[Define to 1 to enable VidMode method])
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_vidmode" = xyes], [
			AC_MSG_ERROR([missing dependencies for VidMode method])
		])
		enable_vidmode=no
	])
], [
	AC_MSG_RESULT([no])
])
AM_CONDITIONAL([ENABLE_VIDMODE], [test "x$enable_vidmode" != xno])

# Check that at least one method is enabled
AS_IF([test "x$enable_randr" = xno -a "x$enable_vidmode" = xno], [
	AC_MSG_ERROR([either RANDR or VidMode must be enabled])
])

# Check for GTK+ status icon
AC_MSG_CHECKING([whether to enable GTK status icon])
AC_ARG_ENABLE([gtk], [AC_HELP_STRING([--enable-gtk],
	[enable GTK status icon])],
	[enable_gtk=$enableval],[enable_gtk=maybe])
AS_IF([test "x$enable_gtk" != xno], [
	AS_IF([test $have_python = yes], [
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_gtk" = xyes], [
			AC_MSG_ERROR([status icon script requires Python 2.6])
		])
		enable_gtk=no
	])
], [
	AC_MSG_RESULT([no])
])
AM_CONDITIONAL([ENABLE_GTK], [test "x$enable_gtk" != xno])

# Checks for header files.
AC_CHECK_HEADERS([locale.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT16_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_SEARCH_LIBS([clock_gettime], [rt])
AC_SEARCH_LIBS([floor], [m])
AC_CHECK_FUNCS([setlocale strchr floor pow clock_gettime])

AC_CONFIG_FILES([
	Makefile
	po/Makefile.in
	src/Makefile
	src/gtk-redshift/Makefile
])
AC_OUTPUT


echo "
 $PACKAGE_NAME $VERSION

    prefix:		${prefix}
    compiler:		${CC}
    cflags:		${CFLAGS}
    ldflags:		${LDFLAGS}
"
