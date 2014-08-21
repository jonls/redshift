/* gamma-quartz.c -- Mac OS X Quartz gamma adjustment source
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "gamma-quartz.h"


static void
quartz_free_crtc(void *data)
{
	free(data);
}

static void
quartz_free_partition(void *data)
{
	close_fake_quartz();
	free(data);
}

static int
quartz_open_site(gamma_server_state_t *state, char *site, gamma_site_state_t *site_out)
{
	(void) state;
	(void) site;
	site_out->data = NULL;
	site_out->partitions_available = 1;
	return 0;
}

static int
quartz_open_partition(gamma_server_state_t *state, gamma_site_state_t *site,
		      size_t partition, gamma_partition_state_t *partition_out)
{
	(void) state;
	(void) site;
	(void) partition;

	partition_out->data = NULL;
	partition_out->crtcs_available = 0;

	uint32_t cap = 4;
	CGDirectDisplayID *crtcs = malloc((size_t)cap * sizeof(CGDirectDisplayID));
	if (crtcs == NULL) {
		perror("malloc");
		return -1;
	}

	/* Count number of displays. */
	CGError r;
	uint32_t crtc_count;
	while (1) {
		r = CGGetOnlineDisplayList(cap, crtcs, &crtc_count);
		if (r != kCGErrorSuccess) {
			fputs(_("Cannot get list of online displays.\n"), stderr);
			free(crtcs);
			close_fake_quartz();
		}
		if (crtc_count < cap)
			break;
		cap <<= 1;
		if (cap == 0) { /* We could also test ~0, but it is still too many. */
			fprintf(stderr,
				_("An impossible number of CRTCs are available according to Quartz.\n"));
			free(crtcs);
			close_fake_quartz();
			return -1;
		}
		crtcs = realloc(crtcs, (size_t)cap * sizeof(CGDirectDisplayID));
		if (crtcs == NULL) {
			perror("realloc");
			close_fake_quartz();
			return -1;
		}
	}

	partition_out->data = crtcs;
	partition_out->crtcs_available = (size_t)crtc_count;

	return 0;
}

static int
quartz_open_crtc(gamma_server_state_t *state, gamma_site_state_t *site,
		 gamma_partition_state_t *partition, size_t crtc, gamma_crtc_state_t *crtc_out)
{
	(void) state;
	(void) site;

	CGDirectDisplayID *crtcs = partition->data;
	CGDirectDisplayID crtc_id = crtcs[crtc];

	crtc_out->data = NULL;
	crtc_out->saved_ramps.red = NULL;

	size_t gamma_size = CGDisplayGammaTableCapacity(crtc_id);
	if (gamma_size < 2) {
		fprintf(stderr,
			_("Quartz reported an impossibly small gamma ramp size for CRTC %ld.\n"),
			crtc);
		return -1;
	}

	/* Specify gamma ramp dimensions. */
	crtc_out->saved_ramps.red_size = gamma_size;
	crtc_out->saved_ramps.green_size = gamma_size;
	crtc_out->saved_ramps.blue_size = gamma_size;

	/* Allocate space for saved gamma ramps. */
	crtc_out->saved_ramps.red = malloc(3 * gamma_size * sizeof(uint16_t));
	if (crtc_out->saved_ramps.red == NULL) {
		perror("malloc");
		return -1;
	}
	crtc_out->saved_ramps.green = crtc_out->saved_ramps.red   + gamma_size;
	crtc_out->saved_ramps.blue  = crtc_out->saved_ramps.green + gamma_size;

	/* Allocate space for gamma ramps reading. */
	crtc_out->data = malloc(3 * gamma_size * sizeof(CGGammaValue));
	if (crtc_out->data == NULL) {
		perror("malloc");
		return -1;
	}

	/* Read current gamma ramps. */
	CGError r;
	uint32_t gamma_size_out;
	CGGammaValue *red_green_blue = crtc_out->data;
	CGGammaValue *red   = red_green_blue + 0 * gamma_size;
	CGGammaValue *green = red_green_blue + 1 * gamma_size;
	CGGammaValue *blue  = red_green_blue + 2 * gamma_size;
	r = CGGetDisplayTransferByTable(crtc_id, gamma_size, red, green, blue, &gamma_size_out);
	if (r != kCGErrorSuccess) {
		fprintf(stderr, _("Cannot read gamma ramps for CRTC %ld.\n"), crtc);
		return -1;
	}
	if (gamma_size_out != gamma_size) {
		fprintf(stderr, _("Quartz changed its mind about gamma ramp sizes,\n"
				  "I do not know what is real anymore.\n"));
		return -1;
	}

	/* Convert current gamma ramps to integer format. */
	for (size_t c = 0; c < 3; c++) {
		uint16_t     *ramp_int   = crtc_out->saved_ramps.red + c * gamma_size;
		CGGammaValue *ramp_float = red_green_blue            + c * gamma_size;
		for (uint32_t i = 0; i < gamma_size; i++) {
			int32_t value = (int32_t)(ramp_float[i] * UINT16_MAX);
			ramp_int[i] = (uint16_t)(value < 0 ? 0 : value > UINT16_MAX ? UINT16_MAX : value);
		}
	}

	return 0;
}

static int
quartz_set_ramps(gamma_server_state_t *state, gamma_crtc_state_t *crtc, gamma_ramps_t ramps)
{
	CGDirectDisplayID *crtcs = state->sites->partitions->data;
	CGDirectDisplayID crtc_id = crtcs[crtc->crtc];
	size_t gamma_size = ramps.red_size;
	
	CGGammaValue *red_green_blue = crtc->data;
	CGGammaValue *red   = red_green_blue + 0 * gamma_size;
	CGGammaValue *green = red_green_blue + 1 * gamma_size;
	CGGammaValue *blue  = red_green_blue + 2 * gamma_size;

	/* Convert pending gamma ramps to float format. */
	for (size_t c = 0; c < 3; c++) {
		uint16_t     *ramp_int   = crtc->current_ramps.red + c * gamma_size;
		CGGammaValue *ramp_float = red_green_blue          + c * gamma_size;
		for (uint32_t i = 0; i < gamma_size; i++)
		        ramp_float[i] = (CGGammaValue)(ramp_int[i]) / UINT16_MAX;
	}

	/* Apply gamma ramps. */
	CGError r = CGSetDisplayTransferByTable(crtc_id, (uint32_t)gamma_size, red, green, blue);
	if (r != kCGErrorSuccess) {
		fprintf(stderr, _("Cannot set gamma ramps for CRTC %ld.\n"), crtc->crtc);
		return -1;
	}

	return 0;
}

static int
quartz_set_option(gamma_server_state_t *state, const char *key, char *value, ssize_t section)
{
	if (strcasecmp(key, "crtc") == 0) {
		ssize_t crtc = strcasecmp(value, "all") ? (ssize_t)atoi(value) : -1;
		if (crtc < 0 && strcasecmp(value, "all")) {
			/* TRANSLATORS: `all' must not be translated. */
			fprintf(stderr, _("CRTC must be `all' or a non-negative integer.\n"));
			return -1;
		}
		if (section >= 0) {
			state->selections[section].crtc = crtc;
		} else {
			for (size_t i = 0; i < state->selections_made; i++)
				state->selections[i].crtc = crtc;
		}
		return 0;
	}
	return 1;
}


int
quartz_init(gamma_server_state_t *state)
{
	int r;
	r = gamma_init(state);
	if (r != 0) return r;

	state->free_partition_data = quartz_free_partition;
	state->free_crtc_data      = quartz_free_crtc;
	state->open_site           = quartz_open_site;
	state->open_partition      = quartz_open_partition;
	state->open_crtc           = quartz_open_crtc;
	state->set_ramps           = quartz_set_ramps;
	state->set_option          = quartz_set_option;

	return 0;
}

int
quartz_start(gamma_server_state_t *state)
{
	return gamma_resolve_selections(state);
}

void
quartz_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with Quartz.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: Mac OS X Quartz help output
	   left column must not be translated. */
	fputs(_("  crtc=N\tX monitor to apply adjustments to\n"), f);
	fputs("\n", f);
}

