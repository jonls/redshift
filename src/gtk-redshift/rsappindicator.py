#!/usr/bin/env python
# rsappindicator.py -- Application Panel Indicator source
# This file is part of Redshift.

# Redshift is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# Redshift is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with Redshift.  If not, see <http://www.gnu.org/licenses/>.

# Copyright (c) 2010  Jon Lund Steffensen <jonlst@gmail.com>


import sys, os
import subprocess, signal
import gettext

import pygtk
pygtk.require("2.0")

import gtk, glib
try:
    import appindicator
except ImportError as ie:
    # No module named appindicator
    sys.exit(str(ie))

import defs
import utils


def run():
    # Internationalisation
    gettext.bindtextdomain('redshift', defs.LOCALEDIR)
    gettext.textdomain('redshift')
    _ = gettext.gettext

    # Start redshift with arguments from the command line
    args = sys.argv[1:]
    args.insert(0, os.path.join(defs.BINDIR, 'redshift'))
    process = subprocess.Popen(args)

    try:
        # Create status icon
        indicator = appindicator.Indicator ("redshift",
                              "redshift",
                              appindicator.CATEGORY_APPLICATION_STATUS)
        indicator.set_status (appindicator.STATUS_ACTIVE)

        def toggle_cb(widget, data=None):
            if indicator.get_icon() == 'redshift':
                indicator.set_icon('redshift-idle')
            else:
                indicator.set_icon('redshift')
            process.send_signal(signal.SIGUSR1)

        def autostart_cb(widget, data=None):
            utils.set_autostart(widget.get_active())

        def destroy_cb(widget, data=None):
            gtk.main_quit()
            return False

        # Create popup menu
        status_menu = gtk.Menu()

        toggle_item = gtk.ImageMenuItem(_('Toggle'))
        toggle_item.connect('activate', toggle_cb)
        status_menu.append(toggle_item)

        autostart_item = gtk.CheckMenuItem(_('Autostart'))
        autostart_item.set_active(utils.get_autostart())
        autostart_item.connect('activate', autostart_cb)
        status_menu.append(autostart_item)

        quit_item = gtk.ImageMenuItem(gtk.STOCK_QUIT)
        quit_item.connect('activate', destroy_cb)
        status_menu.append(quit_item)

        status_menu.show_all()

        # Set the menu
        indicator.set_menu(status_menu)

        def child_cb(pid, cond, data=None):
            sys.exit(-1)

        # Add watch on child process
        glib.child_watch_add(process.pid, child_cb)

        # Run main loop
        gtk.main()

    except KeyboardInterrupt:
        # Ignore user interruption
        pass

    finally:
        # Always terminate redshift
        process.terminate()
        process.wait()
