/* gamma-vidmode.c -- X VidMode gamma adjustment source
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

   Copyright (c) 2010-2014  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include "gamma-vidmode.h"
#include "redshift.h"
#include "colorramp.h"


int
vidmode_init(vidmode_state_t *state)
{
	state->screen_num = -1;
	state->saved_ramps = NULL;

	/* Open display */
	state->display = XOpenDisplay(NULL);
	if (state->display == NULL) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XOpenDisplay");
		return -1;
	}

	return 0;
}

int
vidmode_start(vidmode_state_t *state)
{
	int r;
	int screen_num = state->screen_num;

	if (screen_num < 0) screen_num = DefaultScreen(state->display);
	state->screen_num = screen_num;

	/* Query extension version */
	int major, minor;
	r = XF86VidModeQueryVersion(state->display, &major, &minor);
	if (!r) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XF86VidModeQueryVersion");
		return -1;
	}

	/* Request size of gamma ramps */
	r = XF86VidModeGetGammaRampSize(state->display, state->screen_num,
					&state->ramp_size);
	if (!r) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XF86VidModeGetGammaRampSize");
		return -1;
	}

	if (state->ramp_size == 0) {
		fprintf(stderr, _("Gamma ramp size too small: %i\n"),
			state->ramp_size);
		return -1;
	}

	/* Allocate space for saved gamma ramps */
	state->saved_ramps = malloc(3*state->ramp_size*sizeof(uint16_t));
	if (state->saved_ramps == NULL) {
		perror("malloc");
		return -1;
	}

	uint16_t *gamma_r = &state->saved_ramps[0*state->ramp_size];
	uint16_t *gamma_g = &state->saved_ramps[1*state->ramp_size];
	uint16_t *gamma_b = &state->saved_ramps[2*state->ramp_size];

	/* Save current gamma ramps so we can restore them at program exit. */
	r = XF86VidModeGetGammaRamp(state->display, state->screen_num,
				    state->ramp_size, gamma_r, gamma_g,
				    gamma_b);
	if (!r) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XF86VidModeGetGammaRamp");
		return -1;
	}

	return 0;
}

void
vidmode_free(vidmode_state_t *state)
{
	/* Free saved ramps */
	free(state->saved_ramps);

	/* Close display connection */
	XCloseDisplay(state->display);
}

void
vidmode_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with the X VidMode extension.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: VidMode help output
	   left column must not be translated */
	fputs(_("  screen=N\tX screen to apply adjustments to\n"), f);
	fputs("\n", f);
}

int
vidmode_set_option(vidmode_state_t *state, const char *key, const char *value)
{
	if (strcasecmp(key, "screen") == 0) {
		state->screen_num = atoi(value);
	} else {
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}

	return 0;
}

void
vidmode_restore(vidmode_state_t *state)
{
	uint16_t *gamma_r = &state->saved_ramps[0*state->ramp_size];
	uint16_t *gamma_g = &state->saved_ramps[1*state->ramp_size];
	uint16_t *gamma_b = &state->saved_ramps[2*state->ramp_size];

	/* Restore gamma ramps */
	int r = XF86VidModeSetGammaRamp(state->display, state->screen_num,
					state->ramp_size, gamma_r, gamma_g,
					gamma_b);
	if (!r) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XF86VidModeSetGammaRamp");
	}	
}

int
vidmode_set_temperature(vidmode_state_t *state,
			const color_setting_t *setting)
{
	int r;

	/* Create new gamma ramps */
	uint16_t *gamma_ramps = malloc(3*state->ramp_size*sizeof(uint16_t));
	if (gamma_ramps == NULL) {
		perror("malloc");
		return -1;
	}

	uint16_t *gamma_r = &gamma_ramps[0*state->ramp_size];
	uint16_t *gamma_g = &gamma_ramps[1*state->ramp_size];
	uint16_t *gamma_b = &gamma_ramps[2*state->ramp_size];

	/* Initialize gamma ramps to pure state */
	for (int i = 0; i < state->ramp_size; i++) {
		uint16_t value = (double)i/state->ramp_size * (UINT16_MAX+1);
		gamma_r[i] = value;
		gamma_g[i] = value;
		gamma_b[i] = value;
	}

	colorramp_fill(gamma_r, gamma_g, gamma_b, state->ramp_size,
		       setting);

	/* Set new gamma ramps */
	r = XF86VidModeSetGammaRamp(state->display, state->screen_num,
				    state->ramp_size, gamma_r, gamma_g,
				    gamma_b);
	if (!r) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XF86VidModeSetGammaRamp");
		free(gamma_ramps);
		return -1;
	}

	free(gamma_ramps);

	return 0;
}
