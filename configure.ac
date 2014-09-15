#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([redshift], [1.9.1], [https://github.com/jonls/redshift/issues])
AC_CONFIG_SRCDIR([src/redshift.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects dist-xz])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_INTLTOOL([0.50])

# Checks for libraries.
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE=redshift
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Package name for gettext])


PKG_CHECK_MODULES([GLIB], [glib-2.0 gobject-2.0], [have_glib=yes], [have_glib=no])
PKG_CHECK_MODULES([GEOCLUE], [geoclue], [have_geoclue=yes], [have_geoclue=no])

AC_CHECK_HEADER([windows.h], [have_windows_h=yes], [have_windows_h=no])

# Check for Python
AM_PATH_PYTHON([3.2], [have_python=yes], [have_python=no])


# Check Geoclue location provider
AC_MSG_CHECKING([whether to enable Geoclue location provider])
AC_ARG_ENABLE([geoclue], [AC_HELP_STRING([--enable-geoclue],
	[enable Geoclue location provider])],
	[enable_geoclue=$enableval],[enable_geoclue=maybe])
AS_IF([test "x$enable_geoclue" != xno], [
	AS_IF([test "x$have_geoclue" = xyes -a "x$have_glib" = xyes], [
		AC_DEFINE([ENABLE_GEOCLUE], 1,
			[Define to 1 to enable Geoclue location provider])
		AC_MSG_RESULT([yes])
		enable_geoclue=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_geoclue" = xyes], [
			AC_MSG_ERROR([missing dependencies for Geoclue location provider])
		])
		enable_geoclue=no
	])
], [
	AC_MSG_RESULT([no])
	enable_geoclue=no
])
AM_CONDITIONAL([ENABLE_GEOCLUE], [test "x$enable_geoclue" = xyes])


# Check for GUI status icon
AC_MSG_CHECKING([whether to enable GUI status icon])
AC_ARG_ENABLE([gui], [AC_HELP_STRING([--enable-gui],
	[enable GUI status icon])],
	[enable_gui=$enableval],[enable_gui=maybe])
AS_IF([test "x$enable_gui" != xno], [
	AS_IF([test $have_python = yes], [
		AC_MSG_RESULT([yes])
		enable_gui=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_gui" = xyes], [
			AC_MSG_ERROR([GUI status icon script requires Python])
		])
		enable_gui=no
	])
], [
	AC_MSG_RESULT([no])
	enable_gui=no
])
AM_CONDITIONAL([ENABLE_GUI], [test "x$enable_gui" != xno])

# Check for Ubuntu icons
AC_MSG_CHECKING([whether to enable Ubuntu icons])
AC_ARG_ENABLE([ubuntu], [AC_HELP_STRING([--enable-ubuntu],
	[enable Ubuntu icons])],
        [enable_ubuntu=$enableval],[enable_ubuntu=no])
AS_IF([test "x$enable_ubuntu" != xno], [
        AC_MSG_RESULT([yes])
], [
        AC_MSG_RESULT([no])
])
AM_CONDITIONAL([ENABLE_UBUNTU], [test "x$enable_ubuntu" != xno])


# Check for systemd
PKG_PROG_PKG_CONFIG
AC_MSG_CHECKING([Directory to install systemd user unit files])
AC_ARG_WITH([systemduserunitdir],
            [AS_HELP_STRING([--with-systemduserunitdir=<dir>],
                            [Directory for systemd user unit files])],
            [], [with_systemduserunitdir=$($PKG_CONFIG --variable=systemduserunitdir systemd)])
AS_IF([test -n "$with_systemduserunitdir" -a "x$with_systemduserunitdir" != xno], [
	AC_SUBST([systemduserunitdir], [$with_systemduserunitdir])
	AC_MSG_RESULT([$systemduserunitdir])
	enable_systemd=yes
], [
	AC_MSG_RESULT([not enabled])
	enable_systemd=no
])
AM_CONDITIONAL([ENABLE_SYSTEMD], [test "x$enable_systemd" != xno])


# Checks for header files.
AC_CHECK_HEADERS([libgamma.h locale.h stdint.h stdlib.h string.h unistd.h signal.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT16_T

# Checks for library functions.
AC_SEARCH_LIBS([clock_gettime], [rt])
AC_SEARCH_LIBS([floor], [m])
AC_CHECK_FUNCS([setlocale strchr floor pow clock_gettime])

AC_CONFIG_FILES([
	Makefile
	po/Makefile.in
	src/Makefile
	src/redshift-gtk/Makefile
	contrib/redshift.spec
])
AC_OUTPUT


echo "
 $PACKAGE_NAME $VERSION

    prefix:		${prefix}
    compiler:		${CC}
    cflags:		${CFLAGS}
    ldflags:		${LDFLAGS}

    Location providers:
    Geoclue:		${enable_geoclue}

    GUI:		${enable_gui}
    Ubuntu icons:	${enable_ubuntu}
    systemd units:	${enable_systemd} ${systemduserunitdir}
"
