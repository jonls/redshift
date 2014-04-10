/* location-manual.c -- Manual location provider source
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include "location-manual.h"

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif


int
location_manual_init(location_manual_state_t *state)
{
	state->lat = NAN;
	state->lon = NAN;

	return 0;
}

int
location_manual_start(location_manual_state_t *state)
{
	/* Latitude and longitude must be set */
	if (isnan(state->lat) || isnan(state->lon)) {
		fputs(_("Latitude and longitude must be set.\n"), stderr);
		exit(EXIT_FAILURE);
	}

	return 0;
}

void
location_manual_free(location_manual_state_t *state)
{
	(void) state;
}

void
location_manual_print_help(FILE *f)
{
	fputs(_("Specify location manually.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: Manual location help output
	   left column must not be translated */
	fputs(_("  lat=N\t\tLatitude\n"
		"  lon=N\t\tLongitude\n"), f);
	fputs("\n", f);
	fputs(_("Both values are expected to be floating point numbers,\n"
		"negative values representing west / south, respectively.\n"), f);
	fputs("\n", f);
}

int
location_manual_set_option(location_manual_state_t *state, const char *key,
			   const char *value)
{
	/* Parse float value */
	char *end;
	errno = 0;
	float v = strtof(value, &end);
	if (errno != 0 || *end != '\0') {
		fputs(_("Malformed argument.\n"), stderr);
		return -1;
	}

	if (strcasecmp(key, "lat") == 0) {
		state->lat = v;
	} else if (strcasecmp(key, "lon") == 0) {
		state->lon = v;
	} else {
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}

	return 0;
}

int
location_manual_get_location(location_manual_state_t *state, float *lat,
			     float *lon)
{
	*lat = state->lat;
	*lon = state->lon;

	return 0;
}
