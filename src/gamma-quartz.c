/* gamma-quartz.c -- Quartz (OSX) gamma adjustment
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
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <ApplicationServices/ApplicationServices.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "gamma-quartz.h"
#include "colorramp.h"


int
quartz_init(quartz_state_t *state)
{
	return 0;
}

int
quartz_start(quartz_state_t *state)
{
	return 0;
}

void
quartz_restore(quartz_state_t *state)
{
	CGDisplayRestoreColorSyncSettings();
}

void
quartz_free(quartz_state_t *state)
{
}

void
quartz_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps on OSX using Quartz.\n"), f);
	fputs("\n", f);
}

int
quartz_set_option(quartz_state_t *state, const char *key, const char *value)
{
	fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
	return -1;
}

static void
quartz_set_temperature_for_display(CGDirectDisplayID display, int temp,
				   float brightness, const float gamma[3])
{
	uint32_t ramp_size = CGDisplayGammaTableCapacity(display);
	if (ramp_size == 0) {
		fprintf(stderr, _("Gamma ramp size too small: %i\n"),
			ramp_size);
		return;
	}

	/* Create new gamma ramps */
	float *gamma_ramps = malloc(3*ramp_size*sizeof(float));
	if (gamma_ramps == NULL) {
		perror("malloc");
		return;
	}

	float *gamma_r = &gamma_ramps[0*ramp_size];
	float *gamma_g = &gamma_ramps[1*ramp_size];
	float *gamma_b = &gamma_ramps[2*ramp_size];

	colorramp_fill_float(gamma_r, gamma_g, gamma_b, ramp_size,
			     temp, brightness, gamma);

	CGError error =
		CGSetDisplayTransferByTable(display, ramp_size,
					    gamma_r, gamma_g, gamma_b);
	if (error != kCGErrorSuccess) {
		free(gamma_ramps);
		return;
	}

	free(gamma_ramps);
}

int
quartz_set_temperature(quartz_state_t *state, int temp, float brightness,
		       const float gamma[3])
{
	int r;
	CGError error;
	uint32_t display_count;

	error = CGGetOnlineDisplayList(0, NULL, &display_count);
	if (error != kCGErrorSuccess) return -1;

	CGDirectDisplayID* displays =
		malloc(sizeof(CGDirectDisplayID)*display_count);
	if (displays == NULL) {
		perror("malloc");
		return -1;
	}

	error = CGGetOnlineDisplayList(display_count, displays,
				       &display_count);
	if (error != kCGErrorSuccess) {
		free(displays);
		return -1;
	}

	for (int i = 0; i < display_count; i++) {
		quartz_set_temperature_for_display(displays[i], temp,
						   brightness, gamma);
	}

	free(displays);

	return 0;
}
