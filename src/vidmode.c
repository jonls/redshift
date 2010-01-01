/* vidmode.c -- X VidMode gamma adjustment source
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

   Copyright (c) 2009  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include "vidmode.h"
#include "colorramp.h"


int
vidmode_init(vidmode_state_t *state, int screen_num)
{
	int r;

	/* Open display */
	state->display = XOpenDisplay(NULL);
	if (state->display == NULL) {
		fprintf(stderr, "XOpenDisplay failed.\n");
		return -1;
	}

	if (screen_num < 0) screen_num = DefaultScreen(state->display);
	state->screen_num = screen_num;

	/* Query extension version */
	int major, minor;
	r = XF86VidModeQueryVersion(state->display, &major, &minor);
	if (!r) {
		fprintf(stderr, "XF86VidModeQueryVersion failed.\n");
		XCloseDisplay(state->display);
		return -1;
	}

	return 0;
}

void
vidmode_free(vidmode_state_t *state)
{
	/* Close display connection */
	XCloseDisplay(state->display);
}

void
vidmode_restore(vidmode_state_t *state)
{
}

int
vidmode_set_temperature(vidmode_state_t *state, int temp, float gamma[3])
{
	int r;

	/* Request size of gamma ramps */
	int ramp_size;
	r = XF86VidModeGetGammaRampSize(state->display, state->screen_num,
					&ramp_size);
	if (!r) {
		fprintf(stderr, "XF86VidModeGetGammaRampSize failed.\n");
		return -1;
	}

	if (ramp_size == 0) {
		fprintf(stderr, "Gamma ramp size too small: %i\n",
			ramp_size);
		return -1;
	}

	/* Create new gamma ramps */
	uint16_t *gamma_ramps = malloc(3*ramp_size*sizeof(uint16_t));
	if (gamma_ramps == NULL) {
		perror("malloc");
		return -1;
	}

	uint16_t *gamma_r = &gamma_ramps[0*ramp_size];
	uint16_t *gamma_g = &gamma_ramps[1*ramp_size];
	uint16_t *gamma_b = &gamma_ramps[2*ramp_size];

	colorramp_fill(gamma_r, gamma_g, gamma_b, ramp_size, temp, gamma);

	/* Set new gamma ramps */
	r = XF86VidModeSetGammaRamp(state->display, state->screen_num,
				    ramp_size, gamma_r, gamma_g, gamma_b);
	if (!r) {
		fprintf(stderr, "XF86VidModeSetGammaRamp failed.\n");
		free(gamma_ramps);
		return -1;
	}

	free(gamma_ramps);

	return 0;
}
