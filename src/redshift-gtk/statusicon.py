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

# Copyright (c) 2013  Jon Lund Steffensen <jonlst@gmail.com>


'''GUI status icon for Redshift.

The run method will try to start an appindicator for Redshift. If the
appindicator module isn't present it will fall back to a GTK status icon.
'''

import sys, os
import signal, fcntl
import re
import gettext

from gi.repository import Gdk, Gtk, GLib
try:
    from gi.repository import AppIndicator3 as appindicator
except ImportError:
    appindicator = None

from . import defs
from . import utils

_ = gettext.gettext


def sigterm_handler(signal, frame):
    sys.exit()


class RedshiftStatusIcon(object):
    def __init__(self, args=[]):
        # Initialize state variables
        self._status = False
        self._temperature = 0
        self._period = 'Unknown'
        self._location = (0.0, 0.0)

        # Install TERM signal handler
        signal.signal(signal.SIGTERM, sigterm_handler)

        # Start redshift with arguments
        args.insert(0, os.path.join(defs.BINDIR, 'redshift'))
        if '-v' not in args:
            args.append('-v')

        self.start_child_process(args)

        if appindicator:
            # Create indicator
            self.indicator = appindicator.Indicator.new('redshift',
                                                        'redshift-status-on',
                                                        appindicator.IndicatorCategory.APPLICATION_STATUS)
            self.indicator.set_status(appindicator.IndicatorStatus.ACTIVE)
        else:
            # Create status icon
            self.status_icon = Gtk.StatusIcon()
            self.status_icon.set_from_icon_name('redshift-status-on')
            self.status_icon.set_tooltip_text('Redshift')

        # Create popup menu
        self.status_menu = Gtk.Menu()

        # Add toggle action
        self.toggle_item = Gtk.CheckMenuItem.new_with_label(_('Enabled'))
        self.toggle_item.connect('activate', self.toggle_item_cb)
        self.status_menu.append(self.toggle_item)

        # Add suspend menu
        suspend_menu_item = Gtk.MenuItem.new_with_label(_('Suspend for'))
        suspend_menu = Gtk.Menu()
        for minutes, label in [(30, _('30 minutes')),
                               (60, _('1 hour')),
                               (120, _('2 hours'))]:
            suspend_item = Gtk.MenuItem.new_with_label(label)
            suspend_item.connect('activate', self.suspend_cb, minutes)
            suspend_menu.append(suspend_item)
        suspend_menu_item.set_submenu(suspend_menu)
        self.status_menu.append(suspend_menu_item)

        # Add autostart option
        autostart_item = Gtk.CheckMenuItem.new_with_label(_('Autostart'))
        try:
            autostart_item.set_active(utils.get_autostart())
        except IOError as strerror:
            print(strerror)
            autostart_item.set_property('sensitive', False)
        else:
            autostart_item.connect('toggled', self.autostart_cb)
        finally:
            self.status_menu.append(autostart_item)

        # Add info action
        info_item = Gtk.MenuItem.new_with_label(_('Info'))
        info_item.connect('activate', self.show_info_cb)
        self.status_menu.append(info_item)

        # Add quit action
        quit_item = Gtk.ImageMenuItem.new_with_label(_('Quit'))
        quit_item.connect('activate', self.destroy_cb)
        self.status_menu.append(quit_item)

        # Create info dialog
        self.info_dialog = Gtk.Dialog()
        self.info_dialog.set_title(_('Info'))
        self.info_dialog.add_button(_('Close'), Gtk.ButtonsType.CLOSE)
        self.info_dialog.set_resizable(False)
        self.info_dialog.set_property('border-width', 6)

        self.status_label = Gtk.Label()
        self.status_label.set_alignment(0.0, 0.5)
        self.status_label.set_padding(6, 6);
        self.info_dialog.get_content_area().pack_start(self.status_label, True, True, 0)
        self.status_label.show()

        self.location_label = Gtk.Label()
        self.location_label.set_alignment(0.0, 0.5)
        self.location_label.set_padding(6, 6);
        self.info_dialog.get_content_area().pack_start(self.location_label, True, True, 0)
        self.location_label.show()

        self.temperature_label = Gtk.Label()
        self.temperature_label.set_alignment(0.0, 0.5)
        self.temperature_label.set_padding(6, 6);
        self.info_dialog.get_content_area().pack_start(self.temperature_label, True, True, 0)
        self.temperature_label.show()

        self.period_label = Gtk.Label()
        self.period_label.set_alignment(0.0, 0.5)
        self.period_label.set_padding(6, 6);
        self.info_dialog.get_content_area().pack_start(self.period_label, True, True, 0)
        self.period_label.show()

        self.info_dialog.connect('response', self.response_info_cb)

        if appindicator:
            self.status_menu.show_all()

            # Set the menu
            self.indicator.set_menu(self.status_menu)
        else:
            # Connect signals for status icon and show
            self.status_icon.connect('activate', self.toggle_cb)
            self.status_icon.connect('popup-menu', self.popup_menu_cb)
            self.status_icon.set_visible(True)

        # Initialize suspend timer
        self.suspend_timer = None

        # Handle child input
        # The buffer is encapsulated in a class so we
        # can pass an instance to the child callback.
        class InputBuffer(object):
            buf = ''

        self.input_buffer = InputBuffer()
        self.error_buffer = InputBuffer()

        # Set non blocking
        fcntl.fcntl(self.process[2], fcntl.F_SETFL,
                    fcntl.fcntl(self.process[2], fcntl.F_GETFL) | os.O_NONBLOCK)

        # Add watch on child process
        GLib.child_watch_add(GLib.PRIORITY_DEFAULT, self.process[0], self.child_cb)
        GLib.io_add_watch(self.process[2], GLib.PRIORITY_DEFAULT, GLib.IO_IN,
                          self.child_data_cb, (True, self.input_buffer))
        GLib.io_add_watch(self.process[3], GLib.PRIORITY_DEFAULT, GLib.IO_IN,
                          self.child_data_cb, (False, self.error_buffer))

        # Notify desktop that startup is complete
        Gdk.notify_startup_complete()

    def start_child_process(self, args):
        # Start child process with C locale so we can parse the output
        env = os.environ.copy()
        env['LANG'] = env['LANGUAGE'] = env['LC_ALL'] = env['LC_MESSAGES'] = 'C'
        self.process = GLib.spawn_async(args, envp=['{}={}'.format(k,v) for k, v in env.items()],
                                        flags=GLib.SPAWN_DO_NOT_REAP_CHILD,
                                        standard_output=True, standard_error=True)

    def remove_suspend_timer(self):
        if self.suspend_timer is not None:
            GLib.source_remove(self.suspend_timer)
            self.suspend_timer = None

    def suspend_cb(self, item, minutes):
        if self.is_enabled():
            self.child_toggle_status()

        # If "suspend" is clicked while redshift is disabled, we reenable
        # it after the last selected timespan is over.
        self.remove_suspend_timer()

        # If redshift was already disabled we reenable it nonetheless.
        self.suspend_timer = GLib.timeout_add_seconds(minutes * 60, self.reenable_cb)

    def reenable_cb(self):
        if not self.is_enabled():
            self.child_toggle_status()

    def popup_menu_cb(self, widget, button, time, data=None):
        self.status_menu.show_all()
        self.status_menu.popup(None, None, Gtk.StatusIcon.position_menu,
                               self.status_icon, button, time)

    def toggle_cb(self, widget, data=None):
        self.remove_suspend_timer()
        self.child_toggle_status()

    def toggle_item_cb(self, widget, data=None):
        # Only toggle if a change from current state was requested
        if self.is_enabled() != widget.get_active():
            self.remove_suspend_timer()
            self.child_toggle_status()

    # Info dialog callbacks
    def show_info_cb(self, widget, data=None):
        self.info_dialog.show()

    def response_info_cb(self, widget, data=None):
        self.info_dialog.hide()

    def update_status_icon(self):
        # Update status icon
        if appindicator:
            if self.is_enabled():
                self.indicator.set_icon('redshift-status-on')
            else:
                self.indicator.set_icon('redshift-status-off')
        else:
            if self.is_enabled():
                self.status_icon.set_from_icon_name('redshift-status-on')
            else:
                self.status_icon.set_from_icon_name('redshift-status-off')

    # Status update functions. Called when child process indicates a
    # change in state.
    def change_status(self, status):
        self._status = status

        self.update_status_icon()
        self.toggle_item.set_active(self.is_enabled())
        self.status_label.set_markup('<b>{}:</b> {}'.format(_('Status'),
                                                            _('Enabled') if status else _('Disabled')))

    def change_temperature(self, temperature):
        self._temperature = temperature
        self.temperature_label.set_markup('<b>{}:</b> {}K'.format(_('Color temperature'), temperature))

    def change_period(self, period):
        self._period = period
        self.period_label.set_markup('<b>{}:</b> {}'.format(_('Period'), period))

    def change_location(self, location):
        self._location = location
        self.location_label.set_markup('<b>{}:</b> {}, {}'.format(_('Location'), *location))


    def is_enabled(self):
        return self._status

    def autostart_cb(self, widget, data=None):
        utils.set_autostart(widget.get_active())

    def destroy_cb(self, widget, data=None):
        if not appindicator:
            self.status_icon.set_visible(False)
        Gtk.main_quit()
        return False

    def child_toggle_status(self):
        os.kill(self.process[0], signal.SIGUSR1)

    def child_cb(self, pid, cond, data=None):
        sys.exit(-1)

    def child_key_change_cb(self, key, value):
        if key == 'Status':
            self.change_status(value != 'Disabled')
        elif key == 'Color temperature':
            self.change_temperature(int(value.rstrip('K'), 10))
        elif key == 'Period':
            self.change_period(value)
        elif key == 'Location':
            self.change_location(value.split(', '))

    def child_stdout_line_cb(self, line):
        if line:
            m = re.match(r'([\w ]+): (.+)', line)
            if m:
                key = m.group(1)
                value = m.group(2)
                self.child_key_change_cb(key, value)

    def child_data_cb(self, f, cond, data):
        stdout, ib = data
        ib.buf += os.read(f, 256).decode('utf-8')

        # Split input at line break
        while True:
            first, sep, last = ib.buf.partition('\n')
            if sep == '':
                break
            ib.buf = last
            if stdout:
                self.child_stdout_line_cb(first)

        return True

    def termwait(self):
        os.kill(self.process[0], signal.SIGINT)
        os.waitpid(self.process[0], 0)


def run():
    utils.setproctitle('redshift-gtk')

    # Internationalisation
    gettext.bindtextdomain('redshift', defs.LOCALEDIR)
    gettext.textdomain('redshift')

    # Create status icon
    s = RedshiftStatusIcon(sys.argv[1:])

    try:
        # Run main loop
        Gtk.main()
    except KeyboardInterrupt:
        # Ignore user interruption
        pass
    finally:
        # Always terminate redshift
        s.termwait()
