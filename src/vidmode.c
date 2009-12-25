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

#include "colorramp.h"


int
vidmode_check_extension()
{
	int r;

	/* Open display */
	Display *dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "XOpenDisplay failed.\n");
		return -1;
	}

	/* Query extension version */
	int major, minor;
	r = XF86VidModeQueryVersion(dpy, &major, &minor);
	if (!r) {
		fprintf(stderr, "XF86VidModeQueryVersion failed.\n");
		return -1;
	}

	/* TODO Check major/minor */

	/* Close display */
	XCloseDisplay(dpy);

	return 0;
}

int
vidmode_set_temperature(int screen_num, int temp, float gamma[3])
{
	int r;

	/* Open display */
	Display *dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "XOpenDisplay failed.\n");
		return -1;
	}

	if (screen_num < 0) screen_num = DefaultScreen(dpy);

	/* Request size of gamma ramps */
	int gamma_ramp_size;
	r = XF86VidModeGetGammaRampSize(dpy, screen_num, &gamma_ramp_size);
	if (!r) {
		fprintf(stderr, "XF86VidModeGetGammaRampSize failed.\n");
		XCloseDisplay(dpy);
		return -1;
	}

	if (gamma_ramp_size == 0) {
		fprintf(stderr, "Gamma ramp size too small: %i\n",
			gamma_ramp_size);
		XCloseDisplay(dpy);
		return -1;
	}

	/* Create new gamma ramps */
	uint16_t *gamma_ramps = malloc(3*gamma_ramp_size*sizeof(uint16_t));
	if (gamma_ramps == NULL) abort();

	uint16_t *gamma_r = &gamma_ramps[0*gamma_ramp_size];
	uint16_t *gamma_g = &gamma_ramps[1*gamma_ramp_size];
	uint16_t *gamma_b = &gamma_ramps[2*gamma_ramp_size];

	colorramp_fill(gamma_r, gamma_g, gamma_b, gamma_ramp_size,
		       temp, gamma);

	/* Set new gamma ramps */
	r = XF86VidModeSetGammaRamp(dpy, screen_num, gamma_ramp_size,
				    gamma_r, gamma_g, gamma_b);
	if (!r) {
		fprintf(stderr, "XF86VidModeSetGammaRamp failed.\n");
		XCloseDisplay(dpy);
		return -1;
	}

	free(gamma_ramps);

	/* Close display */
	XCloseDisplay(dpy);

	return 0;
}
