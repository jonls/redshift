/* settings.c -- Main program settings source
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

   Copyright (c) 2013  Jon Lund Steffensen <jonlst@gmail.com>
   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#include "settings.h"

#include <math.h>


void
settings_init(settings_t *settings)
{
  settings->temp_set = -1;
  settings->temp_day = -1;
  settings->temp_night = -1;
  settings->gamma[0] = NAN;
  settings->gamma[1] = NAN;
  settings->gamma[2] = NAN;
  settings->brightness_day = NAN;
  settings->brightness_night = NAN;
  settings->transition = -1;
  settings->transition_low = TRANSITION_LOW;
  settings->transition_high = TRANSITION_HIGH;
}

