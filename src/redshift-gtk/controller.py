# controller.py -- Redshift child process controller
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

import os
import re
import fcntl
import signal
import gettext

import gi
gi.require_version('GLib', '2.0')

from gi.repository import GLib, GObject

from . import defs
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
        'error-occured': (GObject.SIGNAL_RUN_FIRST, None, (str,)),
        'stopped': (GObject.SIGNAL_RUN_FIRST, None, ()),
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
        self.detect_fullscreen = False
        self._thread = None

        # Toggle fullscreen detection
        if watch_events is not None:
            self.detect_fullscreen = True
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
        self.emit('stopped')

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

