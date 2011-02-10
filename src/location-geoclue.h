/* location-geoclue.h -- Geoclue location provider header
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

   Copyright (c) 2010  Mathieu Trudel-Lapierre <mathieu-tl@ubuntu.com>
*/

#ifndef _REDSHIFT_LOCATION_GEOCLUE_H
#define _REDSHIFT_LOCATION_GEOCLUE_H

#include <stdio.h>
#include <geoclue/geoclue-position.h>

typedef struct {
	GeocluePosition *position;		/* main geoclue object */
	const char	*provider;		/* name of a geoclue provider */
	const char	*provider_path;		/* path of the geoclue provider */
} location_geoclue_state_t;

int location_geoclue_init(location_geoclue_state_t *state);
int location_geoclue_start(location_geoclue_state_t *state);
void location_geoclue_free(location_geoclue_state_t *state);

void location_geoclue_print_help(FILE *f);
int location_geoclue_set_option(location_geoclue_state_t *state,
				    const char *key, const char *value);

int location_geoclue_get_location(location_geoclue_state_t *state,
				      float *lat, float *lon);


#endif /* ! _REDSHIFT_LOCATION_GEOCLUE_H */
