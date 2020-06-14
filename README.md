
Redshift
========

Redshift adjusts the color temperature of your screen according to
your surroundings. This may help your eyes hurt less if you are
working in front of the screen at night.

README versions: [latest](https://github.com/jonls/redshift/blob/master/README.md) | [1.12](https://github.com/jonls/redshift/blob/v1.12/README.md) | [1.11](https://github.com/jonls/redshift/blob/v1.11/README.md)

![Redshift logo](http://jonls.dk/assets/redshift-icon-256.png)

Run `redshift -h` for help on command line options. You can run the program
as `redshift-gtk` instead of `redshift` for a graphical status icon.

* Project page: https://github.com/jonls/redshift

Build status
------------

[![Build Status](https://travis-ci.org/jonls/redshift.svg?branch=master)](https://travis-ci.org/jonls/redshift)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/jonls/redshift?branch=master&svg=true)](https://ci.appveyor.com/project/jonls/redshift)

Technical Details (Gamma Ramps)
-------------------------------

Redshift applies a redness effect to the graphical display. The intensity of the redness can be customized and
scheduled to only be applied at night or to be applied with more intensity at night.

Redshift uses the gamma correction ramps to apply this effect which has traditionally been the only
way of applying a color effect to the display. Note that this is really a hack to work around the absense of a
standardized way of applying color effects, and it is resulting in several issues some of which are explained in
the _FAQ_ section below. As long as Redshift is using gamma ramps, many of these issues are impossible to solve properly
in Redshift.

Since Redshift was first created, many desktop environments have adopted similar functionality as an integrated
component:

- GNOME: [Night Light](https://www.gnome.org/news/2017/03/gnome-3-24-released/attachment/night-light/)
- Plasma: Night Color
- Windows: [Night Light](https://support.microsoft.com/en-us/help/4027563/windows-10-set-your-display-for-night-time)
- macOS: Night Shift

Using the features integrated into the desktop environments avoids many of the issues with gamma ramps which is why
these implementations should generally be favored over Redshift. On the other hand, Redshift may offer some additional
flexibility that is not available in the integrated features.

FAQ
---

### How do I install Redshift?

Use the packages provided by your distribution, e.g. for Ubuntu:
`apt-get install redshift` or `apt-get install redshift-gtk`. For developers,
please see _Building from source_ and _Latest builds from master branch_ below.

### How do I setup a configuration file?

A configuration file is not required but is useful for saving custom
configurations and manually defining the location in case of issues with the
automatic location provider. An example configuration can be found in
[redshift.conf.sample](redshift.conf.sample).

The configuration file should be saved in the following location depending on
the platform:

- Linux/macOS: `~/.config/redshift/redshift.conf` (if the environment variable `XDG_CONFIG_HOME` is undefined) or `${XDG_CONFIG_HOME}/redshift/redshift.conf` (if `XDG_CONFIG_HOME` is defined).
- Windows: Put `redshift.conf` in `%USERPROFILE%\AppData\Local\`
    (aka `%localappdata%`).

### Where can I find my coordinates to put in the configuration file?

There are multiple web sites that provide coordinates for map locations, for
example clicking anywhere on Google Maps will bring up a box with the
coordinates. Remember that longitudes in the western hemisphere (e.g. the
Americas) must be provided to Redshift as negative numbers.

### Why does GeoClue fail with access denied error?

It is possible that the location services have been disabled completely. The
check for this case varies by desktop environment. For example, in GNOME the
location services can be toggled in Settings > Privacy > Location Services.

If this is not the case, it is possible that Redshift has been improperly
installed or not been given the required permissions to obtain location
updates from a system administrator. See
https://github.com/jonls/redshift/issues/318 for further discussion on this
issue.

### Why doesn't Redshift work on my Chromebook/Raspberry Pi?

Certain video drivers do not support adjustable gamma ramps. In some cases
Redshift will fail with an error message, but other drivers silently ignore
adjustments to the gamma ramp.

### Why doesn't Redshift change the backlight when I use the brightness option?

Redshift has a brightness adjustment setting but it does not work the way most
people might expect. In fact it is a fake brightness adjustment obtained by
manipulating the gamma ramps which means that it does not reduce the backlight
of the screen. Preferably only use it if your normal backlight adjustment is
too coarse-grained.

### Why doesn't Redshift work on Wayland (e.g. Fedora 25)?

The Wayland protocol does not support Redshift. There is currently no way for
Redshift to adjust the color temperature in Wayland.

Instead, you can use:

- In GNOME Shell: Settings → Display Settings → Night Light
- In Plasma: System Settings → Display and Monitor → Night Color

### Why doesn't Redshift work on Ubuntu with Mir enabled?

Mir does not support Redshift.

### When running as a systemd service, redshift fails to connect to the graphical environment

You need to export your environment variables when your window manager or
compositor start up. Typically, you want to run this as part of its startup:

    systemctl --user import-environment; systemctl --user start graphical-session.target

See your compositor's (or window manager's) documentation for further details
of setting up the systemd user session.

Related issues: [#753](https://github.com/jonls/redshift/pull/753).

### The redness effect is applied during the day instead of at night. Why?

This usually happens to users in America when the longitude has been set in the
configuration file to a positive number. Longitudes in the western hemisphere
should be provided as negative numbers (e.g. New York City is at approximately
latitude/longitude 41, -74).

### Why does the redness effect occasionally switch off for a few seconds?

Redshift uses the gamma ramps of the graphics driver to apply the redness
effect but Redshift cannot block other applications from also changing the
gamma ramps. Some applications (particularly games and video players) will
reset the gamma ramps. After a few seconds Redshift will kick in again. There
is no way for Redshift to prevent this from happening.

### Why does the redness effect continuously flicker?

You may have multiple instances of Redshift running simultaneously. Make sure
that only one instance is running for the display where you are seeing the
flicker.

### Why doesn't Redshift change the color of the mouse cursor?

Mouse cursors are usually handled separately by the graphics hardware and is
not affected by gamma ramps. Some graphics drivers can be configured to use
software cursors instead.

### I have an issue with Redshift but it was not mentioned in this FAQ. What do I do?

Please go to [the issue tracker](https://github.com/jonls/redshift/issues) and
check if your issue has already been reported. If not, please open a new issue
describing you problem.

Latest builds from master branch
--------------------------------

- [Ubuntu PPA](https://launchpad.net/~dobey/+archive/ubuntu/redshift-daily/+packages) (`sudo add-apt-repository ppa:dobey/redshift-daily`)
- [Windows x86_64](https://ci.appveyor.com/api/projects/jonls/redshift/artifacts/redshift-windows-x86_64.zip?branch=master&job=Environment%3A+arch%3Dx86_64&pr=false)
- [Windows x86](https://ci.appveyor.com/api/projects/jonls/redshift/artifacts/redshift-windows-i686.zip?branch=master&job=Environment%3A+arch%3Di686&pr=false)

Contributing / Building from source
-----------------------------------

See the file [CONTRIBUTING](CONTRIBUTING.md) for more details.
