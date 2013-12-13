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
import signal
import re
import gettext

from gi.repository import Gdk, Gtk, GLib, GObject
from gi.repository import Gio

try:
    from gi.repository import AppIndicator3 as appindicator
except ImportError:
    appindicator = None

from . import defs
from . import utils

_ = gettext.gettext


class RedshiftController(GObject.GObject):
    __gsignals__ = {
        'inhibit-changed': (GObject.SIGNAL_RUN_FIRST, None, (bool,)),
        'temperature-changed': (GObject.SIGNAL_RUN_FIRST, None, (int,)),
        'period-changed': (GObject.SIGNAL_RUN_FIRST, None, (str,)),
        'location-changed': (GObject.SIGNAL_RUN_FIRST, None, (float, float))
        }

    def __init__(self, name):
        GObject.GObject.__init__(self)

        # Connect to Redshift DBus service
        bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
        self._redshift = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
                                                'dk.jonls.redshift.Redshift',
                                                '/dk/jonls/redshift/Redshift',
                                                'dk.jonls.redshift.Redshift', None)
        self._redshift_props = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
                                                      'dk.jonls.redshift.Redshift',
                                                      '/dk/jonls/redshift/Redshift',
                                                      'org.freedesktop.DBus.Properties', None)

        self._name = name
        self._cookie = None

        # Setup signals to property changes
        self._redshift.connect('g-properties-changed', self._property_change_cb)

    def _property_change_cb(self, proxy, props, invalid, data=None):
        # Cast to python dict as 'in' keyword is broken for GLib dict
        props = dict(props)
        if 'Inhibited' in props:
            self.emit('inhibit-changed', props['Inhibited'])
        if 'Temperature' in props:
            self.emit('temperature-changed', props['Temperature'])
        if 'Period' in props:
            self.emit('period-changed', props['Period'])
        if 'CurrentLatitude' in props and 'CurrentLongitude' in props:
            self.emit('location-changed', props['CurrentLatitude'], props['CurrentLongitude'])

    @property
    def cookie(self):
        if self._cookie is None:
            self._cookie = self._redshift.AcquireCookie('(s)', self._name)
        return self._cookie

    def release_cookie(self):
        if self._cookie is not None:
            self._redshift.ReleaseCookie('(i)', self._cookie)
        self._cookie = None

    def __del__(self):
        self.release_cookie()

    @property
    def elevation(self):
        return self._redshift.GetElevation()

    @property
    def inhibited(self):
        return self._redshift_props.Get('(ss)', 'dk.jonls.redshift.Redshift', 'Inhibited')

    @property
    def temperature(self):
        return self._redshift_props.Get('(ss)', 'dk.jonls.redshift.Redshift', 'Temperature')

    @property
    def period(self):
        return self._redshift_props.Get('(ss)', 'dk.jonls.redshift.Redshift', 'Period')

    @property
    def location(self):
        lat = self._redshift_props.Get('(ss)', 'dk.jonls.redshift.Redshift', 'CurrentLatitude')
        lon = self._redshift_props.Get('(ss)', 'dk.jonls.redshift.Redshift', 'CurrentLongitude')
        return (lat, lon)

    def set_inhibit(self, inhibit):
        if inhibit:
            self._redshift.Inhibit('(i)', self.cookie)
        else:
            self._redshift.Uninhibit('(i)', self.cookie)


class RedshiftStatusIcon(object):
    def __init__(self):
        # Initialize controller
        self._controller = RedshiftController('redshift-gtk')

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

        # Setup signals to property changes
        self._controller.connect('inhibit-changed', self.inhibit_change_cb)
        self._controller.connect('period-changed', self.period_change_cb)
        self._controller.connect('temperature-changed', self.temperature_change_cb)
        self._controller.connect('location-changed', self.location_change_cb)

        # Set info box text
        self.change_inhibited(self._controller.inhibited)
        self.change_period(self._controller.period)
        self.change_temperature(self._controller.temperature)
        self.change_location(self._controller.location)

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

        # Notify desktop that startup is complete
        Gdk.notify_startup_complete()

    def remove_suspend_timer(self):
        if self.suspend_timer is not None:
            GLib.source_remove(self.suspend_timer)
            self.suspend_timer = None

    def suspend_cb(self, item, minutes):
        # Inhibit
        self._controller.set_inhibit(True)
        self.remove_suspend_timer()

        # If redshift was already disabled we reenable it nonetheless.
        self.suspend_timer = GLib.timeout_add_seconds(minutes * 60, self.reenable_cb)

    def reenable_cb(self):
        self._controller.set_inhibit(False)

    def popup_menu_cb(self, widget, button, time, data=None):
        self.status_menu.show_all()
        self.status_menu.popup(None, None, Gtk.StatusIcon.position_menu,
                               self.status_icon, button, time)

    def toggle_cb(self, widget, data=None):
        self.remove_suspend_timer()
        self._controller.set_inhibit(not self._controller.inhibited)

    def toggle_item_cb(self, widget, data=None):
        # Only toggle if a change from current state was requested
        active = not self._controller.inhibited
        if active != widget.get_active():
            self.remove_suspend_timer()
            self._controller.set_inhibit(not self._controller.inhibited)

    # Info dialog callbacks
    def show_info_cb(self, widget, data=None):
        self.info_dialog.show()

    def response_info_cb(self, widget, data=None):
        self.info_dialog.hide()

    def update_status_icon(self):
        # Update status icon
        if appindicator:
            if not self._controller.inhibited:
                self.indicator.set_icon('redshift-status-on')
            else:
                self.indicator.set_icon('redshift-status-off')
        else:
            if not self._controller.inhibited:
                self.status_icon.set_from_icon_name('redshift-status-on')
            else:
                self.status_icon.set_from_icon_name('redshift-status-off')


    # State update functions
    def inhibit_change_cb(self, controller, inhibit):
        self.change_inhibited(inhibit)

    def period_change_cb(self, controller, period):
        self.change_period(period)

    def temperature_change_cb(self, controller, temperature):
        self.change_temperature(temperature)

    def location_change_cb(self, controller, lat, lon):
        self.change_location((lat, lon))


    # Update interface
    def change_inhibited(self, inhibited):
        self.update_status_icon()
        self.toggle_item.set_active(not inhibited)
        self.status_label.set_markup(_('<b>Status:</b> {}').format(_('Disabled') if inhibited else _('Enabled')))

    def change_temperature(self, temperature):
        self.temperature_label.set_markup(_('<b>Color temperature:</b> {}K').format(temperature))

    def change_period(self, period):
        self.period_label.set_markup(_('<b>Period:</b> {}').format(period))

    def change_location(self, location):
        self.location_label.set_markup(_('<b>Location:</b> {}, {}').format(*location))


    def autostart_cb(self, widget, data=None):
        utils.set_autostart(widget.get_active())

    def destroy_cb(self, widget, data=None):
        if not appindicator:
            self.status_icon.set_visible(False)
        Gtk.main_quit()
        return False

def sigterm_handler(signal, frame):
    sys.exit(0)


def run():
    utils.setproctitle('redshift-gtk')

    # Install TERM signal handler
    signal.signal(signal.SIGTERM, sigterm_handler)

    # Internationalisation
    gettext.bindtextdomain('redshift', defs.LOCALEDIR)
    gettext.textdomain('redshift')

    # Create status icon
    s = RedshiftStatusIcon()

    # Run main loop
    Gtk.main()
