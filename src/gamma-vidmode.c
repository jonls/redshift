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

   Copyright (c) 2010  Jon Lund Steffensen <jonlst@gmail.com>
   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
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
#include "colorramp.h"
#include "redshift.h"


int
vidmode_init(vidmode_state_t *state)
{
	state->selection_count = 0;
	state->selections = malloc(1 * sizeof(vidmode_selection_t));
	if (state->selections == NULL) {
		perror("malloc");
		return -1;
	}
	state->selections->screen_num = -1;
	state->selections->gamma[0] = DEFAULT_GAMMA;
	state->selections->gamma[1] = DEFAULT_GAMMA;
	state->selections->gamma[2] = DEFAULT_GAMMA;
	state->screens_used = 0;
	state->screens = NULL;

	/* Open display */
	state->display = XOpenDisplay(NULL);
	if (state->display == NULL) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XOpenDisplay");
		free(state->selections);
		state->selections = NULL;
		return -1;
	}

	state->screen_count = ScreenCount(state->display);
	if (state->screen_count < 1) {
		fprintf(stderr, _("X request failed: %s\n"),
			"ScreenCount");
		free(state->selections);
		state->selections = NULL;
		return -1;
	}

	return 0;
}

int
vidmode_start(vidmode_state_t *state)
{
	int r;

	/* Query extension version */
	int major, minor;
	r = XF86VidModeQueryVersion(state->display, &major, &minor);
	if (!r) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XF86VidModeQueryVersion");
		return -1;
	}

	/* Use default selection if no other selection is made. */
	if (state->selection_count == 0) {
		/* Select screens. */
		if (state->selections->screen_num != -1) {
			state->selections = realloc(state->selections, 2 * sizeof(vidmode_selection_t));
			state->selection_count = 1;
			if (state->selections != NULL)
				state->selections[1] = state->selections[0];
		} else {
			state->selections = realloc(state->selections,
						    (state->screen_count + 1) * sizeof(vidmode_selection_t));
			state->selection_count = state->screen_count;
			if (state->selections != NULL) {
				int i;
				for (i = 1; i <= state->screen_count; i++) {
					state->selections[i] = *(state->selections);
					state->selections[i].screen_num = i - 1;
				}
			}
		}
		if (state->selections == NULL) {
			perror("realloc");
			return -1;
		}
	}

	int selection_index;
	for (selection_index = 1; selection_index <= state->selection_count; selection_index++) {
		vidmode_selection_t *selection = state->selections + selection_index;

		/* Prepare for adding another screen */
		if (state->screens_used++ == 0)
			state->screens = malloc(1 * sizeof(vidmode_screen_state_t));
		else
			state->screens = realloc(state->screens,
						 state->screens_used * sizeof(vidmode_screen_state_t));
		if (state->screens == NULL) {
			perror(state->screens_used == 1 ? "malloc" : "realloc");
			return -1;
		}

		/* Create entry for other selected CRTC */
		vidmode_screen_state_t *screen = state->screens + state->screens_used - 1;
		screen->screen_num = selection->screen_num;
		screen->ramp_size = -1;
		screen->saved_ramps = NULL;
		screen->gamma[0] = selection->gamma[0];
		screen->gamma[1] = selection->gamma[1];
		screen->gamma[2] = selection->gamma[2];
		screen->gamma_r = NULL;
		screen->gamma_g = NULL;
		screen->gamma_b = NULL;

		/* Request size of gamma ramps */
		r = XF86VidModeGetGammaRampSize(state->display, screen->screen_num,
						&screen->ramp_size);
		if (!r) {
			fprintf(stderr, _("X request failed: %s\n"),
				"XF86VidModeGetGammaRampSize");
			return -1;
		}

		if (screen->ramp_size <= 1) {
			fprintf(stderr, _("Gamma ramp size too small: %i\n"),
				screen->ramp_size);
			return -1;
		}

		/* Allocate space for saved gamma ramps */
		screen->saved_ramps = malloc(3*screen->ramp_size*sizeof(uint16_t));
		if (screen->saved_ramps == NULL) {
			perror("malloc");
			return -1;
		}

		uint16_t *gamma_r = &screen->saved_ramps[0*screen->ramp_size];
		uint16_t *gamma_g = &screen->saved_ramps[1*screen->ramp_size];
		uint16_t *gamma_b = &screen->saved_ramps[2*screen->ramp_size];

		/* Save current gamma ramps so we can restore them at program exit. */
		r = XF86VidModeGetGammaRamp(state->display, screen->screen_num,
					    screen->ramp_size, gamma_r, gamma_g,
					    gamma_b);
		if (!r) {
			fprintf(stderr, _("X request failed: %s\n"),
				"XF86VidModeGetGammaRamp");
			return -1;
		}

		/* Allocate space for gamma ramps */
		uint16_t *gamma_ramps = malloc(3*screen->ramp_size*sizeof(uint16_t));
		if (gamma_ramps == NULL) {
			perror("malloc");
			return -1;
		}

		screen->gamma_r = &gamma_ramps[0*screen->ramp_size];
		screen->gamma_g = &gamma_ramps[1*screen->ramp_size];
		screen->gamma_b = &gamma_ramps[2*screen->ramp_size];
	}

	return 0;
}

void
vidmode_free(vidmode_state_t *state)
{
	/* Free ramps */
	for (int i = 0; i < state->screens_used; i++) {
		if (state->screens[i].saved_ramps != NULL)
			free(state->screens[i].saved_ramps);
		if (state->screens[i].gamma_r != NULL)
			free(state->screens[i].gamma_r);
	}

	/* Free screen selections */
	if (state->screens != NULL) {
		free(state->screens);
		state->screens = NULL;
	}

	/* Free raw selection information */
	if (state->selections != NULL) {
		free(state->selections);
		state->selections = NULL;
	}

	/* Close display connection */
	if (state->display != NULL) {
		XCloseDisplay(state->display);
		state->display = NULL;
	}
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
vidmode_set_option(vidmode_state_t *state, const char *key, const char *value, int section)
{
	if (section == state->selection_count) {
		state->selections = realloc(state->selections,
					    (++(state->selection_count) + 1) * sizeof(vidmode_selection_t));

		if (state->selections == NULL) {
			perror("realloc");
			return -1;
		}

		state->selections[section + 1] = *(state->selections);
		state->selections[section + 1].screen_num = 0;
	}

	if (strcasecmp(key, "screen") == 0) {
		int screen_num = atoi(value);
		state->selections[section + 1].screen_num = screen_num;
		if (screen_num < 0) {
			fprintf(stderr, _("Screen must be a non-negative integer.\n"));
			return -1;
		}
		if (screen_num >= state->screen_count) {
			fprintf(stderr, _("Screen %d does not exist. "),
				screen_num);
			if (state->screen_count > 1) {
				fprintf(stderr, _("Valid screens are [0-%d].\n"),
					state->screen_count - 1);
			} else {
				fprintf(stderr, _("Only screen 0 exists.\n"));
			}
			fprintf(stderr, "Invalid screen.\n");
			return -1;
		}
	} else if (strcasecmp(key, "gamma") == 0) {
		float gamma[3];
		if (parse_gamma_string(value, gamma) < 0) {
			fputs(_("Malformed gamma setting.\n"),
			      stderr);
			return -1;
		}
		if (gamma[0] < MIN_GAMMA || gamma[0] > MAX_GAMMA ||
		    gamma[1] < MIN_GAMMA || gamma[1] > MAX_GAMMA ||
		    gamma[2] < MIN_GAMMA || gamma[2] > MAX_GAMMA) {
			fprintf(stderr,
				_("Gamma value must be between %.1f and %.1f.\n"),
				MIN_GAMMA, MAX_GAMMA);
			return -1;
		}
		state->selections[section + 1].gamma[0] = gamma[0];
		state->selections[section + 1].gamma[1] = gamma[1];
		state->selections[section + 1].gamma[2] = gamma[2];
	} else {
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}

	return 0;
}

void
vidmode_restore(vidmode_state_t *state)
{
	int screen_index;
	for (screen_index = 0; screen_index < state->screens_used; screen_index++) {
		vidmode_screen_state_t *screen = state->screens + screen_index;

		uint16_t *gamma_r = &screen->saved_ramps[0*screen->ramp_size];
		uint16_t *gamma_g = &screen->saved_ramps[1*screen->ramp_size];
		uint16_t *gamma_b = &screen->saved_ramps[2*screen->ramp_size];

		/* Restore gamma ramps */
		int r = XF86VidModeSetGammaRamp(state->display, screen->screen_num,
						screen->ramp_size, gamma_r, gamma_g,
						gamma_b);
		if (!r) {
			fprintf(stderr, _("X request failed: %s\n"),
				"XF86VidModeSetGammaRamp");
		}
	}
}

int
vidmode_set_temperature(vidmode_state_t *state, int temp, float brightness)
{
	int screen_index;
	for (screen_index = 0; screen_index < state->screens_used; screen_index++) {
		vidmode_screen_state_t *screen = state->screens + screen_index;
		int r;

		/* Create new gamma ramps */
		colorramp_fill(screen->gamma_r, screen->gamma_g, screen->gamma_b,
			       screen->ramp_size, temp, brightness, screen->gamma);

		/* Set new gamma ramps */
		r = XF86VidModeSetGammaRamp(state->display, screen->screen_num,
					    screen->ramp_size, screen->gamma_r,
					    screen->gamma_g, screen->gamma_b);
		if (!r) {
			fprintf(stderr, _("X request failed: %s\n"),
				"XF86VidModeSetGammaRamp");
			return -1;
		}
	}

	return 0;
}
