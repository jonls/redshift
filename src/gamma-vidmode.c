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

#include "gamma-vidmode.h"

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


static void
vidmode_free_site(void *data)
{
	/* Close display connection. */
	XCloseDisplay((Display *)data);
}

static void
vidmode_free_partition(void *data)
{
	(void) data;
}

static int
vidmode_open_site(gamma_server_state_t *state, char *site, gamma_site_state_t *site_out)
{
	(void) state;

	/* Open display. */
	Display *display = XOpenDisplay(site);
	site_out->data = display;
	if (display == NULL) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XOpenDisplay");
		fprintf(stderr, _("Tried to open display `%s'\n"),
			site);
		return -1;
	}

	/* Query extension version. */
	int r, major, minor;
	r = XF86VidModeQueryVersion(display, &major, &minor);
	if (!r) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XF86VidModeQueryVersion");
		XCloseDisplay(display);
		return -1;
	}

	/* Get the number of available screens. */
	site_out->partitions_available = (size_t)ScreenCount(display);
	if (site_out->partitions_available < 1) {
		fprintf(stderr, _("X request failed: %s\n"),
			"ScreenCount");
	}

	return 0;
}

static int
vidmode_open_partition(gamma_server_state_t *state, gamma_site_state_t *site,
		       size_t partition, gamma_partition_state_t *partition_out)
{
	(void) state;
	(void) site;
	(void) partition;
	partition_out->data = (void *)partition;
	partition_out->crtcs_available = 1;
	return 0;
}

static int
vidmode_open_crtc(gamma_server_state_t *state, gamma_site_state_t *site,
		  gamma_partition_state_t *partition, size_t crtc, gamma_crtc_state_t *crtc_out)
{
	(void) state;
	(void) crtc;

	Display *display = site->data;
	int screen = (int)(size_t)(partition->data);
	int ramp_size;
	int r;

	crtc_out->data = NULL;

	/* Request size of gamma ramps. */
	r = XF86VidModeGetGammaRampSize(display, screen, &ramp_size);
	if (!r) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XF86VidModeGetGammaRampSize");
		return -1;
	}

	if (ramp_size < 2) {
		fprintf(stderr, _("Gamma ramp size too small: %i\n"),
			ramp_size);
		return -1;
	}

	crtc_out->saved_ramps.red_size   = (size_t)ramp_size;
	crtc_out->saved_ramps.green_size = (size_t)ramp_size;
	crtc_out->saved_ramps.blue_size  = (size_t)ramp_size;

	/* Allocate space for saved gamma ramps. */
	crtc_out->saved_ramps.red = malloc(3 * (size_t)ramp_size * sizeof(uint16_t));
	if (crtc_out->saved_ramps.red == NULL) {
		perror("malloc");
		return -1;
	}

	crtc_out->saved_ramps.green = crtc_out->saved_ramps.red + ramp_size;
	crtc_out->saved_ramps.blue = crtc_out->saved_ramps.green + ramp_size;

	/* Save current gamma ramps so we can restore them at program exit. */
	r = XF86VidModeGetGammaRamp(display, screen, (size_t)ramp_size,
				    crtc_out->saved_ramps.red,
				    crtc_out->saved_ramps.green,
				    crtc_out->saved_ramps.blue);
	if (!r) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XF86VidModeGetGammaRamp");
		return -1;
	}

	return 0;
}

static void
vidmode_invalid_partition(const gamma_site_state_t *site, size_t partition)
{
	fprintf(stderr, _("Screen %ld does not exist. "),
		partition);
	if (site->partitions_available > 1) {
		fprintf(stderr, _("Valid screens are [0-%ld].\n"),
			site->partitions_available - 1);
	} else {
		fprintf(stderr, _("Only screen 0 exists, did you mean CRTC %ld?\n"),
			partition);
		fprintf(stderr, _("If so, you need to use `randr' instead of `vidmode'.\n"));
	}
}

static int
vidmode_set_ramps(gamma_server_state_t *state, gamma_crtc_state_t *crtc, gamma_ramps_t ramps)
{
	int r;
	r = XF86VidModeSetGammaRamp((Display *)(state->sites[crtc->site_index].data), crtc->partition,
				    ramps.red_size, ramps.red, ramps.green, ramps.blue);
	if (!r) {
		fprintf(stderr, _("X request failed: %s\n"),
			"XF86VidModeSetGammaRamp");
	}
	return r;
}

static int
vidmode_set_option(gamma_server_state_t *state, const char *key, char *value, ssize_t section)
{
	if (strcasecmp(key, "screen") == 0) {
		ssize_t screen = strcasecmp(value, "all") ? (ssize_t)atoi(value) : -1;
		if (screen < 0 && strcasecmp(value, "all")) {
			/* TRANSLATORS: `all' must not be translated. */
			fprintf(stderr, _("Screen must be `all' or a non-negative integer.\n"));
			return -1;
		}
		if (section >= 0) {
			state->selections[section].partition = screen;
		} else {
			for (size_t i = 0; i < state->selections_made; i++)
				state->selections[i].partition = screen;
		}
		return 0;
	} else if (strcasecmp(key, "display") == 0) {
		if (section >= 0) {
			state->selections[section].site = strdup(value);
			if (state->selections[section].site == NULL)
				goto strdup_fail;
		} else {
			for (size_t i = 0; i < state->selections_made; i++) {
				state->selections[i].site = strdup(value);
				if (state->selections[i].site == NULL)
					goto strdup_fail;
			}
		}
		return 0;
	}
	return 1;

strdup_fail:
	perror("strdup");
	return -1;
}


int
vidmode_init(gamma_server_state_t *state)
{
	int r;
	r = gamma_init(state);
	if (r != 0) return r;

	state->selections->site    = getenv("DISPLAY") ? strdup(getenv("DISPLAY")) : NULL;
	state->free_site_data      = vidmode_free_site;
	state->free_partition_data = vidmode_free_partition;
	state->open_site           = vidmode_open_site;
	state->open_partition      = vidmode_open_partition;
	state->open_crtc           = vidmode_open_crtc;
	state->invalid_partition   = vidmode_invalid_partition;
	state->set_ramps           = vidmode_set_ramps;
	state->set_option          = vidmode_set_option;

	if (getenv("DISPLAY") != NULL && state->selections->site == NULL) {
		perror("strdup");
		return -1;
	}

	return 0;
}

int
vidmode_start(gamma_server_state_t *state)
{
	return gamma_resolve_selections(state);
}

void
vidmode_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with the X VidMode extension.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: VidMode help output
	   left column must not be translated. */
	fputs(_("  screen=N\tX screen to apply adjustments to\n"
		"  display=NAME\tX display to apply adjustments to\n"), f);
	fputs("\n", f);
}
