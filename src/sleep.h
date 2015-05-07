/* sleep.h -- Sleep settings
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

   Copyright (c) 2014  Jon Lund Steffensen <jonlst@gmail.com>
   Copyright (c) 2015  Mattias Andrée <maandree@member.fsf.org>
*/
#ifndef REDSHIFT_SLEEP_H
#define REDSHIFT_SLEEP_H


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


/* Duration of sleep between screen updates (milliseconds). */
#ifndef SLEEP_DURATION
#  define SLEEP_DURATION        5000
#endif
#ifndef SLEEP_DURATION_SHORT
#  define SLEEP_DURATION_SHORT  100
#endif


extern unsigned int sleep_duration;
extern unsigned int sleep_duration_short;

void parse_sleep_string(const char *str, unsigned int *long_duration, unsigned int *short_duration);


#endif /* REDSHIFT_SLEEP_H */
