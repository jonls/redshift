Name: redshift
Version: 1.12
Release: 1%{dist}
Summary: Adjusts the color temperature of your screen according to time of day
Group: Applications/System
License: GPLv3+
URL: http://jonls.dk/redshift/
Source0: http://launchpad.net/redshift/trunk/%{version}/+download/%{name}-%{version}.tar.xz
BuildRequires: meson
BuildRequires: gettext-devel
BuildRequires: libX11-devel
BuildRequires: libXxf86vm-devel
BuildRequires: libxcb-devel
BuildRequires: glib2-devel
BuildRequires: systemd

%description
Redshift adjusts the color temperature of your screen according to your
surroundings. This may help your eyes hurt less if you are working in
front of the screen at night.

The color temperature is set according to the position of the sun. A
different color temperature is set during night and daytime. During
twilight and early morning, the color temperature transitions smoothly
from night to daytime temperature to allow your eyes to slowly
adapt.

This package provides the base program.

%package -n %{name}-gtk
Summary: GTK integration for Redshift
Group: Applications/System
BuildArch: noarch
BuildRequires: python3-devel >= 3.2
BuildRequires: desktop-file-utils
Requires: python3-gobject
Requires: python3-pyxdg
Requires: %{name} = %{version}-%{release}
Obsoletes: gtk-redshift < %{version}-%{release}

%description -n %{name}-gtk
This package provides GTK integration for Redshift, a screen color
temperature adjustment program.

%prep
%setup -q

%build
# This macro adds "--auto-features=enabled" option and
# tries to enable all features in "meson_options.txt".
%meson -Dquartz=disabled -Dcorelocation=disabled -Dwingdi=disabled -Dsystemduserunitdir=%{_userunitdir}
%meson_build

%install
rm -rf %{buildroot}
%meson_install
# Some files are incorrectly installed into "/usr/lib64/python3.X" on x86_64.
find %{buildroot}/usr/lib64 -name "*.py*" -exec mv {} %{buildroot}/usr/lib/python*/*/*/ \;
%find_lang %{name}
desktop-file-validate %{buildroot}%{_datadir}/applications/redshift.desktop
desktop-file-validate %{buildroot}%{_datadir}/applications/redshift-gtk.desktop

%post -n %{name}-gtk
touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :

%postun -n %{name}-gtk
if [ $1 -eq 0 ] ; then
    touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi

%posttrans -n %{name}-gtk
gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files -f %{name}.lang
%defattr(-,root,root,-)
%doc COPYING NEWS README README-colorramp
%{_bindir}/redshift
%{_mandir}/man1/*
%{_userunitdir}/*

%files -n %{name}-gtk
%defattr(-,root,root,-)
%{_bindir}/redshift-gtk
%{python3_sitelib}/redshift_gtk/
%{_datadir}/icons/hicolor/scalable/apps/redshift*.svg
%{_datadir}/applications/redshift.desktop
%{_datadir}/applications/redshift-gtk.desktop
%{_datadir}/appdata/redshift-gtk.appdata.xml

%changelog
* Sun Jul 8 2018 Ben van der Harg <benvanderharg@yandex.com> - 1.12.1
- Update to 1.12
* Sat Jan 2 2016 Jon Lund Steffensen <jonlst@gmail.com> - 1.11-1
- Update to 1.11

* Sun Jan 4 2015 Jon Lund Steffensen <jonlst@gmail.com> - 1.10-1
- Update to 1.10

* Sun Apr 6 2014 Jon Lund Steffensen <jonlst@gmail.com> - 1.9-1
- Update to 1.9

* Wed Dec 11 2013 Jon Lund Steffensen <jonlst@gmail.com> - 1.8-1
- Update to 1.8

* Sun May 12 2013 Milos Komarcevic <kmilos@gmail.com> - 1.7-5
- Run autoreconf to support aarch64 (#926436)
- Backport fix for geoclue client check (#954014)

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.7-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Sat Jul 21 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.7-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Sat Jan 14 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.7-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Sat Jul 9 2011 Milos Komarcevic <kmilos@gmail.com> - 1.7-1
- Update to 1.7
- Add geoclue BuildRequires
- Change default geoclue provider from Ubuntu GeoIP to Hostip
- Remove manual Ubuntu icons uninstall

* Mon Feb 28 2011 Milos Komarcevic <kmilos@gmail.com> - 1.6-3
- Fix for clock applet detection (#661145)
- Require pyxdg explicitly (#675804)

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.6-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Sat Nov 13 2010 Milos Komarcevic <kmilos@gmail.com> - 1.6-1
- Update to 1.6
- Remove BuildRoot tag and clean section

* Thu Aug 26 2010 Milos Komarcevic <kmilos@gmail.com> - 1.5-1
- Update to 1.5
- Install desktop file

* Mon Jul 26 2010 Milos Komarcevic <kmilos@gmail.com> - 1.4.1-2
- License updated to GPLv3+
- Added python macros to enable building on F12 and EPEL5
- Specific python version BR
- Subpackage requires full base package version
- Increased build log verbosity
- Preserve timestamps on install

* Thu Jun 17 2010 Milos Komarcevic <kmilos@gmail.com> - 1.4.1-1
- Update to 1.4.1

* Thu Jun 10 2010 Milos Komarcevic <kmilos@gmail.com> - 1.3-1
- Initial packaging
