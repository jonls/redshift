/* gamma-drm.c -- DRM gamma adjustment source
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

   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <alloca.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#ifndef O_CLOEXEC
  #define O_CLOEXEC  02000000
#endif

#include "gamma-drm.h"
#include "colorramp.h"
#include "redshift.h"


int
drm_init(drm_state_t *state)
{
	/* Check if we are in a display */
	char* env;
	int in_a_dislay = 0;
	if ((env = getenv("DISPLAY")) != NULL) {
		if (strchr(env, ':'))
			in_a_dislay = 1;
	} else if ((env = getenv("WAYLAND_DISPLAY")) != NULL) {
		if (strstr(env, "wayland-") == env)
			in_a_dislay = 1;
	}
	if (in_a_dislay) {
		fprintf(stderr, _("You appear to be inside a graphical environment\n"
				  "adjustments made using the DRM method will only\n"
				  "be applied when in a TTY.\n"));
	}

	/* Count number of graphics cards. */
	long maxlen = strlen(DRM_DIR_NAME) + strlen(DRM_DEV_NAME) + 10;
	char *pathname = alloca(maxlen * sizeof(char));
	int card_count;
	for (card_count = 0;; card_count++) {
		struct stat _attr;
		sprintf(pathname, DRM_DEV_NAME, DRM_DIR_NAME, card_count);
		if (stat(pathname, &_attr))
			break;
	}

	/* Initialize state. */
	state->selection_count = 0;
	state->selections = malloc(sizeof(drm_selection_t));
	if (state->selections == NULL) {
		perror("malloc");
		return -1;
	}
	state->selections->card_num = -1;
	state->selections->crtc_num = -1;
	state->selections->gamma[0] = DEFAULT_GAMMA;
	state->selections->gamma[1] = DEFAULT_GAMMA;
	state->selections->gamma[2] = DEFAULT_GAMMA;
	state->card_count = card_count;
	state->cards = card_count ? malloc(card_count * sizeof(drm_crtc_state_t)) : NULL;
	if (state->cards == NULL) {
		if (card_count) perror("malloc");
		else            fprintf(stderr, _("Could not find any graphics card using DRM\n"));
		return -1;
	}
	int card_index;
	for (card_index = 0; card_index < card_count; card_index++) {
		state->cards[card_index].fd = -1;
		state->cards[card_index].res = NULL;
		state->cards[card_index].crtcs_used = 0;
		state->cards[card_index].crtcs = NULL;
	}

	return 0;
}

int
drm_start(drm_state_t *state)
{
	/* Use default selection if no other selection is made. */
	if (state->selection_count == 0) {
		/* Select card 0 if CRTC is selected but not a card. */
		if (state->selections->crtc_num != -1)
			if (state->selections->card_num == -1)
				state->selections->card_num = 0;

		/* Select CRTCs. */
		if (state->selections->card_num != -1) {
			state->selections = realloc(state->selections, 2 * sizeof(drm_selection_t));
			state->selection_count = 1;
			if (state->selections != NULL)
				state->selections[1] = state->selections[0];
		} else {
			state->selections = realloc(state->selections,
						    (state->card_count + 1) * sizeof(drm_selection_t));
			state->selection_count = state->card_count;
			if (state->selections != NULL) {
				int i;
				for (i = 1; i <= state->card_count; i++) {
					state->selections[i] = *(state->selections);
					state->selections[i].card_num = i - 1;
				}
			}
		}
		if (state->selections == NULL) {
			perror("realloc");
			return -1;
		}
	}

	/* Apply selections. */
	long maxlen = strlen(DRM_DIR_NAME) + strlen(DRM_DEV_NAME) + 10;
	char *pathname = alloca(maxlen * sizeof(char));
	int skipped = 0;
	int selection_index;
	for (selection_index = 1; selection_index <= state->selection_count; selection_index++) {
		drm_selection_t *selection = state->selections + selection_index - skipped;
		drm_card_state_t *card = state->cards + selection->card_num;

		if (card->fd == -1) {
			/* Acquire access to a graphics card. */
			sprintf(pathname, DRM_DEV_NAME, DRM_DIR_NAME, selection->card_num);
			card->fd = open(pathname, O_RDWR | O_CLOEXEC);
			if (card->fd < 0) {
				/* TODO check access permissions, normally root or
				        membership of the video group is required. */
				perror("open");
				return -1;
			}

			/* Acquire mode resources. */
			card->res = drmModeGetResources(card->fd);
			if (card->res == NULL) {
				fprintf(stderr, _("Failed to get DRM mode resources\n"));
				return -1;
			}
		}

		int crtc_count = card->res->count_crtcs;

		if (selection->crtc_num == -1) {
			/* Select all CRTCs. */
			if (crtc_count < 1)
				continue;
			int n = state->selection_count + crtc_count + 1;
			state->selections = realloc(state->selections, n * sizeof(drm_selection_t));
			selection = state->selections + selection_index;
			if (state->selections == NULL) {
				perror("realloc");
				return -1;
			}
			int crtc_i;
			for (crtc_i = 0; crtc_i < crtc_count; crtc_i++) {
				drm_selection_t *sel = state->selections + ++(state->selection_count);
				*sel = *selection;
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
			return -1;
		}

		/* Prepare for adding another output. */
		if (card->crtcs_used++ == 0)
			card->crtcs = malloc(1 * sizeof(drm_crtc_state_t));
		else
			card->crtcs = realloc(card->crtcs, card->crtcs_used * sizeof(drm_crtc_state_t));
		if (card->crtcs == NULL) {
			perror(card->crtcs_used == 1 ? "malloc" : "realloc");
			return -1;
		}

		/* Create entry for other selected CRTC. */
		drm_crtc_state_t *crtc = card->crtcs + card->crtcs_used - 1;
		crtc->crtc_num = selection->crtc_num;
		crtc->crtc_id = -1;
		crtc->gamma_size = -1;
		crtc->saved_gamma_r = NULL;
		crtc->saved_gamma_g = NULL;
		crtc->saved_gamma_b = NULL;
		crtc->gamma[0] = selection->gamma[0];
		crtc->gamma[1] = selection->gamma[1];
		crtc->gamma[2] = selection->gamma[2];
		crtc->gamma_r = NULL;
		crtc->gamma_g = NULL;
		crtc->gamma_b = NULL;
		
		/* Load CRTC information and gamma ramps. */
		crtc->crtc_id = card->res->crtcs[selection->crtc_num];
		drmModeCrtc* crtc_info = drmModeGetCrtc(card->fd, crtc->crtc_id);
		if (crtc_info == NULL) {
			fprintf(stderr, _("CRTC %i lost, skipping.\n"), crtc->crtc_num);
			crtc->crtc_num = -1;
			continue;
		}
		crtc->gamma_size = crtc_info->gamma_size;
		drmModeFreeCrtc(crtc_info);
		if (crtc->gamma_size <= 1) {
			fprintf(stderr,
				_("DRM failed to use gamma ramps on CRTC %i\n"
				  "on graphics card %i, ignoring device.\n"),
				crtc->crtc_num, selection->card_num);
			continue;
		}
		/* Valgrind complains about us reading uninitialize memory if we just use malloc. */
		crtc->saved_gamma_r = calloc(3 * crtc->gamma_size, sizeof(uint16_t));
		crtc->saved_gamma_g = crtc->saved_gamma_r + crtc->gamma_size;
		crtc->saved_gamma_b = crtc->saved_gamma_g + crtc->gamma_size;
		if (crtc->saved_gamma_r == NULL) {
			perror("malloc");
			return -1;
		}
		int r = drmModeCrtcGetGamma(card->fd, crtc->crtc_id, crtc->gamma_size,
					    crtc->saved_gamma_r, crtc->saved_gamma_g,
					    crtc->saved_gamma_b);
		if (r < 0) {
			free(crtc->saved_gamma_r);
			crtc->saved_gamma_r = NULL;
			fprintf(stderr, _("DRM could not read gamma ramps on CRTC %i\n"
					  "on graphics card %i, ignoring device.\n"),
				crtc->crtc_num, selection->card_num);
		}

		/* Allocate space for gamma ramps. */
		uint16_t *gamma_ramps = malloc(3 * crtc->gamma_size * sizeof(uint16_t));
		if (gamma_ramps == NULL) {
			perror("malloc");
			return -1;
		}

		crtc->gamma_r = gamma_ramps;
		crtc->gamma_g = crtc->gamma_r + crtc->gamma_size;
		crtc->gamma_b = crtc->gamma_g + crtc->gamma_size;
	}

	/* We do not longer need raw selection information, free it. */
	free(state->selections);
	state->selections = NULL;

	state->selection_count -= skipped;
	return 0;
}

void
drm_restore(drm_state_t *state)
{
	int card_index;
	for (card_index = 0; card_index < state->card_count; card_index++) {
		drm_card_state_t *card = state->cards + card_index;
		drm_crtc_state_t *crtcs = card->crtcs;
		int crtc_index;
		for (crtc_index = 0; crtc_index < card->crtcs_used; crtc_index++) {
			drm_crtc_state_t *crtc = card->crtcs + crtc_index;
			if (crtc->gamma_size <= 1 || crtc->crtc_num < 0)
				continue;
			if (crtc->saved_gamma_r != NULL)
				drmModeCrtcSetGamma(card->fd, crtcs->crtc_id, crtc->gamma_size,
						    crtc->saved_gamma_r, crtc->saved_gamma_g,
						    crtc->saved_gamma_b);
		}
	}
}

void
drm_free(drm_state_t *state)
{
	int card_index;
	for (card_index = 0; card_index < state->card_count; card_index++) {
		drm_card_state_t *card = state->cards + card_index;
		if (card->crtcs != NULL) {
			drm_crtc_state_t *crtcs = card->crtcs;
			int crtc_index;
			for (crtc_index = 0; crtc_index < card->crtcs_used; crtc_index++) {
				if (crtcs[crtc_index].saved_gamma_r != NULL)
					free(crtcs[crtc_index].saved_gamma_r);
				if (crtcs[crtc_index].gamma_r != NULL)
					free(crtcs[crtc_index].gamma_r);
			}
			free(card->crtcs);
			card->crtcs = NULL;
		}
		if (card->res != NULL) {
			drmModeFreeResources(card->res);
			card->res = NULL;
		}
		if (card->fd >= 0) {
			close(card->fd);
			card->fd = -1;
		}
	}
	if (state->cards != NULL) {
		free(state->cards);
		state->cards = NULL;
	}
	if (state->selections != NULL) {
		free(state->selections);
		state->selections = NULL;
	}
}

void
drm_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with Direct Rendering Manager.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: DRM help output
	   left column must not be translated */
	fputs(_("  card=N\tGraphics card to apply adjustments to\n"
		"  crtc=N\tCRTC to apply adjustments to\n"), f);
	fputs("\n", f);
}

int
drm_set_option(drm_state_t *state, const char *key, const char *value, int section)
{
	if (section == state->selection_count) {
		state->selections = realloc(state->selections,
					    (++(state->selection_count) + 1) * sizeof(drm_selection_t));

		if (state->selections == NULL) {
			perror("realloc");
			return -1;
		}

		state->selections[section + 1] = *(state->selections);
		state->selections[section + 1].card_num = 0;
		state->selections[section + 1].crtc_num = 0;
	}

	if (strcasecmp(key, "card") == 0) {
		state->selections[section + 1].card_num = atoi(value);
		if (state->selections[section + 1].card_num < 0) {
			fprintf(stderr, _("Card must be a non-negative integer.\n"));
			fprintf(stderr, "Invalid card.\n");
			return -1;
		} else if (state->selections[section + 1].card_num >= state->card_count) {
			fprintf(stderr, _("Valid Cards are [0-%d].\n"),
				state->card_count - 1);
			fprintf(stderr, "Invalid card.\n");
			return -1;
		}
	} else if (strcasecmp(key, "crtc") == 0) {
		state->selections[section + 1].crtc_num = atoi(value);
		if (state->selections[section + 1].crtc_num < 0) {
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

int
drm_set_temperature(drm_state_t *state, int temp, float brightness)
{
	int card_num;

	for (card_num = 0; card_num < state->card_count; card_num++) {
		drm_card_state_t *card = state->cards + card_num;
		int crtc_;
		for (crtc_ = 0; crtc_ < card->crtcs_used; crtc_++) {
			drm_crtc_state_t *crtc = card->crtcs + crtc_;
			if (crtc->gamma_size <= 1 || crtc->crtc_num < 0)
				continue;

			/* Fill gamma ramp and apply. */
			colorramp_fill(crtc->gamma_r, crtc->gamma_g, crtc->gamma_b,
				       crtc->gamma_size, temp, brightness, crtc->gamma);
			drmModeCrtcSetGamma(card->fd, crtc->crtc_id, crtc->gamma_size,
					    crtc->gamma_r, crtc->gamma_g, crtc->gamma_b);

			/* If Direct Rendering Manager is used to change the gamma ramps
			   while an graphical session is active in the foreground (and X
			   display is running on the active VT) Direct Rendering Manager
			   will ignore the request and report that you do not have
			   sufficient permissions. This rejection is ignored so nothing
			   funny happens if the users opens a VT with a graphical session. */
		}
	}

	return 0;
}
