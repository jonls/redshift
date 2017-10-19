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
   Copyright (c) 2017  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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

#include <xf86drm.h>
#include <xf86drmMode.h>

#include "gamma-drm.h"
#include "colorramp.h"


typedef struct {
	int crtc_num;
	int crtc_id;
	int gamma_size;
	uint16_t* r_gamma;
	uint16_t* g_gamma;
	uint16_t* b_gamma;
} drm_crtc_state_t;

typedef struct {
	int card_num;
	int crtc_num;
	int fd;
	drmModeRes* res;
	drm_crtc_state_t* crtcs;
} drm_state_t;


static int
drm_init(drm_state_t **state)
{
	/* Initialize state. */
	*state = malloc(sizeof(drm_state_t));
	if (*state == NULL) return -1;

	drm_state_t *s = *state;
	s->card_num = 0;
	s->crtc_num = -1;
	s->fd = -1;
	s->res = NULL;
	s->crtcs = NULL;

	return 0;
}

static int
drm_start(drm_state_t *state)
{
	/* Acquire access to a graphics card. */
	long maxlen = strlen(DRM_DIR_NAME) + strlen(DRM_DEV_NAME) + 10;
	char pathname[maxlen];

	sprintf(pathname, DRM_DEV_NAME, DRM_DIR_NAME, state->card_num);

	state->fd = open(pathname, O_RDWR | O_CLOEXEC);
	if (state->fd < 0) {
		/* TODO check if access permissions, normally root or
		        membership of the video group is required. */
		perror("open");
		fprintf(stderr, _("Failed to open DRM device: %s\n"),
			pathname);
		return -1;
	}

	/* Acquire mode resources. */
	state->res = drmModeGetResources(state->fd);
	if (state->res == NULL) {
		fprintf(stderr, _("Failed to get DRM mode resources\n"));
		close(state->fd);
		state->fd = -1;
		return -1;
	}

	/* Create entries for selected CRTCs. */
	int crtc_count = state->res->count_crtcs;
	if (state->crtc_num >= 0) {
		if (state->crtc_num >= crtc_count) {
			fprintf(stderr, _("CRTC %d does not exist. "),
				state->crtc_num);
			if (crtc_count > 1) {
				fprintf(stderr, _("Valid CRTCs are [0-%d].\n"),
					crtc_count-1);
			} else {
				fprintf(stderr, _("Only CRTC 0 exists.\n"));
			}
			close(state->fd);
			state->fd = -1;
			drmModeFreeResources(state->res);
			state->res = NULL;
			return -1;
		}

		state->crtcs = malloc(2 * sizeof(drm_crtc_state_t));
		state->crtcs[1].crtc_num = -1;

		state->crtcs->crtc_num = state->crtc_num;
		state->crtcs->crtc_id = -1;
		state->crtcs->gamma_size = -1;
		state->crtcs->r_gamma = NULL;
		state->crtcs->g_gamma = NULL;
		state->crtcs->b_gamma = NULL;
	} else {
		int crtc_num;
		state->crtcs = malloc((crtc_count + 1) * sizeof(drm_crtc_state_t));
		state->crtcs[crtc_count].crtc_num = -1;
		for (crtc_num = 0; crtc_num < crtc_count; crtc_num++) {
			state->crtcs[crtc_num].crtc_num = crtc_num;
			state->crtcs[crtc_num].crtc_id = -1;
			state->crtcs[crtc_num].gamma_size = -1;
			state->crtcs[crtc_num].r_gamma = NULL;
			state->crtcs[crtc_num].g_gamma = NULL;
			state->crtcs[crtc_num].b_gamma = NULL;
		}
	}

	/* Load CRTC information and gamma ramps. */
	drm_crtc_state_t *crtcs = state->crtcs;
	for (; crtcs->crtc_num >= 0; crtcs++) {
		crtcs->crtc_id = state->res->crtcs[crtcs->crtc_num];
		drmModeCrtc* crtc_info = drmModeGetCrtc(state->fd, crtcs->crtc_id);
		if (crtc_info == NULL) {
			fprintf(stderr, _("CRTC %i lost, skipping\n"), crtcs->crtc_num);
			continue;
		}
		crtcs->gamma_size = crtc_info->gamma_size;
		drmModeFreeCrtc(crtc_info);
		if (crtcs->gamma_size <= 1) {
			fprintf(stderr, _("Could not get gamma ramp size for CRTC %i\n"
					  "on graphics card %i, ignoring device.\n"),
				crtcs->crtc_num, state->card_num);
			continue;
		}
		/* Valgrind complains about us reading uninitialize memory if we just use malloc. */
		crtcs->r_gamma = calloc(3 * crtcs->gamma_size, sizeof(uint16_t));
		crtcs->g_gamma = crtcs->r_gamma + crtcs->gamma_size;
		crtcs->b_gamma = crtcs->g_gamma + crtcs->gamma_size;
		if (crtcs->r_gamma != NULL) {
			int r = drmModeCrtcGetGamma(state->fd, crtcs->crtc_id, crtcs->gamma_size,
						    crtcs->r_gamma, crtcs->g_gamma, crtcs->b_gamma);
			if (r < 0) {
				fprintf(stderr, _("DRM could not read gamma ramps on CRTC %i on\n"
						  "graphics card %i, ignoring device.\n"),
					crtcs->crtc_num, state->card_num);
				free(crtcs->r_gamma);
				crtcs->r_gamma = NULL;
			}
		} else {
			perror("malloc");
			drmModeFreeResources(state->res);
			state->res = NULL;
			close(state->fd);
			state->fd = -1;
			while (crtcs-- != state->crtcs)
				free(crtcs->r_gamma);
			free(state->crtcs);
			state->crtcs = NULL;
			return -1;
		}
	}

	return 0;
}

static void
drm_restore(drm_state_t *state)
{
	drm_crtc_state_t *crtcs = state->crtcs;
	while (crtcs->crtc_num >= 0) {
		if (crtcs->r_gamma != NULL) {
			drmModeCrtcSetGamma(state->fd, crtcs->crtc_id, crtcs->gamma_size,
					    crtcs->r_gamma, crtcs->g_gamma, crtcs->b_gamma);
		}
		crtcs++;
	}
}

static void
drm_free(drm_state_t *state)
{
	if (state->crtcs != NULL) {
		drm_crtc_state_t *crtcs = state->crtcs;
		while (crtcs->crtc_num >= 0) {
			free(crtcs->r_gamma);
			crtcs->crtc_num = -1;
			crtcs++;
		}
		free(state->crtcs);
		state->crtcs = NULL;
	}
	if (state->res != NULL) {
		drmModeFreeResources(state->res);
		state->res = NULL;
	}
	if (state->fd >= 0) {
		close(state->fd);
		state->fd = -1;
	}

	free(state);
}

static void
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

static int
drm_set_option(drm_state_t *state, const char *key, const char *value)
{
	if (strcasecmp(key, "card") == 0) {
		state->card_num = atoi(value);
	} else if (strcasecmp(key, "crtc") == 0) {
		state->crtc_num = atoi(value);
		if (state->crtc_num < 0) {
			fprintf(stderr, _("CRTC must be a non-negative integer\n"));
			return -1;
		}
	} else {
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}

	return 0;
}

static int
drm_set_temperature(
	drm_state_t *state, const color_setting_t *setting, int preserve)
{
	drm_crtc_state_t *crtcs = state->crtcs;
	int last_gamma_size = 0;
	uint16_t *r_gamma = NULL;
	uint16_t *g_gamma = NULL;
	uint16_t *b_gamma = NULL;

	for (; crtcs->crtc_num >= 0; crtcs++) {
		if (crtcs->gamma_size <= 1)
			continue;
		if (crtcs->gamma_size != last_gamma_size) {
			if (last_gamma_size == 0) {
				r_gamma = malloc(3 * crtcs->gamma_size * sizeof(uint16_t));
				g_gamma = r_gamma + crtcs->gamma_size;
				b_gamma = g_gamma + crtcs->gamma_size;
			} else if (crtcs->gamma_size > last_gamma_size) {
				r_gamma = realloc(r_gamma, 3 * crtcs->gamma_size * sizeof(uint16_t));
				g_gamma = r_gamma + crtcs->gamma_size;
				b_gamma = g_gamma + crtcs->gamma_size;
			}
			if (r_gamma == NULL) {
				perror(last_gamma_size == 0 ? "malloc" : "realloc");
				return -1;
			}
			last_gamma_size = crtcs->gamma_size;
		}

		/* Initialize gamma ramps to pure state */
		int ramp_size = crtcs->gamma_size;
		for (int i = 0; i < ramp_size; i++) {
			uint16_t value = (double)i/ramp_size * (UINT16_MAX+1);
			r_gamma[i] = value;
			g_gamma[i] = value;
			b_gamma[i] = value;
		}

		colorramp_fill(r_gamma, g_gamma, b_gamma, crtcs->gamma_size,
			       setting);
		drmModeCrtcSetGamma(state->fd, crtcs->crtc_id, crtcs->gamma_size,
				    r_gamma, g_gamma, b_gamma);
	}

	free(r_gamma);

	return 0;
}


const gamma_method_t drm_gamma_method = {
	"drm", 0,
	(gamma_method_init_func *)drm_init,
	(gamma_method_start_func *)drm_start,
	(gamma_method_free_func *)drm_free,
	(gamma_method_print_help_func *)drm_print_help,
	(gamma_method_set_option_func *)drm_set_option,
	(gamma_method_restore_func *)drm_restore,
	(gamma_method_set_temperature_func *)drm_set_temperature
};
