/* sleep.c -- Sleep settings
   This file is part of Redshift.

   Redshift is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Redshift is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Redshift.  If not, see <http://www.gnu.org/licenses/>.

   Copyright (c) 2015  Mattias Andrée <maandree@member.fsf.org>
*/

#include <string.h>
#include <stdlib.h>

#include "sleep.h"


unsigned int sleep_duration = 0;
unsigned int sleep_duration_short = 0;


/* A sleep duration string contains either one floating point,
   or two values, possibly empty strings, separated by a colon. */
void
parse_sleep_string(const char *str, unsigned int *long_duration, unsigned int *short_duration)
{
	char *s = strchr(str, ':');
	if (s == NULL) {
		*long_duration = (unsigned int)(1000 * atof(str));
	} else {
		*(s++) = '\0';
		if (*str) {
			*long_duration = (unsigned int)(1000 * atof(str));
		}
		if (*s) {
			*short_duration = (unsigned int)(1000 * atof(s));
		}
	}
}
