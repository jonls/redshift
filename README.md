
Redshift
========

Redshift adjusts the color temperature of your screen according to
your surroundings. This may help your eyes hurt less if you are
working in front of the screen at night.

![Redshift logo](http://jonls.dk/assets/redshift-icon-256.png)

Run `redshift -h` for help on command line options. You can run the program
as `redshift-gtk` instead of `redshift` for a graphical status icon.

* Website: http://jonls.dk/redshift/
* Project page: https://github.com/jonls/redshift

Build status
------------

[![Build Status](https://travis-ci.org/jonls/redshift.svg?branch=master)](https://travis-ci.org/jonls/redshift)

Building from source
--------------------

See the file [HACKING](HACKING.md) for more details on building from source.

FAQ
---

**Where can I find my coordinates to put in the configuration file?**

There are multiple web sites that provide coordinates for map locations, for
example clicking anywhere on Google Maps will bring up a box with the
coordinates. Remember that longitudes in the western hemisphere (e.g. the
Americas) must be provided to Redshift as negative numbers.

**Why doesn't Redshift work on my Chromebook/Raspberry Pi?**

Certain video drivers do not support adjustable gamma ramps. In some cases
Redshift will fail with an error message, but other drivers silently ignore
adjustments to the gamma ramp.

**Why doesn't Redshift change the backlight when I use the brightness option?**

Redshift has a brightness adjustment setting but it does not work the way most
people might expect. In fact it is a fake brightness adjustment obtained by
manipulating the gamma ramps which means that it does not reduce the backlight
of the screen. Preferable only use it if your normal backlight adjustment is
too coarse-grained.

**Why doesn't Redshift work on Wayland (e.g. Fedora 25)?**

The Wayland protocol does not support Redshift. There is currently no way for
Redshift adjust the color temperature in Wayland.

**Why doesn't Redshift work on Ubuntu with Mir enabled?**

Mir does not support Redshift.

**The redness effect is applied during the day instead of at night. Why?**

This usually happens to users in America when the longitude has been set in the
configuration file to a positive number. Longitudes in the western hemisphere
should be provided as negative numbers (e.g. New York City is at approximately
latitude/longitude 41, -74).

**Why does the redness effect occasionally switch off for a few seconds?**

Redshift uses the gamma ramps of the graphics driver to apply the redness
effect but Redshift cannot block other applications from also changing the
gamma ramps. Some applications (particularly games and video players) will
reset the gamma ramps. After a few seconds Redshift will kick in again. There
is no way for Redshift to prevent this from happening.

**Why does the redness effect continuously flicker?**

You may have multiple instances of Redshift running simultaneously. Make sure
that only one instance is running for the display where you are seeing the
flicker.

**Why doesn't Redshift change the color of the mouse cursor?**

Mouse cursors are usually handled separately by the graphics hardware and is
not affected by gamma ramps. Some graphics drivers can be configured to use
software cursors instead.

**I have an issue with Redshift but it was not mentioned in this FAQ. What
do I do?**

Please go to [the issue tracker](https://github.com/jonls/redshift/issues) and
check if your issue has already been reported. If not, please open a new issue
describing you problem.

Donations
---------

[![Flattr](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/thing/57936/Redshift)
