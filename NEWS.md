News
====

v1.11 (2016-01-02)
------------------
- Add option `preserve` for gamma adjustment methods (`randr`, `vidmode`,
  `quartz`, `w32gdi`) to apply redness on top of current gamma correction.
- Fix #158: Add redshift.desktop file to resolve an issue where Geoclue2
  would not allow redshift to obtain the current location (Laurent Bigonville)
- Fix #263: Make sure that the child process is terminated when redshift-gtk
  exits.
- Fix #284: A sample configuation file has been added to the distribution
  tarball.
- Fix warning message in redshift-gtk that is some cases caused redshift-gtk
  to fail (#271) (Christian Stadelmann, Javier Cantero)
- Fix #174: Use nanosleep() for sleeping to avoid high CPU load on platforms
  (NetBSD, ...) with limitations in usleep() (Piotr Meyer)
- Various updates to man page and translations.


v1.10 (2015-01-04)
------------------
* Fix #80: Add Geoclue2 location provider.
* Add CoreLocation (OSX) location provider and Quartz (OSX) gamma
  adjustment method.
* Add hooks for user actions on period switch.
* Be less verbose when color values/period did not change.
* Add config setting to set gamma separately for day/night.
* Add support for custom transition start and end elevation (Mattias
  Andrée).
* redshift-gtk: Show errors from child process in a dialog.
* Fix #95: Add AppData file for package managers.
* Use gettimeofday if POSIX timers not available (add support for
  OSX).
* Fix #41: Do not jump to 0 % or 100 % when changing direction of
  transition (Mattias Andrée).
* redshift-gtk: Relay USR1 signal to redshift process.
* redshift-gtk: Notify desktop about startup completion.
* Fix: systemd unit files were built from the wrong source.
* Fix #90: Print N/S and E/W in the location (Mattias Andrée).
* Fix #112: redshift-gtk: Do not buffer lines from child indefinitely.
* Fix #105: Limit decimals in displayed location to two.
* Update dependencies listed in HACKING.md (emilf, Kees Hink).
* Fix: Make desktop file translatable.
* Add Travis CI build script.

v1.9.1 (2014-04-20)
-------------------
* Fix: Do not distribute redshift-gtk, only redshift-gtk.in.
* Fix: Geoclue support should pull in Glib as dependency.
* geoclue: Fix segfault when error is NULL (Mattias Andrée).
* geoclue: Set DISPLAY=:0 to work around issue when outside X
  (Mattias Andrée).
* redshift-gtk: Fix crash when toggling state using the status icon.
* redshift-gtk: Fix line splitting logic (Maks Verver).

v1.9 (2014-04-06)
-----------------
* Use improved color scheme provided by Ingo Thies.
* Add drm driver which will apply adjustments on linux consoles
  (Mattias Andrée).
* Remove deprecated GNOME clock location provider.
* Set proc title for redshift-gtk (Linux/BSD) (Philipp Hagemeister).
* Show current temperature, location and status in GUI.
* Add systemd user unit files so that redshift can be used with
  systemd as a session manager (Henry de Valence).
* Use checkbox to toggle Redshift in GUI (Mattias Andrée).
* Gamma correction is applied after brightness and temperature
  (Mattias Andrée).
* Use XDG Base Directory Specification when looking for configuration
  file (Mattias Andrée).
* Load config from %LOCALAPPDATA%\redshift.conf on Windows (TingPing).
* Add RPM spec for Fedora in contrib.
* redshift-gtk has been ported to Python3 and new PyGObject bindings
  for Python.

v1.8 (2013-10-21)
-----------------
* IMPORTANT: gtk-redshift has changed name to redshift-gtk.
* Fix crash when starting geoclue provider. (Thanks to Maks Verver)
* Fix slight flicker int gamme ramp values (Sean Hildebrand)
* Add redshift-gtk option to suspend for a short time period (Jendrik Seipp).
* Add print mode (prints parameters and exits) by Vincent Breitmoser.
* Set buffering on stdout and stderr to line-buffered.
* Allow separate brightness for day and night (Olivier Fabre and Jeremy Erickson).
* Fix various crashes/bugs/typos (Benjamin Kerensa and others)

v1.7 (2011-07-04)
-----------------
* Add Geoclue location provider by Mathieu Trudel-Lapierre.
* Allow brightness to be adjusted (-b).
* Provide option to set color temperature directly (Joe Hillenbrand).
* Add option to show program version (-V).
* Add configure.ac option to install ubuntu icons. They will no longer be
   installed by default (Francesco Marella).
* config: Look in %userprofile%/.config/redshift.conf on windows platform.
* Fix: w32gdi: Obtain a new DC handle on every adjustment. This fixes a bug
   where redshift stops updating the screen.

v1.6 (2010-10-18)
-----------------
* Support for optional configuration file (fixes #590722).
* Add man page for redshift written by Andrew Starr-Bochicchio (fixes #582196).
* Explain in help output that 6500K is the neutral color temperature
  (fixes #627113).
* Fix: Handle multiple instances of the GNOME clock applet; contributed by
  Francesco Marella (fixes #610860).
* Fix: Redshift crashes when VidMode fails (fixes #657451).
* Fix: Toggle menu item should not be of class gtk.ImageMenuItem
  (fixes #620355).
* New translations and translation updates: Lithuanian (Aurimas Fišeras);
  Brazilian Portuguese (Matteus Sthefano Leite da Silva);
  Finnish (Jani Välimaa); Italian (Simone Sandri); French (Emilien Klein);
  Russian (Anton Chernyshov).

v1.5 (2010-08-18)
-----------------
* New ubuntu-mono-dark icons that fit better with the color guidelines.
  Contributed by aleth.
* Improve GNOME location provider (patch by Gabriel de Perthuis).
* Application launcher and autostart feature contributed by Francesco Marella.
* Translation updates: Basque (Ibai Oihanguren); Chinese (Jonathan Lumb);
  French (Hangman, XioNoX); German (Jan-Christoph Borchardt); Hebrew
  (dotancohen); Spanish (Fernando Ossandon).

v1.4.1 (2010-06-15)
-------------------
* Include Ubuntu Mono icons by Joern Konopka.
* Fix: Toggle icon in statusicon.py like appindicator already does.
* Tranlation updates: Spanish (Fernando Ossandon), Russian (Чистый)

v1.4 (2010-06-13)
-----------------
* Command line options for color adjustment methods changed. Procedure for
  setting specific screen (-s) or CRTC (-c) changed. See `redshift -h` for
  more information.
* Automatically obtain the location from the GNOME Clock applet if possible.
* Add application indicator GUI (by Francesco Marella) (fixes #588086).
* Add reset option (-x) that removes any color adjustment applied. Based on
  patch by Dan Helfman (fixes #590777).
* `configure` options for GUI changed; see `configure --help` for more
  information.
* New translations:
  - German (Jan-Christoph Borchardt)
  - Italian (Andrea Amoroso)
  - Czech (clever_fox)
  - Spanish (Fernando Ossandon)
  - Finnish (Ilari Oras)

v1.3 (2010-05-12)
-----------------
* Allow adjusting individual CRTCs when using RANDR. Contributed by
  Alexandros Frantzis.
* Add WinGDI method for gamma adjustments on Windows platform.
* Compile with mingw (tested with cross compiler on ubuntu build system).

v1.2 (2010-02-12)
-----------------
* Native language support: Danish and russian translations included in
  this release. Thanks goes to Gregory Petrosyan for the russian
  translation.

v1.1 (2010-01-14)
-----------------
* Provide a GTK status icon (tray icon) for redshift with the
  gtk-redshift program.

v1.0 (2010-01-09)
-----------------
* Temporarily disable/enable when USR1 signal is received.

v0.4 (2010-01-07)
-----------------
* Restore gamma ramps on program exit.

v0.3 (2009-12-28)
-----------------
* Continuously adjust color temperature. One shot mode can be selected
  with a command line switch.
* Allow selection of X screen to apply adjustments to.

v0.2 (2009-12-23)
-----------------
* Add a different method for setting the gamma ramps. It uses the
  VidMode extension.

v0.1 (2009-11-04)
-----------------
* Initial release.
