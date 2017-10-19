This is a document describing how redshift works. It might be useful
if the normal docs don't answer a question, or when you want to hack
on the code.


Programs
========

redshift
--------

redshift is a program written in C that tries to figure out the user's
location during startup, and then goes into a loop setting the display
gamma according to the time of day every couple seconds or minutes
(details?).

On systems that support signals, it reacts to the SIGUSR1 signal by
switching to day/night immediately, and when receiving SIGINT or
SIGTERM, it restores the screen gamma (to 6500K).

Redshift knows short and long transitions, short transitions being
used at start and when reacting to signals. Short transitions take
about 10 seconds; long transitions about 50 minutes.

Once running, redshift currently doesn't check location providers
again.


redshift-gtk
------------

redshift-gtk is a small program written in Python that shows a status
icon (what is an appindicator versus a GTK status icon?) (does it
change the icon according to internal program state of redshift? 
doesn't seem so) and run an instance of the "redshift" program, and
will send it SIGUSR1 each time the user clicks the icon.


Alternative Features
====================

Redshift interacts with the rest of the system in two ways: reading
the location, and setting the gamma. Both can be done in different
ways, and so for both areas there are configure options to
enable/disable compilation of the various methods. ./configure --help
shows more about what parts of the program that can be conditionally
compiled.

NOTE: some features have to be disabled explicitely, like
--disable-gnome-clock to prevent the gnome-clock code from being built
in.

The two groups of features shall be called: "location providers" and
"adjustment methods".

These are probably not the best names for these things but at least
I've been mostly consistent with the naming throughout the source code
(I hope).

First adjustment methods: There is "randr" which is the preferred
because it has support for multiple outputs per X screen which is lacking
in "vidmode". Both are APIs in the X server that allow for manipulation
of gamma ramps, which is what Redshift uses to change the screen color
temperature. There's also "wingdi" which is for the Windows version,
and "drm" which allows manipulation of gamma ramps in a TTY in Linux.

Then there are location providers: "manual", "geoclue2" and "corelocation".
Some time ago there was only one way to specify the
location which had to be done manually with the argument "-l LAT:LON".
Then later, automatic "location providers" were added and the syntax
had to be changed to "-l PROVIDER:OPTIONS" where OPTIONS are arguments
specific to the provider. But to make people less confused about the
change I decided to still support the "-l LAT:LON" syntax, so if the
PROVIDER is a number, the whole thing is parsed as LAT:LON. You could
run redshift with "-l manual:lat=55:lon=12" and get the same effect as
"-l 55:12".

So there are currently two automatic location providers "gnome-clock"
and "geoclue". From the beginning I was looking for a way to get the
location automatically (from e.g. GPS) and Geoclue seemed like a good
idea, but upon closer investigation it turned out to be horribly
unstable. At this time GNOME had a clock applet which was present by
default (at least in Ubuntu) that allowed the user to set a home town.
This setting was registered in the gconf key
/apps/panel/applets/clock_screen*/prefs/cities.
The idea was to use this information until Geoclue had become more
stable. To me, it always was a hack. Now that the Clock applet has
gone (at least in Ubuntu) the "gnome-clock" makes little sense and
causes a lot of trouble, so I really want to get rid of it as soon as
possible. The problem is that Geoclue is still problematic for some
people.

Lastly, there's the support for configuration files for which there's
no real documentation, but all the options that can be set on the
command line can also be set in the config file.
