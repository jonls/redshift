/* location-manual.h -- Manual location provider header
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

#ifndef _REDSHIFT_LOCATION_MANUAL_H
#define _REDSHIFT_LOCATION_MANUAL_H


typedef struct {
	float lat;
	float lon;
} location_manual_state_t;


int location_manual_init(location_manual_state_t *state, char *args);
void location_manual_free(location_manual_state_t *state);
void location_manual_print_help(FILE *f);
int location_manual_get_location(location_manual_state_t *state, float *lat,
				 float *lon);


#endif /* ! _REDSHIFT_LOCATION_MANUAL_H */
