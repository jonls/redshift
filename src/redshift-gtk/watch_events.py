# detect-fullscreen.py -- Detect if a window is in fullscreen on the same monitor
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

# Copyright (c) 2013-2014  Jon Lund Steffensen <jonlst@gmail.com>


'''Detect if a window is in fullscreen


'''
import time
import threading
import gi
gi.require_version('Gtk', '3.0')

from gi.repository import GObject

from ewmh import EWMH


## Threading from https://gist.github.com/nzjrs/51686            
class _IdleObject(GObject.GObject):
    """
    Override GObject.GObject to always emit signals in the main thread
    by emmitting on an idle handler
    """
    def __init__(self):
        GObject.GObject.__init__(self)

    def emit(self, *args):
        GObject.idle_add(GObject.GObject.emit,self,*args)


class WatchThread(threading.Thread, _IdleObject):
    """
    Cancellable thread which uses GObject signals to return information
    to the GUI.
    """
    __gsignals__ =  { 
        "completed": (GObject.SIGNAL_RUN_FIRST, None, (bool,)),
        "inhibit-triggered": (GObject.SIGNAL_RUN_FIRST, None, (bool,)),
        }

    def __init__(self, delta = 2, fullscreen=None):
        threading.Thread.__init__(self)
        _IdleObject.__init__(self)
        self._ewmh = EWMH()
        self.cancelled = False
        self.delta = delta
        self.fullscreen = fullscreen
        self.setName("Detect fullscreen mode")

    def cancel(self):
        """
        Threads in python are not cancellable, so we implement our own
        cancellation logic
        """
        self.cancelled = True

    def detect_fullscreen(self) -> bool:
        """Return True if the active window is fullscreen
        """
        fullscreen = False
        
        try:
            # Determine if a fullscreen application is running
            window = self._ewmh.getActiveWindow()
            # ewmh.getWmState(window) returns None is scenarios where
            # ewmh.getWmState(window, str=True) throws an exception
            # (it's a bug in pyewmh):
            if window and self._ewmh.getWmState(window):
                list_states = self._ewmh.getWmState(window, True)
                fullscreen = "_NET_WM_STATE_FULLSCREEN" in list_states
        except Exception as e:
            print("Error ignored:\n", e)
            return None
        return fullscreen
     
    def run(self):
        """Watch if the fullscreen mode is detected
        
        Wait `delta` seconds between each check. It can be cancelled by 
        changing the `cancelled` attribute. The `inhibit-triggered` 
        signal is emitted when the state is changed, holding the value
        True if the active window has become fullscreen, False if it has
        quit the fullscreen mode.
        """
        # Init the fullscreen state if it was not user-defined
        if self.fullscreen is None:
            self.fullscreen = self.detect_fullscreen()
        
        # Continuously detect changes in fullscreen mode    
        try:
            while(not self.cancelled):
                f = self.detect_fullscreen()

                if f is not None and f != self.fullscreen:
                    self.fullscreen = f
                    self.emit("inhibit-triggered", self.fullscreen)
                time.sleep(self.delta)
        except Exception as e:
            raise
            self.emit("completed", False)
        else:
            self.emit("completed", True)


