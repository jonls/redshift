/* gamma-randr.c -- X RANDR gamma adjustment source
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

   Copyright (c) 2010-2017  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include "gamma-randr.h"
#include "redshift.h"
#include "colorramp.h"


#define RANDR_VERSION_MAJOR  1
#define RANDR_VERSION_MINOR  3


typedef struct {
	xcb_randr_crtc_t crtc;
	unsigned int ramp_size;
	uint16_t *saved_ramps;
} randr_crtc_state_t;

typedef struct {
	xcb_connection_t *conn;
	xcb_screen_t *screen;
	int preferred_screen;
	int screen_num;
	int crtc_num_count;
	int* crtc_num;
	unsigned int crtc_count;
	randr_crtc_state_t *crtcs;
} randr_state_t;


static int
randr_init(randr_state_t **state)
{
	/* Initialize state. */
	*state = malloc(sizeof(randr_state_t));
	if (*state == NULL) return -1;

	randr_state_t *s = *state;
	s->screen_num = -1;
	s->crtc_num = NULL;

	s->crtc_num_count = 0;
	s->crtc_count = 0;
	s->crtcs = NULL;

	xcb_generic_error_t *error;

	/* Open X server connection */
	s->conn = xcb_connect(NULL, &s->preferred_screen);

	/* Query RandR version */
	xcb_randr_query_version_cookie_t ver_cookie =
		xcb_randr_query_version(s->conn, RANDR_VERSION_MAJOR,
					RANDR_VERSION_MINOR);
	xcb_randr_query_version_reply_t *ver_reply =
		xcb_randr_query_version_reply(s->conn, ver_cookie, &error);

	/* TODO What does it mean when both error and ver_reply is NULL?
	   Apparently, we have to check both to avoid seg faults. */
	if (error || ver_reply == NULL) {
		int ec = (error != 0) ? error->error_code : -1;
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Query Version", ec);
		xcb_disconnect(s->conn);
		free(s);
		return -1;
	}

	if (ver_reply->major_version != RANDR_VERSION_MAJOR ||
	    ver_reply->minor_version < RANDR_VERSION_MINOR) {
		fprintf(stderr, _("Unsupported RANDR version (%u.%u)\n"),
			ver_reply->major_version, ver_reply->minor_version);
		free(ver_reply);
		xcb_disconnect(s->conn);
		free(s);
		return -1;
	}

	free(ver_reply);

	return 0;
}

static int
randr_start(randr_state_t *state)
{
	xcb_generic_error_t *error;

	int screen_num = state->screen_num;
	if (screen_num < 0) screen_num = state->preferred_screen;

	/* Get screen */
	const xcb_setup_t *setup = xcb_get_setup(state->conn);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	state->screen = NULL;

	for (int i = 0; iter.rem > 0; i++) {
		if (i == screen_num) {
			state->screen = iter.data;
			break;
		}
		xcb_screen_next(&iter);
	}

	if (state->screen == NULL) {
		fprintf(stderr, _("Screen %i could not be found.\n"),
			screen_num);
		return -1;
	}

	/* Get list of CRTCs for the screen */
	xcb_randr_get_screen_resources_current_cookie_t res_cookie =
		xcb_randr_get_screen_resources_current(state->conn,
						       state->screen->root);
	xcb_randr_get_screen_resources_current_reply_t *res_reply =
		xcb_randr_get_screen_resources_current_reply(state->conn,
							     res_cookie,
							     &error);

	if (error) {
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Get Screen Resources Current",
			error->error_code);
		return -1;
	}

	state->crtc_count = res_reply->num_crtcs;
	state->crtcs = calloc(state->crtc_count, sizeof(randr_crtc_state_t));
	if (state->crtcs == NULL) {
		perror("malloc");
		state->crtc_count = 0;
		return -1;
	}

	xcb_randr_crtc_t *crtcs =
		xcb_randr_get_screen_resources_current_crtcs(res_reply);

	/* Save CRTC identifier in state */
	for (int i = 0; i < state->crtc_count; i++) {
		state->crtcs[i].crtc = crtcs[i];
	}

	free(res_reply);

	/* Save size and gamma ramps of all CRTCs.
	   Current gamma ramps are saved so we can restore them
	   at program exit. */
	for (int i = 0; i < state->crtc_count; i++) {
		xcb_randr_crtc_t crtc = state->crtcs[i].crtc;

		/* Request size of gamma ramps */
		xcb_randr_get_crtc_gamma_size_cookie_t gamma_size_cookie =
			xcb_randr_get_crtc_gamma_size(state->conn, crtc);
		xcb_randr_get_crtc_gamma_size_reply_t *gamma_size_reply =
			xcb_randr_get_crtc_gamma_size_reply(state->conn,
							    gamma_size_cookie,
							    &error);

		if (error) {
			fprintf(stderr, _("`%s' returned error %d\n"),
				"RANDR Get CRTC Gamma Size",
				error->error_code);
			return -1;
		}

		unsigned int ramp_size = gamma_size_reply->size;
		state->crtcs[i].ramp_size = ramp_size;

		free(gamma_size_reply);

		if (ramp_size == 0) {
			fprintf(stderr, _("Gamma ramp size too small: %i\n"),
				ramp_size);
			return -1;
		}

		/* Request current gamma ramps */
		xcb_randr_get_crtc_gamma_cookie_t gamma_get_cookie =
			xcb_randr_get_crtc_gamma(state->conn, crtc);
		xcb_randr_get_crtc_gamma_reply_t *gamma_get_reply =
			xcb_randr_get_crtc_gamma_reply(state->conn,
						       gamma_get_cookie,
						       &error);

		if (error) {
			fprintf(stderr, _("`%s' returned error %d\n"),
				"RANDR Get CRTC Gamma", error->error_code);
			return -1;
		}

		uint16_t *gamma_r =
			xcb_randr_get_crtc_gamma_red(gamma_get_reply);
		uint16_t *gamma_g =
			xcb_randr_get_crtc_gamma_green(gamma_get_reply);
		uint16_t *gamma_b =
			xcb_randr_get_crtc_gamma_blue(gamma_get_reply);

		/* Allocate space for saved gamma ramps */
		state->crtcs[i].saved_ramps =
			malloc(3*ramp_size*sizeof(uint16_t));
		if (state->crtcs[i].saved_ramps == NULL) {
			perror("malloc");
			free(gamma_get_reply);
			return -1;
		}

		/* Copy gamma ramps into CRTC state */
		memcpy(&state->crtcs[i].saved_ramps[0*ramp_size], gamma_r,
		       ramp_size*sizeof(uint16_t));
		memcpy(&state->crtcs[i].saved_ramps[1*ramp_size], gamma_g,
		       ramp_size*sizeof(uint16_t));
		memcpy(&state->crtcs[i].saved_ramps[2*ramp_size], gamma_b,
		       ramp_size*sizeof(uint16_t));

		free(gamma_get_reply);
	}

	return 0;
}

static void
randr_restore(randr_state_t *state)
{
	xcb_generic_error_t *error;

	/* Restore CRTC gamma ramps */
	for (int i = 0; i < state->crtc_count; i++) {
		xcb_randr_crtc_t crtc = state->crtcs[i].crtc;

		unsigned int ramp_size = state->crtcs[i].ramp_size;
		uint16_t *gamma_r = &state->crtcs[i].saved_ramps[0*ramp_size];
		uint16_t *gamma_g = &state->crtcs[i].saved_ramps[1*ramp_size];
		uint16_t *gamma_b = &state->crtcs[i].saved_ramps[2*ramp_size];

		/* Set gamma ramps */
		xcb_void_cookie_t gamma_set_cookie =
			xcb_randr_set_crtc_gamma_checked(state->conn, crtc,
							 ramp_size, gamma_r,
							 gamma_g, gamma_b);
		error = xcb_request_check(state->conn, gamma_set_cookie);

		if (error) {
			fprintf(stderr, _("`%s' returned error %d\n"),
				"RANDR Set CRTC Gamma", error->error_code);
			fprintf(stderr, _("Unable to restore CRTC %i\n"), i);
		}
	}
}

static void
randr_free(randr_state_t *state)
{
	/* Free CRTC state */
	for (int i = 0; i < state->crtc_count; i++) {
		free(state->crtcs[i].saved_ramps);
	}
	free(state->crtcs);
	free(state->crtc_num);

	/* Close connection */
	xcb_disconnect(state->conn);

	free(state);
}

static void
randr_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with the X RANDR extension.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: RANDR help output
	   left column must not be translated */
	fputs(_("  screen=N\t\tX screen to apply adjustments to\n"
		"  crtc=N\tList of comma separated CRTCs to apply"
		" adjustments to\n"),
	      f);
	fputs("\n", f);
}

static int
randr_set_option(randr_state_t *state, const char *key, const char *value)
{
	if (strcasecmp(key, "screen") == 0) {
		state->screen_num = atoi(value);
	} else if (strcasecmp(key, "crtc") == 0) {
		char *tail;

		/* Check how many crtcs are configured */
		const char *local_value = value;
		while (1) {
			errno = 0;
			int parsed = strtol(local_value, &tail, 0);
			if (parsed == 0 && (errno != 0 ||
					    tail == local_value)) {
				fprintf(stderr, _("Unable to read screen"
						  " number: `%s'.\n"), value);
				return -1;
			} else {
				state->crtc_num_count += 1;
			}
			local_value = tail;

			if (*local_value == ',') {
				local_value += 1;
			} else if (*local_value == '\0') {
				break;
			}
		}

		/* Configure all given crtcs */
		state->crtc_num = calloc(state->crtc_num_count, sizeof(int));
		local_value = value;
		for (int i = 0; i < state->crtc_num_count; i++) {
			errno = 0;
			int parsed = strtol(local_value, &tail, 0);
			if (parsed == 0 && (errno != 0 ||
					    tail == local_value)) {
				return -1;
			} else {
				state->crtc_num[i] = parsed;
			}
			local_value = tail;

			if (*local_value == ',') {
				local_value += 1;
			} else if (*local_value == '\0') {
				break;
			}
		}
	} else if (strcasecmp(key, "preserve") == 0) {
		fprintf(stderr, _("Parameter `%s` is now always on; "
				  " Use the `%s` command-line option"
				  " to disable.\n"),
			key, "-P");
	} else {
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}

	return 0;
}

static int
randr_set_temperature_for_crtc(
	randr_state_t *state, int crtc_num, const color_setting_t *setting,
	int preserve)
{
	xcb_generic_error_t *error;

	if (crtc_num >= state->crtc_count || crtc_num < 0) {
		fprintf(stderr, _("CRTC %d does not exist. "),
			crtc_num);
		if (state->crtc_count > 1) {
			fprintf(stderr, _("Valid CRTCs are [0-%d].\n"),
				state->crtc_count-1);
		} else {
			fprintf(stderr, _("Only CRTC 0 exists.\n"));
		}

		return -1;
	}

	xcb_randr_crtc_t crtc = state->crtcs[crtc_num].crtc;
	unsigned int ramp_size = state->crtcs[crtc_num].ramp_size;

	/* Create new gamma ramps */
	uint16_t *gamma_ramps = malloc(3*ramp_size*sizeof(uint16_t));
	if (gamma_ramps == NULL) {
		perror("malloc");
		return -1;
	}

	uint16_t *gamma_r = &gamma_ramps[0*ramp_size];
	uint16_t *gamma_g = &gamma_ramps[1*ramp_size];
	uint16_t *gamma_b = &gamma_ramps[2*ramp_size];

	if (preserve) {
		/* Initialize gamma ramps from saved state */
		memcpy(gamma_ramps, state->crtcs[crtc_num].saved_ramps,
		       3*ramp_size*sizeof(uint16_t));
	} else {
		/* Initialize gamma ramps to pure state */
		for (int i = 0; i < ramp_size; i++) {
			uint16_t value = (double)i/ramp_size * (UINT16_MAX+1);
			gamma_r[i] = value;
			gamma_g[i] = value;
			gamma_b[i] = value;
		}
	}

	colorramp_fill(gamma_r, gamma_g, gamma_b, ramp_size,
		       setting);

	/* Set new gamma ramps */
	xcb_void_cookie_t gamma_set_cookie =
		xcb_randr_set_crtc_gamma_checked(state->conn, crtc,
						 ramp_size, gamma_r,
						 gamma_g, gamma_b);
	error = xcb_request_check(state->conn, gamma_set_cookie);

	if (error) {
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Set CRTC Gamma", error->error_code);
		free(gamma_ramps);
		return -1;
	}

	free(gamma_ramps);

	return 0;
}

static int
randr_set_temperature(
	randr_state_t *state, const color_setting_t *setting, int preserve)
{
	int r;

	/* If no CRTC numbers have been specified,
	   set temperature on all CRTCs. */
	if (state->crtc_num_count == 0) {
		for (int i = 0; i < state->crtc_count; i++) {
			r = randr_set_temperature_for_crtc(
				state, i, setting, preserve);
			if (r < 0) return -1;
		}
	} else {
		for (int i = 0; i < state->crtc_num_count; ++i) {
			r = randr_set_temperature_for_crtc(
				state, state->crtc_num[i], setting, preserve);
			if (r < 0) return -1;
		}
	}

	return 0;
}


const gamma_method_t randr_gamma_method = {
	"randr", 1,
	(gamma_method_init_func *)randr_init,
	(gamma_method_start_func *)randr_start,
	(gamma_method_free_func *)randr_free,
	(gamma_method_print_help_func *)randr_print_help,
	(gamma_method_set_option_func *)randr_set_option,
	(gamma_method_restore_func *)randr_restore,
	(gamma_method_set_temperature_func *)randr_set_temperature
};
