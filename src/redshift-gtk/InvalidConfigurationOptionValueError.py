# InvalidConfigurationOptionValue.py -- Raised for invalid configuration values
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


"""Exception for invalid values in a Redshift configuration file.

Note that this is exception is thrown by the Redshift's pythonic configuration
file parser (configuration.py) only if a value, that is not validated by
Redshift's built-in C-based option parser, is invalid.
"""

class InvalidConfigurationOptionValueError(Exception):
    def __init__(self, file_path, section, option, invalid_value, 
        correct_values=[], additional_explanation=''):
            self.file_path = file_path
            self.section = section
            self.option = option
            self.invalid_value = invalid_value
            self.correct_values = correct_values
            self.additional_explanation = additional_explanation

    def __str__(self):
        message = 'Invalid configuration option value detected for ' \
            'configuration option "%s" in section "%s" of file "%s": ' \
            '"%s"' % (self.option, self.section, self.file_path, 
                self.invalid_value)
        if self.correct_values:
            correct_values_strings = list(
                map(lambda value: '"%s"' % value, self.correct_values)
            )
            message += ' (correct values would be %s)' % \
                ', '.join(correct_values_strings)
        
        message += '.'

        if self.additional_explanation:
            message += ' ' + self.additional_explanation

        if not message.endswith('.'):
            message += '.'

        return message
