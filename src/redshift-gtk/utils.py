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
# Copyright (c) 2011  Jon Lund Steffensen <jonlst@gmail.com>

import os
from xdg import BaseDirectory as base
from xdg import DesktopEntry as desktop

REDSHIFT_DESKTOP = 'redshift-gtk.desktop'

# Keys to set when enabling/disabling autostart.
# Only first one is checked on "get".
AUTOSTART_KEYS = (('Hidden', ('true', 'false')),
                  ('X-GNOME-Autostart-enabled', ('false', 'true')))


def open_autostart_file():
    autostart_dir = base.save_config_path("autostart")
    autostart_file = os.path.join(autostart_dir, REDSHIFT_DESKTOP)

    if not os.path.exists(autostart_file):
        desktop_files = list(base.load_data_paths("applications",
                                                  REDSHIFT_DESKTOP))

        if not desktop_files:
            raise IOError("Installed redshift desktop file not found!")

        desktop_file_path = desktop_files[0]

        # Read installed file
        dfile = desktop.DesktopEntry(desktop_file_path)
        for key, values in AUTOSTART_KEYS:
            dfile.set(key, values[False])
        dfile.write(filename=autostart_file)
    else:
        dfile = desktop.DesktopEntry(autostart_file)

    return dfile, autostart_file

def get_autostart():
    dfile, path = open_autostart_file()
    check_key, check_values = AUTOSTART_KEYS[0]
    return dfile.get(check_key) == check_values[True]

def set_autostart(active):
    dfile, path = open_autostart_file()
    for key, values in AUTOSTART_KEYS:
        dfile.set(key, values[active])
    dfile.write(filename=path)
