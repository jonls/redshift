/* gamma-common.c -- Gamma adjustment method common functionallity source
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

#include "gamma-common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif



/* Initialize the adjustment method common parts of a state,
   this should be done before initialize the adjustment method
   specific parts. */
int
gamma_init(gamma_server_state_t *state)
{
	state->data = NULL;
	state->sites_used = 0;
	state->sites = NULL;
	state->selections_made = 1;
	state->selections = malloc(sizeof(gamma_selection_state_t));

	if (state->selections == NULL) {
		perror("malloc");
		state->selections_made = 0;
		return -1;
	}

	/* Defaults selection. */
	state->selections->crtc = -1;
	state->selections->partition = -1;
	state->selections->site = NULL;
	state->selections->settings.gamma_correction[0] = DEFAULT_GAMMA;
	state->selections->settings.gamma_correction[1] = DEFAULT_GAMMA;
	state->selections->settings.gamma_correction[2] = DEFAULT_GAMMA;
	state->selections->settings.gamma = DEFAULT_GAMMA;
	state->selections->settings.brightness = DEFAULT_BRIGHTNESS;
	state->selections->settings.temperature = NEUTRAL_TEMP;

	return 0;
}


/* Free all CRTC selection data in a state. */
void
gamma_free_selections(gamma_server_state_t *state)
{
	size_t i;

	/* Free data in each selection. */
	for (i = 0; i < state->selections_made; i++)
		if (state->selections[i].site != NULL)
			free(state->selections[i].site);
	state->selections_made = 0;

	/* Free the selection array. */
	if (state->selections != NULL) {
		free(state->selections);
		state->selections = NULL;
	}
}


/* Free all data in a state. */
void
gamma_free(gamma_server_state_t *state)
{
	size_t s, p, c;
	gamma_site_state_t *site;
	gamma_partition_state_t *partition;
	gamma_crtc_state_t *crtc;

	/* Free selections. */
	gamma_free_selections(state);

	/* Free each site. */
	for (s = 0; s < state->sites_used; s++) {
		site = state->sites + s;
		/* Free each partition. */
		for (p = 0; p < site->partitions_available; p++) {
			partition = site->partitions + p;
			if (partition->used == 0)
				continue;
			/* Free each CRTC. */
			for (c = 0; c < partition->crtcs_used; c++) {
				crtc = partition->crtcs + c;

				/* Free gamma ramps. */
				if (crtc->saved_ramps.red != NULL)
					free(crtc->saved_ramps.red);
				if (crtc->current_ramps.red != NULL)
					free(crtc->current_ramps.red);

				/* Free method dependent CRTC data. */
				if (crtc->data != NULL)
					state->free_crtc_data(crtc->data);
			}
			/* Free CRTC array. */
			if (partition->crtcs != NULL)
				free(partition->crtcs);

			/* Free method dependent partition data. */
			if (partition->data != NULL)
				state->free_partition_data(partition->data);
		}
		/* Free partition array. */
		if (site->partitions != NULL)
			free(site->partitions);

		/* Free site identifier. */
		if (site->site != NULL)
			free(site->site);

		/* Free method dependent site data. */
		if (site->data != NULL)
			state->free_site_data(site->data);
	}
	/* Free site array. */
	if (state->sites != NULL) {
		free(state->sites);
		state->sites = NULL;
	}

	/* Free method dependent state data. */
	if (state->data != NULL) {
		state->free_state_data(state->data);
		state->data = NULL;
	}
}
