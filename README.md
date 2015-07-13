
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

Quick install & forget on Ubuntu
-----------------------
Download Redshift using Ubuntu Software Center or launch command in terminal: `sudo apt-get install redshift-gtk`

Open system menu and launch Session and Startup. Redshift should be already on the list. Tick the box next to it's name. From the next login, Redshift will adjust your screen automatically.

If Redshift is not working, it propably can't detect your location. To fix this, edit the entry in Session and Startup and modify command from `redshift-gtk` to `redshift -l LAT:LONG`, where `LAT` is your location latitude and `LONG` is your location longitude.

For example, when you are in New York, enter `redshift-gtk -l 41:-74`

Save it, tick the box next to Redshift name and close the window. 

Donations
---------

[![Flattr](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/thing/57936/Redshift)
