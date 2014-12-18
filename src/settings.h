/* settings.h -- Main program settings header
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

#ifndef REDSHIFT_SETTINGS_H
#define REDSHIFT_SETTINGS_H

#include "solar.h"


/* Bounds for parameters. */
#define MIN_TEMP   1000
#define MAX_TEMP  25000
#define MIN_BRIGHTNESS  0.1
#define MAX_BRIGHTNESS  1.0
#define MIN_GAMMA   0.1
#define MAX_GAMMA  10.0

/* Angular elevation of the sun at which the color temperature
   transition period starts and ends (in degress).
   Transition during twilight, and while the sun is lower than
   3.0 degrees above the horizon. */
#define TRANSITION_LOW     SOLAR_CIVIL_TWILIGHT_ELEV
#define TRANSITION_HIGH    3.0

/* Default values for parameters. */
#define DEFAULT_DAY_TEMP    5500
#define DEFAULT_NIGHT_TEMP  3500
#define DEFAULT_BRIGHTNESS   1.0
#define DEFAULT_GAMMA        1.0


typedef struct
{
  int temp_set;
  int temp_day;
  int temp_night;
  float gamma[3];
  float brightness_day;
  float brightness_night;
  int transition;
  float transition_low;
  float transition_high;
  int reload_transition;
  
} settings_t;


/* A gamma string contains either one floating point value,
   or three values separated by colon. */
int parse_gamma_string(const char *str, float gamma[]);

void settings_init(settings_t *settings);
void settings_copy(settings_t *restrict dest, const settings_t *restrict src);
void settings_finalize(settings_t *settings);
int settings_parse(settings_t *settings, const char* name, char* value);
int settings_validate(settings_t *settings, int manual_mode, int reset_mode);
void settings_interpolate(settings_t *out, settings_t low, settings_t high, double weight);


#endif /* ! REDSHIFT_SETTINGS_H */
