#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([redshift], [1.6], [https://bugs.launchpad.net/redshift])
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
PKG_CHECK_MODULES([GEOCLUE], [geoclue], [have_geoclue=yes], [have_geoclue=no])

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
	AS_IF([test "x$have_glib" = "xyes" -a "x$have_gconf" = xyes], [
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

# Check Geoclue location provider
AC_MSG_CHECKING([whether to enable Geoclue location provider])
AC_ARG_ENABLE([geoclue], [AC_HELP_STRING([--enable-geoclue],
	[enable Geoclue location provider])],
	[enable_geoclue=$enableval],[enable_geoclue=maybe])
AS_IF([test "x$enable_geoclue" != xno], [
	AS_IF([test "x$have_geoclue" = xyes], [
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
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_gui" = xyes], [
			AC_MSG_ERROR([GUI status icon script requires Python 2.6])
		])
		enable_gui=no
	])
], [
	AC_MSG_RESULT([no])
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

# Checks for header files.
AC_CHECK_HEADERS([locale.h stdint.h stdlib.h string.h unistd.h signal.h])

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
    Geoclue:		${enable_geoclue}

    GUI:		${enable_gui}
    Ubuntu icons:	${enable_ubuntu}
"
