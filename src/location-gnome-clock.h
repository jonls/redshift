/* location-gnome-clock.h -- GNOME Panel Clock location provider header
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

   Copyright (c) 2010  Jon Lund Steffensen <jonlst@gmail.com>
*/

#ifndef _REDSHIFT_LOCATION_GNOME_CLOCK_H
#define _REDSHIFT_LOCATION_GNOME_CLOCK_H

#include <stdio.h>


typedef struct {
	float lat;
	float lon;
} location_gnome_clock_state_t;


int location_gnome_clock_init(location_gnome_clock_state_t *state);
int location_gnome_clock_start(location_gnome_clock_state_t *state);
void location_gnome_clock_free(location_gnome_clock_state_t *state);

void location_gnome_clock_print_help(FILE *f);
int location_gnome_clock_set_option(location_gnome_clock_state_t *state,
				    const char *key, const char *value);

int location_gnome_clock_get_location(location_gnome_clock_state_t *state,
				      float *lat, float *lon);


#endif /* ! _REDSHIFT_LOCATION_GNOME_CLOCK_H */
