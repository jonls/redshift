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

# Checks for libraries.
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

PKG_CHECK_MODULES([DRM], [libdrm], [have_drm=yes], [have_drm=no])

PKG_CHECK_MODULES([X11], [x11], [have_x11=yes], [have_x11=no])
PKG_CHECK_MODULES([XF86VM], [xxf86vm], [have_xf86vm=yes], [have_xf86vm=no])
PKG_CHECK_MODULES([XCB], [xcb], [have_xcb=yes], [have_xcb=no])
PKG_CHECK_MODULES([XCB_RANDR], [xcb-randr],
	[have_xcb_randr=yes], [have_xcb_randr=no])

PKG_CHECK_MODULES([GLIB], [glib-2.0 gobject-2.0], [have_glib=yes], [have_glib=no])
PKG_CHECK_MODULES([GEOCLUE], [geoclue], [have_geoclue=yes], [have_geoclue=no])

AC_CHECK_HEADER([windows.h], [have_windows_h=yes], [have_windows_h=no])

AC_CHECK_HEADER([CoreGraphics/CGDirectDisplay.h], [have_cgdirectdisplay_h=yes], [have_cgdirectdisplay_h=no])

# Check for Python
AM_PATH_PYTHON([3.2], [have_python=yes], [have_python=no])

# Check DRM method
AC_MSG_CHECKING([whether to enable DRM method])
AC_ARG_ENABLE([drm], [AC_HELP_STRING([--enable-drm],
	[enable DRM method])],
	[enable_drm=$enableval],[enable_drm=maybe])
AS_IF([test "x$enable_drm" != xno], [
	AS_IF([test $have_drm = yes], [
		AC_DEFINE([ENABLE_DRM], 1,
			[Define to 1 to enable DRM method])
		AC_MSG_RESULT([yes])
		enable_drm=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_drm" = xyes], [
			AC_MSG_ERROR([missing dependencies for DRM method])
		])
		enable_drm=no
	])
], [
	AC_MSG_RESULT([no])
	enable_drm=no
])
AM_CONDITIONAL([ENABLE_DRM], [test "x$enable_drm" = xyes])

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

# Check for fake Quartz
AC_MSG_CHECKING([whether to enable fake Quartz])
AC_ARG_ENABLE([fakequartz], [AC_HELP_STRING([--enable-fakequartz],
	[enable fake Quartz])],
        [enable_fakequartz=$enableval],[enable_fakequartz=no])
AS_IF([test "x$enable_fakequartz" != xno], [
	AC_DEFINE([FAKE_QUARTZ], 1,
		[Define to 1 to enable Quartz method])
        AC_MSG_RESULT([yes])
	enable_fakequartz=yes
], [
        AC_MSG_RESULT([no])
])
AM_CONDITIONAL([FAKE_QUARTZ], [test "x$enable_fakequartz" != xno])

# Check Quartz method
AC_MSG_CHECKING([whether to enable Quartz method])
AC_ARG_ENABLE([quartz], [AC_HELP_STRING([--enable-quartz],
	[enable Quartz method])],
	[enable_quartz=$enableval],[enable_quartz=maybe])
AS_IF([test "x$enable_quartz" != xno], [
	AS_IF([test $have_cgdirectdisplay_h = yes -o $enable_fakequartz = yes], [
		AC_DEFINE([ENABLE_QUARTZ], 1,
			[Define to 1 to enable Quartz method])
		AC_MSG_RESULT([yes])
		enable_quartz=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_quartz" = xyes], [
			AC_MSG_ERROR([missing CoreGraphics framework for Quartz method])
		])
		enable_quartz=no
	])
], [
	AC_MSG_RESULT([no])
	enable_quartz=no
])
AM_CONDITIONAL([ENABLE_QUARTZ], [test "x$enable_quartz" = xyes])

# Check for fake Windows GDI
AC_MSG_CHECKING([whether to enable fake WinGDI])
AC_ARG_ENABLE([fakegdi], [AC_HELP_STRING([--enable-fakegdi],
	[enable fake WinGDI])],
        [enable_fakegdi=$enableval],[enable_fakegdi=no])
AS_IF([test "x$enable_fakegdi" != xno], [
	AC_DEFINE([FAKE_W32GDI], 1,
		[Define to 1 to enable WinGDI method])
        AC_MSG_RESULT([yes])
	enable_fakegdi=yes
], [
        AC_MSG_RESULT([no])
])
AM_CONDITIONAL([FAKE_W32GDI], [test "x$enable_fakegdi" != xno])

# Check Windows GDI method 
AC_MSG_CHECKING([whether to enable WinGDI method])
AC_ARG_ENABLE([wingdi], [AC_HELP_STRING([--enable-wingdi],
	[enable WinGDI method])],
	[enable_wingdi=$enableval],[enable_wingdi=maybe])
AS_IF([test "x$enable_wingdi" != xno], [
	AS_IF([test $have_windows_h = yes -o $enable_fakegdi = yes], [
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

    Adjustment methods:
    DRM:		${enable_drm}
    RANDR:		${enable_randr}
    VidMode:		${enable_vidmode}
    WinGDI:		${enable_wingdi}
    Quartz:		${enable_quartz}

    Location providers:
    Geoclue:		${enable_geoclue}

    GUI:		${enable_gui}
    Ubuntu icons:	${enable_ubuntu}
    systemd units:	${enable_systemd} ${systemduserunitdir}
"
