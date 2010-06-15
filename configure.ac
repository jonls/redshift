#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([redshift], [1.4.1], [https://bugs.launchpad.net/redshift])
AC_CONFIG_SRCDIR([src/redshift.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([dist-bzip2])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

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

PKG_CHECK_MODULES([GLIB], [glib-2.0 gobject-2.0], [have_glib=yes], [have_glib=no])
PKG_CHECK_MODULES([GCONF], [gconf-2.0], [have_gconf=yes], [have_gconf=no])

AC_CHECK_HEADER([windows.h], [have_windows_h=yes], [have_windows_h=no])

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
		enable_randr=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_randr" = xyes], [
			AC_MSG_ERROR([missing dependencies for RANDR method])
		])
		enable_randr=no
	])
], [
	AC_MSG_RESULT([no])
	enable_randr=no
])
AM_CONDITIONAL([ENABLE_RANDR], [test "x$enable_randr" = xyes])

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
		enable_vidmode=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_vidmode" = xyes], [
			AC_MSG_ERROR([missing dependencies for VidMode method])
		])
		enable_vidmode=no
	])
], [
	AC_MSG_RESULT([no])
	enable_vidmode=no
])
AM_CONDITIONAL([ENABLE_VIDMODE], [test "x$enable_vidmode" = xyes])

# Check Windows GDI method 
AC_MSG_CHECKING([whether to enable WinGDI method])
AC_ARG_ENABLE([wingdi], [AC_HELP_STRING([--enable-wingdi],
	[enable WinGDI method])],
	[enable_wingdi=$enableval],[enable_wingdi=maybe])
AS_IF([test "x$enable_wingdi" != xno], [
	AS_IF([test $have_windows_h = yes], [
		AC_DEFINE([ENABLE_WINGDI], 1,
			[Define to 1 to enable WinGDI method])
		AC_MSG_RESULT([yes])
		enable_wingdi=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_wingdi" = xyes], [
			AC_MSG_ERROR([missing Windows API headers for WinGDI method])
		])
		enable_wingdi=no
	])
], [
	AC_MSG_RESULT([no])
	enable_wingdi=no
])
AM_CONDITIONAL([ENABLE_WINGDI], [test "x$enable_wingdi" = xyes])

# Check that at least one method is enabled
AS_IF([test "x$enable_randr" = xno -a "x$enable_vidmode" = xno -a "x$enable_wingdi" = xno], [
	AC_MSG_ERROR([either RANDR, VidMode or WinGDI must be enabled])
])

# Check GNOME Clock location provider
AC_MSG_CHECKING([whether to enable GNOME Clock location provider])
AC_ARG_ENABLE([gnome-clock], [AC_HELP_STRING([--enable-gnome-clock],
	[enable GNOME Clock location provider])],
	[enable_gnome_clock=$enableval],[enable_gnome_clock=maybe])
AS_IF([test "x$enable_gnome_clock" != xno], [
	AS_IF([test $have_glib = yes -a $have_gconf = yes], [
		AC_DEFINE([ENABLE_GNOME_CLOCK], 1,
			[Define to 1 to enable GNOME Clock location provider])
		AC_MSG_RESULT([yes])
		enable_gnome_clock=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_gnome_clock" = xyes], [
			AC_MSG_ERROR([missing dependencies for GNOME Clock location provider])
		])
		enable_gnome_clock=no
	])
], [
	AC_MSG_RESULT([no])
	enable_gnome_clock=no
])
AM_CONDITIONAL([ENABLE_GNOME_CLOCK], [test "x$enable_gnome_clock" = xyes])


# Check for GUI
# There doesn't seem to be an easy way of detecting the presence
# of a python module. For that reason, statusicon is the default GUI.
AC_MSG_CHECKING([which GUI to enable])
AC_ARG_ENABLE([gui], [AC_HELP_STRING([--enable-gui],
	[enable GUI (appindicator, statusicon or none)])],
	[enable_gui=$enableval],[enable_gui=maybe])

# Statusicon GUI
AS_IF([test "x$enable_gui" = xstatusicon -o "x$enable_gui" = xmaybe], [
	AS_IF([test "x$have_python" != xyes], [
		AS_IF([test "x$enable_gui" = xstatusicon], [
			AC_MSG_ERROR([status icon script requires Python 2.6])
		])
		enable_gui=maybe
	], [
		enable_gui=statusicon
	])
])

# Appindicator GUI
AS_IF([test "x$enable_gui" = xappindicator -o "x$enable_gui" = xmaybe], [
	AS_IF([test "x$have_python" != xyes], [
		AS_IF([test "x$enable_gui" = xappindicator], [
			AC_MSG_ERROR([application indicator script requires Python 2.6])
		])
		enable_gui=maybe
	], [
		enable_gui=appindicator
	])
])

# Fall back to no GUI
AS_IF([test "x$enable_gui" != xappindicator -a "x$enable_gui" != xstatusicon], [
	enable_gui=none
])

AC_MSG_RESULT([$enable_gui])

AM_CONDITIONAL([ENABLE_STATUSICON], [test "x$enable_gui" = xstatusicon])
AM_CONDITIONAL([ENABLE_APPINDICATOR], [test "x$enable_gui" = xappindicator])


# Checks for header files.
AC_CHECK_HEADERS([locale.h stdint.h stdlib.h string.h unistd.h sys/signal.h])

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
	src/gtk-redshift/Makefile
])
AC_OUTPUT


echo "
 $PACKAGE_NAME $VERSION

    prefix:		${prefix}
    compiler:		${CC}
    cflags:		${CFLAGS}
    ldflags:		${LDFLAGS}

    Adjustment methods:
    RANDR:		${enable_randr}
    VidMode:		${enable_vidmode}
    WinGDI:		${enable_wingdi}

    Location providers:
    GNOME Clock:	${enable_gnome_clock}

    GUI:		${enable_gui}
"
