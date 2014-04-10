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

   Copyright (c) 2010  Jon Lund Steffensen <jonlst@gmail.com>
   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include "gamma-randr.h"
#include "colorramp.h"
#include "redshift.h"


#define RANDR_VERSION_MAJOR  1
#define RANDR_VERSION_MINOR  3


int
randr_init(randr_state_t *state)
{
	/* Initialize state */
	state->conn = NULL;
	state->selection_count = 0;
	state->selections = malloc(1 * sizeof(randr_selection_t));
	if (state->selections == NULL) {
		perror("malloc");
		return -1;
	}
	state->crtcs_used = 0;
	state->crtcs = NULL;

	xcb_generic_error_t *error;

	/* Open X server connection */
	state->conn = xcb_connect(NULL, &state->preferred_screen);

	/* Query RandR version */
	xcb_randr_query_version_cookie_t ver_cookie =
		xcb_randr_query_version(state->conn, RANDR_VERSION_MAJOR,
					RANDR_VERSION_MINOR);
	xcb_randr_query_version_reply_t *ver_reply =
		xcb_randr_query_version_reply(state->conn, ver_cookie, &error);

	/* TODO What does it mean when both error and ver_reply is NULL?
	   Apparently, we have to check both to avoid seg faults. */
	if (error || ver_reply == NULL) {
		int ec = (error != 0) ? error->error_code : -1;
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Query Version", ec);
		xcb_disconnect(state->conn);
		return -1;
	}

	if (ver_reply->major_version != RANDR_VERSION_MAJOR ||
	    ver_reply->minor_version < RANDR_VERSION_MINOR) {
		fprintf(stderr, _("Unsupported RANDR version (%u.%u)\n"),
			ver_reply->major_version, ver_reply->minor_version);
		free(ver_reply);
		xcb_disconnect(state->conn);
		return -1;
	}

	free(ver_reply);

	/* Default selection */
	state->selections->screen_num = -1;
	state->selections->crtc_num = -1;
	state->selections->gamma[0] = DEFAULT_GAMMA;
	state->selections->gamma[1] = DEFAULT_GAMMA;
	state->selections->gamma[2] = DEFAULT_GAMMA;

	return 0;
}

int
randr_start(randr_state_t *state)
{
	xcb_generic_error_t *error;

	/* Get screens */
	const xcb_setup_t *setup = xcb_get_setup(state->conn);
	int screen_count = xcb_setup_roots_length(setup);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xcb_screen_t **screens = alloca(screen_count * sizeof(xcb_screen_t*));

	for (int i = 0; i < screen_count; i++) {
		screens[i] = iter.data;
		xcb_screen_next(&iter);
	}

	/* Use default selection if no other selection is made */
	if (state->selection_count == 0) {
		/* Select preferred screen if CRTC is selected but not a screen */
		if (state->selections->crtc_num != -1)
			if (state->selections->screen_num == -1)
				state->selections->screen_num = state->preferred_screen;

		/* Select CRTCs */
		if (state->selections->screen_num != -1) {
			state->selections = realloc(state->selections, 2 * sizeof(randr_selection_t));
			state->selection_count = 1;
			if (state->selections != NULL)
				state->selections[1] = state->selections[0];
		} else {
			state->selections = realloc(state->selections,
						    (screen_count + 1) * sizeof(randr_selection_t));
			state->selection_count = screen_count;
			if (state->selections != NULL) {
				int i;
				for (i = 1; i <= screen_count; i++) {
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

	/* Prepare storage screen information */
	xcb_randr_get_screen_resources_current_reply_t **screen_reses =
		alloca(screen_count * sizeof(xcb_randr_get_screen_resources_current_reply_t*));
	for (int i = 0; i < screen_count; i++)
		screen_reses[i] = NULL;

	/* Load implied screens and CRTCs and screen information and verify indices */
	int selection_index;
	for (selection_index = 1; selection_index <= state->selection_count; selection_index++) {
		randr_selection_t *selection = state->selections + selection_index;
		int screen_num = selection->screen_num;

		/* Verify screen index */
		if (screen_num >= screen_count) {
			fprintf(stderr, _("Screen %d does not exist. "),
				screen_num);
			if (screen_count > 1) {
				fprintf(stderr, _("Valid screens are [0-%d].\n"),
					screen_count - 1);
			} else {
				fprintf(stderr, _("Only screen 0 exists, did you mean CRTC %d?\n"),
					screen_num);
			}
			fprintf(stderr, "Invalid screen.\n");
			goto fail;
		}

		/* Load screen information if not already loaded */
		if (screen_reses[screen_num] == NULL) {
			/* Get list of CRTCs for the screens */
			xcb_randr_get_screen_resources_current_cookie_t res_cookie =
				xcb_randr_get_screen_resources_current(state->conn,
								       screens[screen_num]->root);
			xcb_randr_get_screen_resources_current_reply_t *res_reply =
				xcb_randr_get_screen_resources_current_reply(state->conn,
									     res_cookie,
									     &error);
			screen_reses[screen_num] = res_reply;

			if (error) {
				fprintf(stderr, _("`%s' returned error %d\n"),
					"RANDR Get Screen Resources Current",
					error->error_code);
				goto fail;
			}
		}

		xcb_randr_get_screen_resources_current_reply_t* res = screen_reses[screen_num];
		int crtc_count = res->num_crtcs;

		if (selection->crtc_num == -1) {
			/* Select all CRTCs. */
			if (crtc_count < 1)
				continue;
			int n = state->selection_count + crtc_count + 1;
			state->selections = realloc(state->selections, n * sizeof(randr_selection_t));
			selection = state->selections + selection_index;
			if (state->selections == NULL) {
				perror("realloc");
				goto fail;
			}
			int crtc_i;
			for (crtc_i = 0; crtc_i < crtc_count; crtc_i++) {
				randr_selection_t *sel = state->selections + ++(state->selection_count);
				*sel = state->selections[selection_index];
				sel->crtc_num = crtc_i;
			}
			*selection = state->selections[state->selection_count--];
		}

		/* Verify CRT index. */
		if (selection->crtc_num >= crtc_count) {
			fprintf(stderr, _("CRTC %d does not exist. "),
				selection->crtc_num);
			if (crtc_count > 1) {
				fprintf(stderr, _("Valid CRTCs are [0-%d].\n"),
					crtc_count - 1);
			} else {
				fprintf(stderr, _("Only CRTC 0 exists.\n"));
			}
			fprintf(stderr, "Invalid CRTC.\n");
			goto fail;
		}
	}

	/* Allocate memory for CRTCs */
	state->crtcs = calloc(state->selection_count, sizeof(randr_crtc_state_t));
	if (state->crtcs == NULL) {
		perror("malloc");
		goto fail;
	}

	/* Load CRTC information and store gamma */
	int skipped = 0;
	for (selection_index = 1; selection_index <= state->selection_count; selection_index++) {
		randr_selection_t *selection = state->selections + selection_index - skipped;
		if (selection->crtc_num < 0) {
			skipped++;
			continue;
		}
		xcb_randr_get_screen_resources_current_reply_t *res = screen_reses[selection->screen_num];
		xcb_randr_crtc_t *crtcs =
			xcb_randr_get_screen_resources_current_crtcs(res);
		randr_crtc_state_t *crtc = &state->crtcs[state->crtcs_used++];

		/* Store gamma correction from selection data */
		crtc->gamma[0] = selection->gamma[0];
		crtc->gamma[1] = selection->gamma[1];
		crtc->gamma[2] = selection->gamma[2];

		/* Save CRTC identifier in state */
		crtc->crtc = crtcs[selection->crtc_num];

		/* Save size and gamma ramps of all CRTCs.
		   Current gamma ramps are saved so we can restore them
		   at program exit. */

		/* Request size of gamma ramps */
		xcb_randr_get_crtc_gamma_size_cookie_t gamma_size_cookie =
			xcb_randr_get_crtc_gamma_size(state->conn, crtc->crtc);
		xcb_randr_get_crtc_gamma_size_reply_t *gamma_size_reply =
			xcb_randr_get_crtc_gamma_size_reply(state->conn,
							    gamma_size_cookie,
							    &error);

		if (error) {
			fprintf(stderr, _("`%s' returned error %d\n"),
				"RANDR Get CRTC Gamma Size",
				error->error_code);
			goto fail;
		}

		unsigned int ramp_size = gamma_size_reply->size;
		crtc->ramp_size = ramp_size;

		free(gamma_size_reply);

		if (ramp_size <= 1) {
			fprintf(stderr, _("Gamma ramp size too small: %i\n"),
				ramp_size);
			goto fail;
		}

		/* Request current gamma ramps */
		xcb_randr_get_crtc_gamma_cookie_t gamma_get_cookie =
			xcb_randr_get_crtc_gamma(state->conn, crtc->crtc);
		xcb_randr_get_crtc_gamma_reply_t *gamma_get_reply =
			xcb_randr_get_crtc_gamma_reply(state->conn,
						       gamma_get_cookie,
						       &error);

		if (error) {
			fprintf(stderr, _("`%s' returned error %d\n"),
				"RANDR Get CRTC Gamma", error->error_code);
			goto fail;
		}

		uint16_t *gamma_r =
			xcb_randr_get_crtc_gamma_red(gamma_get_reply);
		uint16_t *gamma_g =
			xcb_randr_get_crtc_gamma_green(gamma_get_reply);
		uint16_t *gamma_b =
			xcb_randr_get_crtc_gamma_blue(gamma_get_reply);

		/* Allocate space for saved gamma ramps */
		crtc->saved_ramps = malloc(3*ramp_size*sizeof(uint16_t));
		if (crtc->saved_ramps == NULL) {
			perror("malloc");
			free(gamma_get_reply);
			goto fail;
		}

		/* Copy gamma ramps into CRTC state */
		memcpy(&crtc->saved_ramps[0*ramp_size], gamma_r,
		       ramp_size*sizeof(uint16_t));
		memcpy(&crtc->saved_ramps[1*ramp_size], gamma_g,
		       ramp_size*sizeof(uint16_t));
		memcpy(&crtc->saved_ramps[2*ramp_size], gamma_b,
		       ramp_size*sizeof(uint16_t));

		free(gamma_get_reply);

		/* Allocate space for gamma ramps */
		uint16_t *gamma_ramps = malloc(3*ramp_size*sizeof(uint16_t));
		if (gamma_ramps == NULL) {
			perror("malloc");
			goto fail;
		}

		crtc->gamma_r = &gamma_ramps[0*ramp_size];
		crtc->gamma_g = &gamma_ramps[1*ramp_size];
		crtc->gamma_b = &gamma_ramps[2*ramp_size];
	}

	/* Release screen resources */
	for (int i = 0; i < screen_count; i++)
		if (screen_reses[i] != NULL)
			free(screen_reses[i]);

	/* We do not longer need raw selection information, free it */
	free(state->selections);
	state->selections = NULL;

	state->selection_count -= skipped;
	return 0;

fail:
	/* Release screen resources */
	for (int i = 0; i < screen_count; i++)
		if (screen_reses[i] != NULL)
			free(screen_reses[i]);

	return -1;
}

void
randr_restore(randr_state_t *state)
{
	xcb_generic_error_t *error;

	/* Restore CRTC gamma ramps */
	for (int i = 0; i < state->crtcs_used; i++) {
		randr_crtc_state_t* crtc = &state->crtcs[i];

		unsigned int ramp_size = crtc->ramp_size;
		uint16_t *gamma_r = &crtc->saved_ramps[0*ramp_size];
		uint16_t *gamma_g = &crtc->saved_ramps[1*ramp_size];
		uint16_t *gamma_b = &crtc->saved_ramps[2*ramp_size];

		/* Set gamma ramps */
		xcb_void_cookie_t gamma_set_cookie =
			xcb_randr_set_crtc_gamma_checked(state->conn, crtc->crtc,
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

void
randr_free(randr_state_t *state)
{
	/* Free saved gamma ramps */
	for (int i = 0; i < state->crtcs_used; i++) {
		if (state->crtcs[i].saved_ramps != NULL)
			free(state->crtcs[i].saved_ramps);
		if (state->crtcs[i].gamma_r != NULL)
			free(state->crtcs[i].gamma_r);
	}

	/* Free CRTC states */
	if (state->crtcs != NULL) {
		free(state->crtcs);
		state->crtcs = NULL;
	}

	/* Close connection */
	if (state->conn != NULL) {
		xcb_disconnect(state->conn);
		state->conn = NULL;
	}

	/* Free raw selection information */
	if (state->selections != NULL) {
		free(state->selections);
		state->selections = NULL;
	}
}

void
randr_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with the X RANDR extension.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: RANDR help output
	   left column must not be translated */
	fputs(_("  screen=N\tX screen to apply adjustments to\n"
		"  crtc=N\tCRTC to apply adjustments to\n"), f);
	fputs("\n", f);
}

int
randr_set_option(randr_state_t *state, const char *key, const char *value, int section)
{
	if (section == state->selection_count) {
		state->selections = realloc(state->selections,
					    (++(state->selection_count) + 1) * sizeof(randr_selection_t));

		if (state->selections == NULL) {
			perror("realloc");
			return -1;
		}

		state->selections[section + 1] = *(state->selections);
		state->selections[section + 1].screen_num = state->preferred_screen;
		state->selections[section + 1].crtc_num = 0;
	}

	if (strcasecmp(key, "screen") == 0) {
		int screen_num = atoi(value);
		state->selections[section + 1].screen_num = screen_num;
		if (screen_num < 0) {
			fprintf(stderr, _("Screen must be a non-negative integer.\n"));
			return -1;
		}
	} else if (strcasecmp(key, "crtc") == 0) {
		int crtc_num = atoi(value);
		state->selections[section + 1].crtc_num = crtc_num;
		if (crtc_num < 0) {
			fprintf(stderr, _("CRTC must be a non-negative integer.\n"));
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

static int
randr_set_temperature_for_crtc(randr_state_t *state, int crtc_num, int temp,
			       float brightness)
{
	randr_crtc_state_t *crtc = &state->crtcs[crtc_num];
	xcb_generic_error_t *error;

	unsigned int ramp_size = crtc->ramp_size;

	/* Create new gamma ramps */
	colorramp_fill(crtc->gamma_r, crtc->gamma_g, crtc->gamma_b,
		       ramp_size, temp, brightness, crtc->gamma);

	/* Set new gamma ramps */
	xcb_void_cookie_t gamma_set_cookie =
		xcb_randr_set_crtc_gamma_checked(state->conn, crtc->crtc,
						 ramp_size, crtc->gamma_r,
						 crtc->gamma_g, crtc->gamma_b);
	error = xcb_request_check(state->conn, gamma_set_cookie);

	if (error) {
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Set CRTC Gamma", error->error_code);
		return -1;
	}

	return 0;
}

int
randr_set_temperature(randr_state_t *state, int temp, float brightness)
{
	int r;

	for (int i = 0; i < state->crtcs_used; i++) {
		r = randr_set_temperature_for_crtc(state, i,
						   temp, brightness);
		if (r < 0) return -1;
	}

	return 0;
}
