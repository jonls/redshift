#!/usr/bin/env python3

# configuration.py -- Determine and parse Redshift configuration
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

# Copyright (c) 2018  Florian Rademacher <florian.rademacher@fh-dortmund.de>


"""Parser for Redshift configuration files.

The parser will first try to determine the path of Redshift's configuration 
file. Therefore, it uses the same logic as config-ini.c, i.e., the source
file that contains the INI config file parser of Redshift's C-based 
implementation. The configuration parser will hence apply the following 
rules in the given order to determine the path to the configuration file:
    1) Command line argument -c is given
            Use the given file as configuration file.

    2) XDG_CONFIG_HOME is set
            Look for a file named "redshift.conf" in XDG_CONFIG_HOME.

    3) Only on Windows: Use content of environment variable "localappdata"
            Look for a file named "redshift.conf" in "localappdata".

    4) Use content of environment variable "HOME"
            Look for a file named ".config/redshift.conf" in "HOME".

    5) Only on *nix: Look for a file named ".config/redshift.conf" in "~".

    6) Only on *nix: XDG_CONFIG_DIRS is set
            Look for a file named "redshift.conf" in one of the folders of
            XDG_CONFIG_DIRS.

    7) Only on *nix: Look for a file named "redshift.conf" in "/etc".

If one of the cases above applies and an existing configuration file could be
determined, the parser will parse it and provide callers with the parsed
configuration entries.
"""

import argparse
import os
import platform
import sys

from configparser import ConfigParser, NoSectionError

try:
    from xdg.BaseDirectory import xdg_config_home, xdg_config_dirs
    has_xdg = True
except ImportError:
    has_xdg = False


class RedshiftConfiguration(object):
    """Class that represents a parsed Redshift configuraton."""

    # According to the Python doc of the configparser module
    _BOOLEAN_LIKE_OPTION_VALUE_MAPPING = {
        '1' : True,  
        '0' : False,

        'yes' : True,    
        'no'  : False, 

        'true'  : True, 
        'false' : False, 

        'on'  : True,
        'off' : False
    }

    def determine_configuration_file_path(self, args=[]):
        """Determine path of configuration file or None, if no file could be.
        
        The parameter args is a list of command line arguments passed to
        redshift-gtk.
        """
        self._args = []
        if isinstance(args, list):
            self._args = args[1:]            

        self._argument_parser = argparse.ArgumentParser()
        self._argument_parser.add_argument('-c', dest='configuration_file')

        configuration_file_path_methods = [
            self._configuration_file_path_from_command_line,
            self._configuration_file_path_from_xdg_config_home,
            self._windows_configuration_file_path_from_localappdata,
            self._configuration_file_path_from_env_home,
            self._nix_configuration_file_path_from_home,
            self._nix_configuration_file_path_from_xdg_config_dirs,
            self._nix_configuration_file_path_from_etc
        ]
        method_index = 0
        last_method_index = len(configuration_file_path_methods) - 1

        existing_configuration_file_path = None
        while method_index <= last_method_index \
            and not existing_configuration_file_path:
            method = configuration_file_path_methods[method_index]            
            existing_configuration_file_path = \
                self._returns_existing_configuration_file(method)
            method_index += 1

        return existing_configuration_file_path

    def parse_configuration(self, configuration_file_path):
        """Parse existing configuration file."""
        if not configuration_file_path:
            raise ValueError('No file specified for parsing.')

        if not self._file_exists(configuration_file_path):
            raise ValueError('File "%s" does not exist.' % \
                configuration_file_path)
         
        try:
            self._parsed_configuration = ConfigParser()
            self._parsed_configuration.read(configuration_file_path)
            self._parsed_configuration_is_valid = True            
        except Exception as ex:
            self._parsed_configuration = None
            self._parsed_configuration_is_valid = False
            raise ex

    def __getitem__(self, items):
        """Make the configuration behave like a dictionary.

        The method takes a string key or a tuple of string keys, which indicate
        the section names for which configuration option values shall be 
        returned:
            - If a string is passed to the method, it returns a dictionary
              (key: option name, value: option value as string).
            - If a tuple is passed to the method, it returns a nested 
              dictionary (key: section name, value: option dictionary for the
              section).
        """
        if not items \
            or (not isinstance(items, str) and not isinstance(items, tuple)) \
            or not self._parsed_configuration:
            raise KeyError(items)

        if isinstance(items, str):
            return self._get_configuration_options_str(items, 
                self._parsed_configuration)
        elif isinstance(items, tuple):
            return self._get_configuration_options_tuple(items,
                self._parsed_configuration)

    def _is_parsed_configuration(self):
        """Helper to determine if this object represents a parsed 
        configuration.
        """
        try:
            return self._parsed_configuration is not None
        except AttributeError:
            return False

    def _is_valid_configuration(self):
        """Helper to determine if this object represents a valid 
        configuration.
        """
        try:
            return self._is_parsed_configuration \
                and self._parsed_configuration_is_valid
        except AttributeError:
            return False

    def _get_configuration_options_str(self, section, parsed_configuration):
        """Get configuration options as list of tuples for a single section."""
        section_options = {}
        try:
            section_options = parsed_configuration.items(section)
        except NoSectionError:
            raise KeyError(section)

        return {option_name : option_value 
                for option_name, option_value in section_options}

    def _get_configuration_options_tuple(self, sections, parsed_configuration):
        """Get configuration options as dictionary (key: section name, value: 
        list of section options' value tuples) for a set of sections."""
        result_options = {}

        for section_name in sections:
            configuration_options_for_section = \
                self._get_configuration_options_str(section_name, 
                    parsed_configuration)
            result_options[section_name] = configuration_options_for_section

        return result_options

    def _returns_existing_configuration_file(self, file_path_method):
        """Execute a method that returns a possible configuration file and
        return a string containing the path of an existing configuration file
        or None otherwise.
        """
        file_path = file_path_method()

        if type(file_path) == str and self._file_exists(file_path):
            return file_path
        elif type(file_path) == list:
            return self._first_existing_file(file_path)
        else:
            return None

    def _configuration_file_path_from_command_line(self):
        """Path for configuration file specified via the command line."""
        parsed_arguments = self._argument_parser.parse_args(self._args)
        try:
            return parsed_arguments.configuration_file
        except AttributeError:
            return None

    def _configuration_file_path_from_xdg_config_home(self):
        """Path for configuration file in XDG_CONFIG_HOME."""
        if has_xdg and xdg_config_home:
            return os.path.join(xdg_config_home, 'redshift.conf')
        else:
            return None

    def _windows_configuration_file_path_from_localappdata(self):
        """Windows only: Path for configuration file in localappdata."""
        if platform.system() != 'Windows':
            return None

        localappdata_folder = os.environ.get('localappdata')
        if localappdata_folder:
            return os.path.join(localappdata_folder, 'redshift.conf')
        else:
            return None

    def _configuration_file_path_from_env_home(self):
        """Path for configuration file at the place determined by the HOME 
        environment variable.
        """
        home_folder = os.environ.get('HOME')
        if home_folder:
            return os.path.join(home_folder, '.config', 'redshift.conf')
        else:
            return None

    def _nix_configuration_file_path_from_home(self):
        """*nix only: Path for configuration file at ~."""
        if not self._is_unix_like_platform:
            return None

        home_folder = os.environ.get('HOME')
        if home_folder:
            return os.path.join(home_folder, '.config', 'redshift.conf')
        else:
            return None

    def _nix_configuration_file_path_from_xdg_config_dirs(self):
        """*nix only: Paths for configuration file in XDG_CONFIG_DIRS."""
        if not self._is_unix_like_platform:
            return None

        if has_xdg and xdg_config_dirs:
            return [os.path.join(d, 'redshift.conf') for d in xdg_config_dirs]
        else:
            return None

    def _nix_configuration_file_path_from_etc(self):
        """*nix only: Path for configuration file in /etc."""
        if not self._is_unix_like_platform:
            return None

        return os.path.join(' ', 'etc', 'redshift.conf').lstrip()

    def _is_unix_like_platform(self):
        """Determine if the platform on which Redshift runs, is Unix-like."""
        non_unix_like_platforms = ['win32', 'cygwin']
        
        # We use sys.platform() instead of platform.system(), because, 
        # according to the Python docs, it seems that it's non Unix-like return
        # values are limited to "win32" and "cygwin". We consider "darwin",
        # i.e., Mac OS X, as being Unix-like, too.
        platform = sys.platform()
        return platform not in non_unix_like_platforms

    def _first_existing_file(self, file_path_list):
        """Return the first existing file from the given list of file paths or
        None otherwise.
        """
        existing_files = list(filter(self._file_exists, file_path_list))
        if existing_files:
            return existing_files[0]
        else:
            return None

    def _file_exists(self, file_path):
        """Test if a file exists using a try-except construct to avoid race
        conditions.
        """
        try:
            f = open(file_path)
            f.close()
            return True
        except FileNotFoundError:
            return False


if __name__ == '__main__':
    rc = RedshiftConfiguration()
    try:
        config_file_path = rc.determine_configuration_file_path(sys.argv)
        rc.parse_configuration(config_file_path)
    except Exception as ex:
        raise ex
        print(str(ex))
