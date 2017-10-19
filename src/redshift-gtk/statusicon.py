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

# Copyright (c) 2013-2017  Jon Lund Steffensen <jonlst@gmail.com>


"""GUI status icon for Redshift.

The run method will try to start an appindicator for Redshift. If the
appindicator module isn't present it will fall back to a GTK status icon.
"""

import sys
import signal
import gettext

import gi
gi.require_version('Gtk', '3.0')

from gi.repository import Gtk, GLib

try:
    gi.require_version('AppIndicator3', '0.1')
    from gi.repository import AppIndicator3 as appindicator
except (ImportError, ValueError):
    appindicator = None

from .controller import RedshiftController
from . import defs
from . import utils
try:
    from . import watch_events
except (ImportError, ValueError):
    watch_events = None

_ = gettext.gettext


class RedshiftController(GObject.GObject):
    '''A GObject wrapper around the child process'''

    __gsignals__ = {
        'inhibit-changed': (GObject.SIGNAL_RUN_FIRST, None, (bool,)),
        'temperature-changed': (GObject.SIGNAL_RUN_FIRST, None, (int,)),
        'period-changed': (GObject.SIGNAL_RUN_FIRST, None, (str,)),
        'location-changed': (GObject.SIGNAL_RUN_FIRST, None, (float, float)),
        'fullscreen-changed': (GObject.SIGNAL_RUN_FIRST, None, (bool,)),
        'error-occured': (GObject.SIGNAL_RUN_FIRST, None, (str,))
        }

    def __init__(self, args):
        '''Initialize controller and start child process

        The parameter args is a list of command line arguments to pass on to
        the child process. The "-v" argument is automatically added.'''

        GObject.GObject.__init__(self)

        # Initialize state variables
        self._inhibited = False
        self._temperature = 0
        self._period = 'Unknown'
        self._location = (0.0, 0.0)
        self._fullscreen = False
        self._manually_inhibited = False
        self._thread = None

        # Toggle fullscreen detection
        if watch_events is not None:
            if '-f' in args:
                args.remove('-f')
                self._fullscreen = True

        # Start redshift with arguments
        args.insert(0, os.path.join(defs.BINDIR, 'redshift'))
        if '-v' not in args:
            args.insert(1, '-v')

        # Start child process with C locale so we can parse the output
        env = os.environ.copy()
        env['LANG'] = env['LANGUAGE'] = env['LC_ALL'] = env['LC_MESSAGES'] = 'C'
        self._process = GLib.spawn_async(args, envp=['{}={}'.format(k,v) for k, v in env.items()],
                                         flags=GLib.SPAWN_DO_NOT_REAP_CHILD,
                                         standard_output=True, standard_error=True)

        # Start thread to detect fullscreen
        self.set_run_threads(self._fullscreen)
        
        # Wrap remaining contructor in try..except to avoid that the child
        # process is not closed properly.
        try:
            # Handle child input
            # The buffer is encapsulated in a class so we
            # can pass an instance to the child callback.
            class InputBuffer(object):
                buf = ''

            self._input_buffer = InputBuffer()
            self._error_buffer = InputBuffer()
            self._errors = ''

            # Set non blocking
            fcntl.fcntl(self._process[2], fcntl.F_SETFL,
                        fcntl.fcntl(self._process[2], fcntl.F_GETFL) | os.O_NONBLOCK)

            # Add watch on child process
            GLib.child_watch_add(GLib.PRIORITY_DEFAULT, self._process[0], self._child_cb)
            GLib.io_add_watch(self._process[2], GLib.PRIORITY_DEFAULT, GLib.IO_IN,
                              self._child_data_cb, (True, self._input_buffer))
            GLib.io_add_watch(self._process[3], GLib.PRIORITY_DEFAULT, GLib.IO_IN,
                              self._child_data_cb, (False, self._error_buffer))

            # Signal handler to relay USR1 signal to redshift process
            def relay_signal_handler(signal):
                os.kill(self._process[0], signal)
                return True

            GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGUSR1,
                                 relay_signal_handler, signal.SIGUSR1)
        except:
            self.termwait()
            raise

    @property
    def inhibited(self):
        '''Current inhibition state'''
        return self._inhibited

    @property
    def temperature(self):
        '''Current screen temperature'''
        return self._temperature

    @property
    def period(self):
        '''Current period of day'''
        return self._period

    @property
    def location(self):
        '''Current location'''
        return self._location

    @property
    def fullscreen(self):
        '''Current state of fullscreen detection'''
        return self._fullscreen

    @fullscreen.setter
    def fullscreen(self, state):
        '''Set the fullscreen detection state'''
        if self._fullscreen != state:
            self._fullscreen == self.set_run_threads(state)

    def set_manually_inhibit(self, inhibit):
        '''Set manual inhibition state'''
        self._manually_inhibited = inhibit
        if self.fullscreen:
            self.set_run_threads(not inhibit)
        self.set_inhibit(inhibit)

    def set_inhibit(self, inhibit):
        '''Set inhibition state'''
        if inhibit != self._inhibited:
            self._child_toggle_inhibit()

    def _child_signal(self, sg):
        """Send signal to child process."""
        os.kill(self._process[0], sg)

    def _child_toggle_inhibit(self):
        '''Sends a request to the child process to toggle state'''
        self._child_signal(signal.SIGUSR1)

    def _child_cb(self, pid, status, data=None):
        '''Called when the child process exists'''

        # Empty stdout and stderr
        for f in (self._process[2], self._process[3]):
            while True:
                buf = os.read(f, 256).decode('utf-8')
                if buf == '':
                    break
                if f == self._process[3]: # stderr
                    self._errors += buf

        # Check exit status of child
        report_errors = False
        try:
            GLib.spawn_check_exit_status(status)
        except GLib.GError:
            self.emit('error-occured', self._errors)

        GLib.spawn_close_pid(self._process[0])
        Gtk.main_quit()

    def _child_key_change_cb(self, key, value):
        '''Called when the child process reports a change of internal state'''

        def parse_coord(s):
            '''Parse coordinate like `42.0 N` or `91.5 W`'''
            v, d = s.split(' ')
            return float(v) * (1 if d in 'NE' else -1)

        if key == 'Status':
            new_inhibited = value != 'Enabled'
            if new_inhibited != self._inhibited:
                self._inhibited = new_inhibited
                self.emit('inhibit-changed', new_inhibited)
        elif key == 'Color temperature':
            new_temperature = int(value.rstrip('K'), 10)
            if new_temperature != self._temperature:
                self._temperature = new_temperature
                self.emit('temperature-changed', new_temperature)
        elif key == 'Period':
            new_period = value
            if new_period != self._period:
                self._period = new_period
                self.emit('period-changed', new_period)
        elif key == 'Location':
            new_location = tuple(parse_coord(x) for x in value.split(', '))
            if new_location != self._location:
                self._location = new_location
                self.emit('location-changed', *new_location)

    def _child_stdout_line_cb(self, line):
        '''Called when the child process outputs a line to stdout'''
        if line:
            m = re.match(r'([\w ]+): (.+)', line)
            if m:
                key = m.group(1)
                value = m.group(2)
                self._child_key_change_cb(key, value)

    def _child_data_cb(self, f, cond, data):
        '''Called when the child process has new data on stdout/stderr'''

        stdout, ib = data
        ib.buf += os.read(f, 256).decode('utf-8')

        # Split input at line break
        while True:
            first, sep, last = ib.buf.partition('\n')
            if sep == '':
                break
            ib.buf = last
            if stdout:
                self._child_stdout_line_cb(first)
            else:
                self._errors += first + '\n'

        return True

    def set_run_threads(self, state):
        '''Set the threads running if state=True or stop them.'''
        if state and self._thread is None:
            self.start_threads()
        elif not state and self._thread is not None:
            self.stop_threads()
        return state
    
    def start_threads(self):
        '''Start the threads
        
        Watch asynchronously for the active window getting fullscreen
        '''
        if watch_events is None:
            self._thread = None
            return
        
        # Initialize the thread
        self._thread = watch_events.WatchThread()

        # Connect the thread signals
        self._thread.connect("completed", self._register_thread_cancelled)
        self._thread.connect("inhibit-triggered", self.set_auto_inhibit)

        # Start thread
        self._thread.start()

    def stop_threads(self, block=False):
        """Stops all threads. If block is True then actually wait for 
        the thread to finish (may block the UI) 
        """
        if self._thread:
            self._thread.cancel()
            if block:
                if self._thread.isAlive():
                    self._thread.join()
        self._thread = None

    def _register_thread_cancelled(self, thread, state):
        '''Relaunch the thread if failed'''
        pass

    def set_auto_inhibit(self, thread, state):
        '''Set inhibit if the active window change fullscreen state'''
        if state != self.inhibited:
            if not self._manually_inhibited:
                print(_("[{}] Change of fullscreen mode detected, redshift {}.").format(self.__class__.__name__, _('disabled') if state else _('enabled')))
                self.set_inhibit(state)
 
    def terminate_child(self):
        """Send SIGINT to child process."""
        self.stop_threads()
        self._child_signal(signal.SIGINT)

    def kill_child(self):
        """Send SIGKILL to child process."""
        self.stop_threads()
        self._child_signal(signal.SIGKILL)


class RedshiftStatusIcon(object):
    """The status icon tracking the RedshiftController."""

    def __init__(self, controller):
        """Creates a new instance of the status icon."""

        self._controller = controller

        if appindicator:
            # Create indicator
            self.indicator = appindicator.Indicator.new(
                'redshift',
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

        # Add fullscreen menu
        if watch_events is not None:
            self.fullscreen_item = Gtk.CheckMenuItem.new_with_label(_('Disable on fullscreen'))
            self.fullscreen_item.connect('activate', self.fullscreen_item_cb)
            self.status_menu.append(self.fullscreen_item)
        else:
            self.fullscreen_item = None

        # Add autostart option
        if utils.supports_autostart():
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
        self.info_dialog = Gtk.Window(title=_('Info'))
        self.info_dialog.set_resizable(False)
        self.info_dialog.set_property('border-width', 6)
        self.info_dialog.connect('delete-event', self.close_info_dialog_cb)

        content_area = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.info_dialog.add(content_area)
        content_area.show()

        self.status_label = Gtk.Label()
        self.status_label.set_alignment(0.0, 0.5)
        self.status_label.set_padding(6, 6)
        content_area.pack_start(self.status_label, True, True, 0)
        self.status_label.show()

        self.location_label = Gtk.Label()
        self.location_label.set_alignment(0.0, 0.5)
        self.location_label.set_padding(6, 6)
        content_area.pack_start(self.location_label, True, True, 0)
        self.location_label.show()

        self.temperature_label = Gtk.Label()
        self.temperature_label.set_alignment(0.0, 0.5)
        self.temperature_label.set_padding(6, 6)
        content_area.pack_start(self.temperature_label, True, True, 0)
        self.temperature_label.show()

        self.period_label = Gtk.Label()
        self.period_label.set_alignment(0.0, 0.5)
        self.period_label.set_padding(6, 6)
        content_area.pack_start(self.period_label, True, True, 0)
        self.period_label.show()

        self.close_button = Gtk.Button(label=_('Close'))
        content_area.pack_start(self.close_button, True, True, 0)
        self.close_button.connect('clicked', self.close_info_dialog_cb)
        self.close_button.show()

        # Setup signals to property changes
        self._controller.connect('inhibit-changed', self.inhibit_change_cb)
        self._controller.connect('period-changed', self.period_change_cb)
        self._controller.connect(
            'temperature-changed', self.temperature_change_cb)
        self._controller.connect('location-changed', self.location_change_cb)
        self._controller.connect('fullscreen-changed', self.fullscreen_change_cb)
        self._controller.connect('error-occured', self.error_occured_cb)
        self._controller.connect('stopped', self.controller_stopped_cb)

        # Set info box text
        self.change_inhibited(self._controller.inhibited)
        self.change_period(self._controller.period)
        self.change_temperature(self._controller.temperature)
        self.change_location(self._controller.location)
        self.change_fullscreen(self._controller.fullscreen)

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

    def remove_suspend_timer(self):
        """Disable any previously set suspend timer."""
        if self.suspend_timer is not None:
            GLib.source_remove(self.suspend_timer)
            self.suspend_timer = None

    def manually_inhibit(self, inhibit):
        '''Callback that handles manual inhibition'''

        # Inhibit
        self._controller.set_manually_inhibit(inhibit)

    def suspend_cb(self, item, minutes):
        """Callback that handles activation of a suspend timer.

        The minutes parameter is the number of minutes to suspend. Even if
        redshift is not disabled when called, it will still set a suspend timer
        and reactive redshift when the timer is up.
        """
        # Inhibit
        self.manually_inhibit(True)

        # If "suspend" is clicked while redshift is disabled, we reenable
        # it after the last selected timespan is over.
        self.remove_suspend_timer()

        # If redshift was already disabled we reenable it nonetheless.
        self.suspend_timer = GLib.timeout_add_seconds(
            minutes * 60, self.reenable_cb)

    def reenable_cb(self):
        """Callback to reenable redshift when a suspend timer expires."""
        self.manually_inhibit(False)

    def popup_menu_cb(self, widget, button, time, data=None):
        """Callback when the popup menu on the status icon has to open."""
        self.status_menu.show_all()
        self.status_menu.popup(None, None, Gtk.StatusIcon.position_menu,
                               self.status_icon, button, time)

    def toggle_cb(self, widget, data=None):
        """Callback when a request to toggle redshift was made."""
        self.remove_suspend_timer()
        self.manually_inhibit(not self._controller.inhibited)

    def toggle_item_cb(self, widget, data=None):
        """Callback when a request to toggle redshift was made.

        This ensures that the state of redshift is synchronised with
        the toggle state of the widget (e.g. Gtk.CheckMenuItem).
        """
        active = not self._controller.inhibited
        if active != widget.get_active():
            self.remove_suspend_timer()
            self.manually_inhibit(not self._controller.inhibited)

    def fullscreen_item_cb(self, widget, data=None):
        '''Callback then a request to disable redshift on fullscreen was made from a fullscreen item

        This ensures that the state of redshift is synchronised with
        the fullscreen state of the widget (e.g. Gtk.CheckMenuItem).'''

        fullscreen = self._controller.fullscreen
        if fullscreen != widget.get_active():
            self._controller.fullscreen = not fullscreen

    # Info dialog callbacks
    def show_info_cb(self, widget, data=None):
        """Callback when the info dialog should be presented."""
        self.info_dialog.show()

    def response_info_cb(self, widget, data=None):
        """Callback when a button in the info dialog was activated."""
        self.info_dialog.hide()

    def close_info_dialog_cb(self, widget, data=None):
        """Callback when the info dialog is closed."""
        self.info_dialog.hide()
        return True

    def update_status_icon(self):
        """Update the status icon according to the internally recorded state.

        This should be called whenever the internally recorded state
        might have changed.
        """
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
        """Callback when controller changes inhibition status."""
        self.change_inhibited(inhibit)

    def period_change_cb(self, controller, period):
        """Callback when controller changes period."""
        self.change_period(period)

    def temperature_change_cb(self, controller, temperature):
        """Callback when controller changes temperature."""
        self.change_temperature(temperature)

    def location_change_cb(self, controller, lat, lon):
        """Callback when controlled changes location."""
        self.change_location((lat, lon))

    def fullscreen_change_cb(self, controller, state):
        '''Callback when controlled changes fullscreen'''
        self.change_fullscreen(state)

    def error_occured_cb(self, controller, error):
        """Callback when an error occurs in the controller."""
        error_dialog = Gtk.MessageDialog(
            None, Gtk.DialogFlags.MODAL, Gtk.MessageType.ERROR,
            Gtk.ButtonsType.CLOSE, '')
        error_dialog.set_markup(
            '<b>Failed to run Redshift</b>\n<i>' + error + '</i>')
        error_dialog.run()

        # Quit when the model dialog is closed
        sys.exit(-1)

    def controller_stopped_cb(self, controller):
        """Callback when controlled is stopped successfully."""
        Gtk.main_quit()

    # Update interface
    def change_inhibited(self, inhibited):
        """Change interface to new inhibition status."""
        self.update_status_icon()
        self.toggle_item.set_active(not inhibited)
        self.status_label.set_markup(
            _('<b>Status:</b> {}').format(
                _('Disabled') if inhibited else _('Enabled')))

    def change_temperature(self, temperature):
        """Change interface to new temperature."""
        self.temperature_label.set_markup(
            '<b>{}:</b> {}K'.format(_('Color temperature'), temperature))
        self.update_tooltip_text()

    def change_period(self, period):
        """Change interface to new period."""
        self.period_label.set_markup(
            '<b>{}:</b> {}'.format(_('Period'), period))
        self.update_tooltip_text()

    def change_location(self, location):
        """Change interface to new location."""
        self.location_label.set_markup(
            '<b>{}:</b> {}, {}'.format(_('Location'), *location))

    def change_fullscreen(self, state):
        '''Change interface to new fullscreen status'''
        if self.fullscreen_item is not None:
            self.fullscreen_item.set_active(state)

    def update_tooltip_text(self):
        """Update text of tooltip status icon."""
        if not appindicator:
            self.status_icon.set_tooltip_text('{}: {}K, {}: {}'.format(
                _('Color temperature'), self._controller.temperature,
                _('Period'), self._controller.period))

    def autostart_cb(self, widget, data=None):
        """Callback when a request to toggle autostart is made."""
        utils.set_autostart(widget.get_active())

    def destroy_cb(self, widget, data=None):
        """Callback when a request to quit the application is made."""
        if not appindicator:
            self.status_icon.set_visible(False)
        self.info_dialog.destroy()
        self._controller.terminate_child()
        return False


def run():
    utils.setproctitle('redshift-gtk')

    # Internationalisation
    gettext.bindtextdomain('redshift', defs.LOCALEDIR)
    gettext.textdomain('redshift')

    for help_arg in ('-h', '--help'):
        if help_arg in sys.argv:
            print(_('Please run `redshift -h` for help output.'))
            sys.exit(-1)

    # Create redshift child process controller
    c = RedshiftController(sys.argv[1:])

    def terminate_child(data=None):
        c.terminate_child()
        return False

    # Install signal handlers
    GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGTERM,
                         terminate_child, None)
    GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGINT,
                         terminate_child, None)

    try:
        # Create status icon
        RedshiftStatusIcon(c)

        # Run main loop
        Gtk.main()
    except:
        c.kill_child()
        raise
