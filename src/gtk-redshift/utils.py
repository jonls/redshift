# utils.py -- utility functions source
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

# Copyright (c) 2010  Francesco Marella <francesco.marella@gmail.com>

import os
from xdg import BaseDirectory as base
from xdg import DesktopEntry as desktop

REDSHIFT_DESKTOP = 'gtk-redshift.desktop'


def get_autostart():
    AUTOSTART_KEY = "X-GNOME-Autostart-enabled"
    autostart_dir = base.save_config_path("autostart")
    autostart_file = os.path.join(autostart_dir, REDSHIFT_DESKTOP)
    if not os.path.exists(autostart_file):
        raise IOError("Installed redshift desktop file not found!")
    else:
        dfile = desktop.DesktopEntry(autostart_file)
        if dfile.get(AUTOSTART_KEY) == 'false':
            return False
        else:
            return True

def set_autostart(active):
    AUTOSTART_KEY = "X-GNOME-Autostart-enabled"
    autostart_dir = base.save_config_path("autostart")
    autostart_file = os.path.join(autostart_dir, REDSHIFT_DESKTOP)
    if not os.path.exists(autostart_file):
        desktop_files = list(base.load_data_paths("applications", 
            REDSHIFT_DESKTOP))
        if not desktop_files:
            raise IOError("Installed redshift desktop file not found!")
            return
        desktop_file_path = desktop_files[0]
        # Read installed file and modify it
        dfile = desktop.DesktopEntry(desktop_file_path)
    else:
        dfile = desktop.DesktopEntry(autostart_file)
    activestr = str(bool(active)).lower()
    # print "Setting autostart to %s" % activestr
    dfile.set(AUTOSTART_KEY, activestr)
    dfile.write(filename=autostart_file)
