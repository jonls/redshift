# statusicon.py -- GUI status icon source
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


'''GUI status icon for Redshift.

The run method will try to start an appindicator for Redshift. If the
appindicator module isn't present it will fall back to a GTK status icon.
'''

import sys, os
import subprocess, signal
import gettext

import pygtk
pygtk.require("2.0")

import gtk, glib
try:
    import appindicator
except ImportError:
    appindicator = None

import defs
import utils


SUSPEND_TIMER = None


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
        if appindicator:
            # Create indicator
            indicator = appindicator.Indicator('redshift',
                                               'redshift-status-on',
                                               appindicator.CATEGORY_APPLICATION_STATUS)
            indicator.set_status(appindicator.STATUS_ACTIVE)
        else:
            # Create status icon
            status_icon = gtk.StatusIcon()
            status_icon.set_from_icon_name('redshift-status-on')
            status_icon.set_tooltip('Redshift')

        def is_enabled():
            if appindicator:
                return indicator.get_icon() == 'redshift-status-on'
            else:
                return status_icon.get_icon_name() == 'redshift-status-on'

        def remove_suspend_timer():
            global SUSPEND_TIMER
            if SUSPEND_TIMER is not None:
                glib.source_remove(SUSPEND_TIMER)
                SUSPEND_TIMER = None

        def toggle_cb(widget, data=None):
            # If the user toggles redshift, we forget about the suspend timer.
            # Only then widget is not None.
            if widget:
                remove_suspend_timer()

            process.send_signal(signal.SIGUSR1)
            if appindicator:
                if indicator.get_icon() == 'redshift-status-on':
                    indicator.set_icon('redshift-status-off')
                else:
                    indicator.set_icon('redshift-status-on')
            else:
	            if status_icon.get_icon_name() == 'redshift-status-on':
                        status_icon.set_from_icon_name('redshift-status-off')
	            else:
                        status_icon.set_from_icon_name('redshift-status-on')

        def enable_cb():
            if is_enabled():
                return
            # Enable redshift
            toggle_cb(None)

        def suspend_cb(widget, minutes):
            if is_enabled():
                # Disable redshift
                toggle_cb(None)
            # If "suspend" is clicked while redshift is disabled, we reenable
            # it after the last selected timespan is over.
            remove_suspend_timer()
            # If redshift was already disabled we reenable it nonetheless.
            global SUSPEND_TIMER
            SUSPEND_TIMER = glib.timeout_add_seconds(minutes * 60, enable_cb)

        def autostart_cb(widget, data=None):
            utils.set_autostart(widget.get_active())

        def destroy_cb(widget, data=None):
            if not appindicator:
                status_icon.set_visible(False)
            gtk.main_quit()
            return False

        # Create popup menu
        status_menu = gtk.Menu()

        toggle_item = gtk.MenuItem(_('Toggle'))
        toggle_item.connect('activate', toggle_cb)
        status_menu.append(toggle_item)

        suspend_menu_item = gtk.MenuItem(_('Suspend for'))
        suspend_menu = gtk.Menu()
        for minutes, label in [(30, _('30 minutes')), (60, _('1 hour')),
                               (120, _('2 hours'))]:
            suspend_item = gtk.MenuItem(label)
            suspend_item.connect('activate', suspend_cb, minutes)
            suspend_menu.append(suspend_item)
        suspend_menu_item.set_submenu(suspend_menu)
        status_menu.append(suspend_menu_item)

        autostart_item = gtk.CheckMenuItem(_('Autostart'))
        try:
            autostart_item.set_active(utils.get_autostart())
        except IOError as strerror:
            print strerror
            autostart_item.set_property('sensitive', False)
        else:
            autostart_item.connect('activate', autostart_cb)
        finally:
            status_menu.append(autostart_item)

        quit_item = gtk.ImageMenuItem(gtk.STOCK_QUIT)
        quit_item.connect('activate', destroy_cb)
        status_menu.append(quit_item)

        if appindicator:
            status_menu.show_all()

            # Set the menu
            indicator.set_menu(status_menu)
        else:
            def popup_menu_cb(widget, button, time, data=None):
                status_menu.show_all()
                status_menu.popup(None, None, gtk.status_icon_position_menu,
                                  button, time, status_icon)

            # Connect signals for status icon and show
            status_icon.connect('activate', toggle_cb)
            status_icon.connect('popup-menu', popup_menu_cb)
            status_icon.set_visible(True)

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
