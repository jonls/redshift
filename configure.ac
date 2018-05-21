#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([redshift], [1.12], [https://github.com/jonls/redshift/issues])
AC_CONFIG_SRCDIR([src/redshift.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects dist-xz])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_LIBTOOL
AC_PROG_OBJC # For macOS support modules
AC_LANG([C])

AC_PROG_INTLTOOL([0.50])

AC_CANONICAL_HOST

# Test host platform
build_windows=no
case "${host_os}" in
	mingw*)
		build_windows=yes
		;;
esac

# Test whether to compile Windows resources
AC_CHECK_TOOL([WINDRES], [windres], [])
AS_IF([test "x$build_windows" = "xyes" -a -n "x$WINDRES"], [
	enable_windows_resource=yes
], [
	enable_windows_resource=no
])
AM_CONDITIONAL([ENABLE_WINDOWS_RESOURCE],
	[test "x$enable_windows_resource" = xyes])


# Test whether Objective C compiler works
AC_MSG_CHECKING([whether Objective C compiler works])
AC_LANG_PUSH([Objective C])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])], [
	AC_MSG_RESULT([yes])
	have_objc_compiler=yes
], [
	AC_MSG_RESULT([no])
	have_objc_compiler=no
])
AC_LANG_POP([Objective C])

# Checks for libraries.
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE=redshift
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Package name for gettext])


PKG_CHECK_MODULES([DRM], [libdrm], [have_drm=yes], [have_drm=no])

PKG_CHECK_MODULES([X11], [x11], [have_x11=yes], [have_x11=no])
PKG_CHECK_MODULES([XF86VM], [xxf86vm], [have_xf86vm=yes], [have_xf86vm=no])
PKG_CHECK_MODULES([XCB], [xcb], [have_xcb=yes], [have_xcb=no])
PKG_CHECK_MODULES([XCB_RANDR], [xcb-randr],
	[have_xcb_randr=yes], [have_xcb_randr=no])

PKG_CHECK_MODULES([GLIB], [glib-2.0 gobject-2.0], [have_glib=yes], [have_glib=no])
PKG_CHECK_MODULES([GEOCLUE2], [glib-2.0 gio-2.0 >= 2.26], [have_geoclue2=yes], [have_geoclue2=no])

# macOS headers
AC_CHECK_HEADER([ApplicationServices/ApplicationServices.h], [have_appserv_h=yes], [have_appserv_h=no])

# CoreLocation.h is an Objective C header. Only test if
# Objective C compiler works. AC_CHECK_HEADER does not
# appear to work if the Ojective C compiler is not
# available so we need a custom test.
AC_MSG_CHECKING([whether CoreLocation/CoreLocation.h is usable])
AS_IF([test "x$have_objc_compiler" = xyes], [
	AC_LANG_PUSH([Objective C])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#import <CoreLocation/CoreLocation.h>]],[[]])], [
		AC_MSG_RESULT([yes])
		have_corelocation_h=yes
	], [
		AC_MSG_RESULT([no])
		have_corelocation_h=no
	])
	AC_LANG_POP([Objective C])
], [
	AC_MSG_RESULT([no Objective C compiler available])
	have_corelocation_h=no
])

# Windows header
AC_CHECK_HEADER([windows.h], [have_windows_h=yes], [have_windows_h=no])

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

# Check Quartz (macOS) method
AC_MSG_CHECKING([whether to enable Quartz method])
AC_ARG_ENABLE([quartz], [AC_HELP_STRING([--enable-quartz],
	[enable Quartz (macOS) method])],
	[enable_quartz=$enableval],[enable_quartz=maybe])
AS_IF([test "x$enable_quartz" != xno], [
	AS_IF([test $have_appserv_h = yes], [
		QUARTZ_CFLAGS=""
		QUARTZ_LIBS="-framework ApplicationServices"
		AC_DEFINE([ENABLE_QUARTZ], 1,
			[Define to 1 to enable Quartz method])
		AC_MSG_RESULT([yes])
		enable_quartz=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_quartz" = xyes], [
			AC_MSG_ERROR([missing Quartz headers])
		])
		enable_quartz=no
	])
], [
	AC_MSG_RESULT([no])
	enable_quartz=no
])
AM_CONDITIONAL([ENABLE_QUARTZ], [test "x$enable_quartz" = xyes])
AC_SUBST([QUARTZ_CFLAGS])
AC_SUBST([QUARTZ_LIBS])

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


# Check Geoclue2 location provider
AC_MSG_CHECKING([whether to enable Geoclue2 location provider])
AC_ARG_ENABLE([geoclue2], [AC_HELP_STRING([--enable-geoclue2],
	[enable Geoclue2 location provider])],
	[enable_geoclue2=$enableval],[enable_geoclue2=maybe])
AS_IF([test "x$enable_geoclue2" != xno], [
	AS_IF([test "x$have_geoclue2" = xyes], [
		AC_DEFINE([ENABLE_GEOCLUE2], 1,
			[Define to 1 to enable Geoclue2 location provider])
		AC_MSG_RESULT([yes])
		enable_geoclue2=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_geoclue2" = xyes], [
			AC_MSG_ERROR([missing dependencies for Geoclue2 location provider])
		])
		enable_geoclue2=no
	])
], [
	AC_MSG_RESULT([no])
	enable_geoclue2=no
])
AM_CONDITIONAL([ENABLE_GEOCLUE2], [test "x$enable_geoclue2" = xyes])

# Check CoreLocation (macOS) provider
AC_MSG_CHECKING([whether to enable CoreLocation method])
AC_ARG_ENABLE([corelocation], [AC_HELP_STRING([--enable-corelocation],
	[enable CoreLocation (macOS) provider])],
	[enable_corelocation=$enableval],[enable_corelocation=maybe])
AS_IF([test "x$enable_corelocation" != xno], [
	AS_IF([test "x$have_corelocation_h" = xyes], [
		CORELOCATION_CFLAGS=""
		CORELOCATION_LIBS="-framework Foundation -framework Cocoa -framework CoreLocation"
		AC_DEFINE([ENABLE_CORELOCATION], 1,
			[Define to 1 to enable CoreLocation provider])
		AC_MSG_RESULT([yes])
		enable_corelocation=yes
	], [
		AC_MSG_RESULT([missing dependencies])
		AS_IF([test "x$enable_corelocation" = xyes], [
			AC_MSG_ERROR([missing CoreLocation headers])
		])
		enable_corelocation=no
	])
], [
	AC_MSG_RESULT([no])
	enable_corelocation=no
])
AM_CONDITIONAL([ENABLE_CORELOCATION], [test "x$enable_corelocation" = xyes])
AC_SUBST([CORELOCATION_CFLAGS])
AC_SUBST([CORELOCATION_LIBS])


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


# Check for AppArmor
AC_MSG_CHECKING([whether to enable AppArmor profile])
AC_ARG_ENABLE([apparmor], [AC_HELP_STRING([--enable-apparmor],
	[enable AppArmor profile])],
        [enable_apparmor=$enableval],[enable_apparmor=no])
AS_IF([test "x$enable_apparmor" != xno], [
        AC_MSG_RESULT([yes])
	enable_apparmor=yes
], [
        AC_MSG_RESULT([no])
	enable_apparmor=no
])
AM_CONDITIONAL([ENABLE_APPARMOR], [test "x$enable_apparmor" != xno])


# Checks for header files.
AC_CHECK_HEADERS([locale.h stdint.h stdlib.h string.h unistd.h signal.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT16_T

# Checks for library functions.
AC_SEARCH_LIBS([clock_gettime], [rt])
AC_SEARCH_LIBS([floor], [m])
AC_CHECK_FUNCS([setlocale strchr floor pow])

AC_CONFIG_FILES([
	Makefile
	po/Makefile.in
	src/Makefile
	src/redshift-gtk/Makefile
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
    Quartz (macOS):	${enable_quartz}
    WinGDI (Windows):	${enable_wingdi}

    Location providers:
    Geoclue2:			${enable_geoclue2}
    CoreLocation (macOS):	${enable_corelocation}

    GUI:		${enable_gui}
    Ubuntu icons:	${enable_ubuntu}
    systemd units:	${enable_systemd} ${systemduserunitdir}
    AppArmor profile:   ${enable_apparmor}
"
