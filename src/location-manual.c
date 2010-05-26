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
location_manual_init(location_manual_state_t *state, char *args)
{
	state->lat = NAN;
	state->lon = NAN;

	/* Parse arguments. */
	int count = 0;
	while (args != NULL) {
		if (count > 1) {
			fputs(_("Too many arguments.\n"), stderr);
			return -1;
		}

		char *next_arg = strchr(args, ':');
		if (next_arg != NULL) *(next_arg++) = '\0';

		/* Parse float value */
		char *end;
		errno = 0;
		float value = strtof(args, &end);
		if (errno != 0 || *end != '\0') {
			fputs(_("Malformed argument.\n"), stderr);
			return -1;
		}

		switch (count) {
		case 0: state->lat = value; break;
		case 1: state->lon = value; break;
		}

		args = next_arg;
		count += 1;
	}

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
}

void
location_manual_print_help(FILE *f)
{
	fputs(_("Specify location manually.\n"), f);
	fputs("\n", f);

	fputs(_("  First argument is latitude,\n"
		"  second argument is longitude\n"), f);
	fputs("\n", f);
}

int
location_manual_get_location(location_manual_state_t *state, float *lat,
			     float *lon)
{
	*lat = state->lat;
	*lon = state->lon;

	return 0;
}
