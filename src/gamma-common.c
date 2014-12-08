/* gamma-common.c -- Gamma adjustment method common functionality source
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
#include "adjustments.h"
#include "colorramp.h"

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
	state->selections->crtcs = NULL;
	state->selections->crtcs_count = 0;
	state->selections->partitions = NULL;
	state->selections->partitions_count = 0;
	state->selections->sites = NULL;
	state->selections->sites_count = 0;
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
	size_t i, j;

	/* Free data in each selection. */
	for (i = 0; i < state->selections_made; i++) {
		if (state->selections[i].crtcs != NULL)
			free(state->selections[i].crtcs);
		if (state->selections[i].partitions != NULL)
			free(state->selections[i].partitions);
		if (state->selections[i].sites != NULL) {
			for (j = 0; j < state->selections[i].sites_count; j++) {
				if (state->selections[i].sites[j] != NULL)
					free(state->selections[i].sites[j]);
			}
			free(state->selections[i].sites);
		}
	}
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


/* Create CRTC iterator. */
gamma_iterator_t
gamma_iterator(gamma_server_state_t *state)
{
	gamma_iterator_t iterator = {
		.crtc      = NULL,
		.partition = NULL,
		.site      = NULL,
		.state     = state
	};
	return iterator;
}


/* Get next CRTC. */
int
gamma_iterator_next(gamma_iterator_t *iterator)
{
	/* First CRTC. */
	if (iterator->crtc == NULL) {
		if (iterator->state->sites_used == 0)
			return 0;
		iterator->site      = iterator->state->sites;
		iterator->partition = iterator->site->partitions;
		gamma_partition_state_t *partitions_end =
			iterator->site->partitions +
			iterator->site->partitions_available;
		while (iterator->partition->used == 0) {
			iterator->partition++;
			if (iterator->partition == partitions_end)
				return 0;
		}
		if (iterator->partition->crtcs_used == 0)
			return 0;
		iterator->crtc = iterator->partition->crtcs;
		return 1;
	}

	/* Next CRTC. */
	size_t crtc_i      = (size_t)(iterator->crtc - iterator->partition->crtcs) + 1;
	size_t partition_i = iterator->crtc->partition;
	size_t site_i      = iterator->crtc->site_index;

	if (crtc_i == iterator->partition->crtcs_used) {
		crtc_i = 0;
		partition_i += 1;
	}
next_partition:
	if (partition_i == iterator->site->partitions_available) {
		partition_i = 0;
		site_i += 1;
	}
	if (site_i == iterator->state->sites_used)
		return 0;
	if (iterator->state->sites[site_i].partitions[partition_i].used == 0) {
		partition_i += 1;
		goto next_partition;
	}

	iterator->site      = iterator->state->sites     + site_i;
	iterator->partition = iterator->site->partitions + partition_i;
	iterator->crtc      = iterator->partition->crtcs + crtc_i;
	return 1;
}


/* Find the index of a site or the index for a new site. */
size_t
gamma_find_site(const gamma_server_state_t *state, const char *site)
{
	size_t site_index;

	/* Find matching already opened site. */
	for (site_index = 0; site_index < state->sites_used; site_index++) {
		char *test_site = state->sites[site_index].site;
		if (test_site == NULL || site == NULL) {
			if (test_site == NULL && site == NULL)
				break;
		}
		else if (!strcmp(site, test_site))
			break;
	}

	return site_index;
}


/* Open a site. */
static int
gamma_open_site(gamma_server_state_t *state, size_t site_index,
		char *site_name, gamma_site_state_t **site_out)
{
	int rc = -1, r;
	gamma_site_state_t *site;

	/* Grow array with sites, we temporarily store the new array
	   a temporarily variable so that we can properly release
	   resources on error. */
	gamma_site_state_t *new_sites;
	size_t alloc_size = (site_index + 1) * sizeof(gamma_site_state_t);
	new_sites = state->sites != NULL ?
		    realloc(state->sites, alloc_size) :
		    malloc(alloc_size);
	if (new_sites == NULL) {
		perror(state->sites != NULL ? "realloc" : "malloc");
		goto fail;
	}
	state->sites = new_sites;
	site = state->sites + site_index;

	/* Make sure these are not freed before allocated on error. */
	site->site = NULL;
	site->partitions = NULL;

	r = state->open_site(state, site_name, site);
	if (r != 0) {
		rc = r;
		goto fail;
	}

	/* Increment now (rather than earlier), so we do not get segfault on error. */
	state->sites_used += 1;

	if (site_name != NULL) {
		site->site = strdup(site_name);
		if (site->site == NULL) {
			perror("strdup");
			goto fail;
		}
	}

	/* calloc is used so that `used` in each partition is set to false. */
	site->partitions = calloc(site->partitions_available,
				  sizeof(gamma_partition_state_t));
	if (site->partitions == NULL) {
		site->partitions_available = 0;
		perror(state->sites != NULL ? "realloc" : "malloc");
		goto fail;
	}

	*site_out = site;
	return 0;

fail:
	return rc;
}


/* Open a CRTC. */
static int
gamma_open_crtc(gamma_server_state_t *state, gamma_site_state_t *site,
		gamma_partition_state_t *partition, size_t site_index,
		size_t partition_index, size_t crtc_index,
		gamma_selection_state_t *selection)
{
	int rc = -1, r;

	gamma_crtc_state_t *crtc = partition->crtcs + partition->crtcs_used;
	size_t total_ramp_size = 0, rrs, grs;
	uint16_t *ramps;

	r = state->open_crtc(state, site, partition, crtc_index, crtc);
	if (r != 0) {
		rc = r;
		goto fail;
	}
	crtc->crtc = crtc_index;
	crtc->partition = partition_index;
	crtc->site_index = site_index;
	partition->crtcs_used += 1;

	/* Store adjustment settigns. */
	crtc->settings = selection->settings;

	/* Create crtc->current_ramps. */
	crtc->current_ramps = crtc->saved_ramps;
	total_ramp_size += rrs = crtc->current_ramps.red_size;
	total_ramp_size += grs = crtc->current_ramps.green_size;
	total_ramp_size +=       crtc->current_ramps.blue_size;
	total_ramp_size *= sizeof(uint16_t);
	ramps = malloc(total_ramp_size);
	crtc->current_ramps.red   = ramps;
	crtc->current_ramps.green = ramps + rrs;
	crtc->current_ramps.blue  = ramps + rrs + grs;
	if (ramps == NULL) {
		perror("malloc");
		goto fail;
	}

	return 0;

fail:
	return rc;
}


/* Open partitions. */
static int
gamma_open_partitions(gamma_server_state_t *state, gamma_site_state_t *site,
		      gamma_selection_state_t *selection, int all_partitions)
{
	int rc = -1, r;

	/* Select all partitions if none have been specified explicity. */
	if (all_partitions && selection->partitions_count < site->partitions_available) {
		size_t p, n = site->partitions_available;
		if (selection->partitions != NULL)
			free(selection->partitions);
		selection->partitions = malloc(n * sizeof(size_t));
		if (selection->partitions == NULL) {
			perror("malloc");
			goto fail;
		}
		for (p = 0; p < n; p++)
			selection->partitions[p] = p;
	}
	if (all_partitions) {
		selection->partitions_count = site->partitions_available;
	}

	/* Open partitions. */
	for (size_t p = 0; p < selection->partitions_count; p++) {
		size_t partition_index = selection->partitions[p];

		/* Validate partition index. */
		if (partition_index >= site->partitions_available) {
			state->invalid_partition(site, partition_index);
			goto fail;
		}

		gamma_partition_state_t *partition = site->partitions + partition_index;
		if (partition->used) continue; /* Already open. */

		/* Open partition. */
		r = state->open_partition(state, site, partition_index, partition);
		if (r != 0) {
			rc = r;
			goto fail;
		}

		partition->used = 1;
	}

	return 0;

fail:
	return rc;
}


/* Open multiple CRTCs. */
static int
gamma_open_crtcs(gamma_server_state_t *state, gamma_site_state_t *site,
		 size_t site_index, size_t partition_index,
		 gamma_selection_state_t *selection, int all_crtcs)
{
	int rc = -1, r;
	gamma_partition_state_t *partition = site->partitions + partition_index;

	/* Select all CRTCs if none have been specified explicity. */
	if (all_crtcs && selection->crtcs_count < partition->crtcs_available) {
		size_t c, n = partition->crtcs_available;
		if (selection->crtcs != NULL)
			free(selection->crtcs);
		selection->crtcs = malloc(n * sizeof(size_t));
		if (selection->crtcs == NULL) {
			perror("malloc");
			goto fail;
		}
		for (c = 0; c < n; c++)
			selection->crtcs[c] = c;
	}
	if (all_crtcs) {
		selection->crtcs_count = partition->crtcs_available;
	}

	/* Grow array with selected CRTCs, we temporarily store
	   the new array a temporarily variable so that we can
	   properly release resources on error. */
	gamma_crtc_state_t *new_crtcs;
	size_t alloc_size = partition->crtcs_used + selection->crtcs_count;
	alloc_size *= sizeof(gamma_crtc_state_t);
	new_crtcs = partition->crtcs != NULL ?
		    realloc(partition->crtcs, alloc_size) :
		    malloc(alloc_size);
	if (new_crtcs == NULL) {
		perror(partition->crtcs != NULL ? "realloc" : "malloc");
		goto fail;
	}
	partition->crtcs = new_crtcs;

	for (size_t c = 0; c < selection->crtcs_count; c++) {
		size_t crtc_index = selection->crtcs[c];

		/* Validate CRTC index. */
		if (crtc_index >= partition->crtcs_available) {
			fprintf(stderr, _("CRTC %ld does not exist. "),
				crtc_index);
			if (partition->crtcs_available > 1) {
				fprintf(stderr, _("Valid CRTCs are [0-%ld].\n"),
					partition->crtcs_available - 1);
			} else {
				fprintf(stderr, _("Only CRTC 0 exists.\n"));
			}
			return -1;
		}

		/* Open CRTC. */
		r = gamma_open_crtc(state, site, partition, site_index,
				    partition_index, crtc_index, selection);
		if (r != 0) {
			rc = r;
			goto fail;
		}
	}

	return 0;

fail:
	return rc;
}


/* Resolve selections. */
int
gamma_resolve_selections(gamma_server_state_t *state)
{
	int default_selection = state->selections_made == 1;
	int rc = -1, r;

	/* Shift the selections so that the iteration finds the
	   default selection if no other selection is made. */
	if (default_selection) {
		state->selections_made += 1;
		state->selections--;
	}

	for (size_t i = 1; i < state->selections_made; i++) {
		gamma_selection_state_t *selection = state->selections + i;
		int all_crtcs = selection->crtcs == NULL;
		int all_partitions = selection->partitions == NULL;
		gamma_site_state_t *site;
		size_t site_index;

		/* Select default site if none have been specified. */
		if (selection->sites == NULL) {
			selection->sites = malloc(1 * sizeof(char *));
			if (selection->sites == NULL) {
				perror("malloc");
				goto fail;
			}
			selection->sites[0] = NULL;
			selection->sites_count = 1;
		}

		for (size_t s = 0; s < selection->sites_count; s++) {
			/* Find matching already opened site. */
			site_index = gamma_find_site(state, selection->sites[s]);

			/* Open site if not found. */
			if (site_index == state->sites_used) {
				r = gamma_open_site(state, site_index, selection->sites[s], &site);
				if (r != 0) {
					rc = r;
					goto fail;
				}
			} else {
				site = state->sites + site_index;
			}

			/* Open partitions. */
			r = gamma_open_partitions(state, site, selection, all_partitions);
			if (r != 0) {
				rc = r;
				goto fail;
			}

			/* Open CRTCs. */
			for (size_t p = 0; p < selection->partitions_count; p++) {
				size_t partition_index = selection->partitions[p];
				r = gamma_open_crtcs(state, site, site_index,
						     partition_index, selection, all_crtcs);
				if (r != 0) {
					rc = r;
					goto fail;
				}
			}
		}
	}

	rc = 0;

fail:
	/* Undo the shift made in the beginning of this function. */
	if (default_selection) {
		state->selections_made -= 1;
		state->selections++;
	}

	gamma_free_selections(state);

	return rc;
}


/* Restore gamma ramps. */
void
gamma_restore(gamma_server_state_t *state)
{
	gamma_iterator_t iter = gamma_iterator(state);
	while (gamma_iterator_next(&iter)) {
		if (iter.crtc->saved_ramps.red == NULL)
			continue;
		state->set_ramps(state, iter.crtc, iter.crtc->saved_ramps);
	}
}

/* Update gamma ramps. */
int
gamma_update(gamma_server_state_t *state)
{
	gamma_iterator_t iter = gamma_iterator(state);
	int r;
	while (gamma_iterator_next(&iter)) {
		if (iter.crtc->current_ramps.red == NULL)
			continue;
		colorramp_fill(iter.crtc->current_ramps, iter.crtc->settings);
		r = state->set_ramps(state, iter.crtc, iter.crtc->current_ramps);
		if (r != 0) return r;
	}
	return 0;
}


/* Methods for updating adjustments on all CRTCs. */

static gamma_crtc_selection_t all_crtcs = {
	.site = -1,
	.partition = -1,
	.crtc = -1
};

#define __gamma_update_all(PROP)						\
	void									\
	gamma_update_all_##PROP(gamma_server_state_t *state, float PROP)	\
	{									\
		gamma_update_##PROP(state, all_crtcs, PROP);			\
	}

__gamma_update_all(gamma)
__gamma_update_all(brightness)
__gamma_update_all(temperature)

#undef __gamma_update_all


/* Methods for updating adjustments on selected CRTCs. */

#define __test(ANCESTOR, THIS)  (crtcs.THIS < 0 || iter.THIS == iter.ANCESTOR->THIS##s + crtcs.THIS)
#define __test_all()            (__test(state, site) && __test(site, partition) && __test(partition, crtc))

#define __update(PROP)										\
	if (crtcs.site < 0 || crtcs.partition < 0 || crtcs.crtc < 0) {				\
		gamma_iterator_t iter = gamma_iterator(state);					\
		if (crtcs.site < 0 && crtcs.partition < 0 && crtcs.crtc < 0) {			\
			while (gamma_iterator_next(&iter))					\
				iter.crtc->settings.PROP = PROP;				\
		} else {									\
			while (gamma_iterator_next(&iter))					\
				if (__test_all())						\
					iter.crtc->settings.PROP = PROP;			\
		}										\
	} else if ((size_t)(crtcs.site) < state->sites_used) {					\
		gamma_site_state_t site = state->sites[crtcs.site];				\
		gamma_partition_state_t partition;						\
		if ((size_t)(crtcs.partition) < site.partitions_available) {			\
			partition = site.partitions[crtcs.partition];				\
			if (partition.used && (size_t)(crtcs.crtc) < partition.crtcs_used)	\
				partition.crtcs[crtcs.crtc].settings.PROP = PROP;		\
		}										\
	}

void
gamma_update_gamma(gamma_server_state_t *state, gamma_crtc_selection_t crtcs, float gamma)
{
	if (gamma < MIN_GAMMA) gamma = MIN_GAMMA;
#ifdef MAX_BRIGHTNESS
	if (gamma > MAX_GAMMA) gamma = MAX_GAMMA;
#endif
	__update(gamma)
}

void
gamma_update_brightness(gamma_server_state_t *state, gamma_crtc_selection_t crtcs, float brightness)
{
	if (brightness < MIN_BRIGHTNESS) brightness = MIN_BRIGHTNESS;
#ifdef MAX_BRIGHTNESS
	if (brightness > MAX_BRIGHTNESS) brightness = MAX_BRIGHTNESS;
#endif
	__update(brightness)
}

void
gamma_update_temperature(gamma_server_state_t *state, gamma_crtc_selection_t crtcs, float temperature)
{
	if (temperature < MIN_TEMP) temperature = MIN_TEMP;
	if (temperature > MAX_TEMP) temperature = MAX_TEMP;
	__update(temperature)
}

#undef __update
#undef __test_all
#undef __test


/* Duplicate memory area. */
static void *
memdup(void *src, size_t n)
{
	char *dest = malloc(n);
	if (dest == NULL)
		return NULL;
	memcpy(dest, src, n);
	return dest;
}


/* Parse and apply an option. */
int
gamma_set_option(gamma_server_state_t *state, const char *key, char *value, ssize_t section)
{
	int r;

	if (section == (ssize_t)(state->selections_made)) {
		/* Grow array with selections, we temporarily store
		   the new array a temporarily variable so that we can
		   properly release resources on error. */
		gamma_selection_state_t *sels;
		size_t alloc_size = state->selections_made + 1;
		alloc_size *= sizeof(gamma_selection_state_t);
		sels = realloc(state->selections, alloc_size);
		if (sels == NULL) {
			perror("realloc");
			return -1;
		}
		state->selections = sels;

		/* Copy default selection. */
		sels[section] = *sels;
		if (sels->sites != NULL) {
			size_t i;
			sels[section].sites = memdup(sels->sites,
						     sels->sites_count * sizeof(char *));
			if (sels[section].sites == NULL) {
				perror("memdup");
				return -1;
			}
			for (i = 0; i < sels->sites_count; i++) {
				sels[section].sites[i] = strdup(sels->sites[i]);
				if (sels[section].sites[i] == NULL) {
					perror("strdup");
					return -1;
				}
			}
		}
		if (sels->partitions != NULL) {
			sels[section].partitions = memdup(sels->partitions,
							  sels->partitions_count * sizeof(size_t));
			if (sels[section].partitions == NULL) {
				perror("memdup");
				return -1;
			}
		}
		if (sels->crtcs != NULL) {
			sels[section].crtcs = memdup(sels->crtcs,
						     sels->crtcs_count * sizeof(size_t));
			if (sels[section].crtcs == NULL) {
				perror("memdup");
				return -1;
			}
		}

		/* Increment this last, so we do not get segfault on error. */
		state->selections_made += 1;
	}

	if (strcasecmp(key, "gamma") == 0) {
		float gamma[3];
		if (parse_gamma_string(value, gamma) < 0) {
			fputs(_("Malformed gamma setting.\n"),
			      stderr);
			return -1;
		}
#ifdef MAX_GAMMA
		if (gamma[0] < MIN_GAMMA || gamma[0] > MAX_GAMMA ||
		    gamma[1] < MIN_GAMMA || gamma[1] > MAX_GAMMA ||
		    gamma[2] < MIN_GAMMA || gamma[2] > MAX_GAMMA) {
			fprintf(stderr,
				_("Gamma value must be between %.1f and %.1f.\n"),
				MIN_GAMMA, MAX_GAMMA);
			return -1;
		}
#else
		if (gamma[0] < MIN_GAMMA ||
		    gamma[1] < MIN_GAMMA ||
		    gamma[2] < MIN_GAMMA) {
			fprintf(stderr,
				_("Gamma value must be atleast %.1f.\n"),
				MIN_GAMMA);
			return -1;
		}
#endif
		if (section >= 0) {
			state->selections[section].settings.gamma_correction[0] = gamma[0];
			state->selections[section].settings.gamma_correction[1] = gamma[1];
			state->selections[section].settings.gamma_correction[2] = gamma[2];
		} else {
			for (size_t i = 0; i < state->selections_made; i++) {
				state->selections[i].settings.gamma_correction[0] = gamma[0];
				state->selections[i].settings.gamma_correction[1] = gamma[1];
				state->selections[i].settings.gamma_correction[2] = gamma[2];
			}
		}
	} else {
		r = state->set_option(state, key, value, section);
		if (r <= 0)
			return r;
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}
	return 0;
}


/* A gamma string contains either one floating point value,
   or three values separated by colon. */
int
parse_gamma_string(char *str, float gamma[3])
{
	char *s = strchr(str, ':');
	if (s == NULL) {
		/* Use value for all channels. */
		float g = atof(str);
		gamma[0] = gamma[1] = gamma[2] = g;
	} else {
		/* Parse separate value for each channel. */
		*(s++) = '\0';
		char *g_s = s;
		s = strchr(s, ':');
		if (s == NULL) return -1;

		*(s++) = '\0';
		gamma[0] = atof(str); /* Red   */
		gamma[1] = atof(g_s); /* Blue  */
		gamma[2] = atof(s);   /* Green */
	}

	return 0;
}


/* Parse an array for size_t:s. */
static size_t *
parse_size_t_array(char *value, char delimiter, const char *name, size_t *n_out)
{
	size_t n = 0;
	char *begin;
	size_t *array;

	/* Count number of specified values and split the values. */
	begin = value;
	while (1) {
		char *end = strchr(begin, delimiter);
		int last = end == NULL;
		if (last) end = strchr(begin, '\0');

		end[0] = '\0';
		n++;

		if (last) break;
		begin = end + 1;
	}

	/* Allocate array and store element count for caller. */
	array = malloc(n * sizeof(size_t));
	if (array == NULL) {
		perror("malloc");
		return NULL;
	}
	*n_out = n;

	/* Convert to array. */
	begin = value;
	for (size_t i = 0; i < n; i++) {
		char *end;
		ssize_t parsed = strtol(begin, &end, 10);

		if (begin[0] == '\0' || end[0] != '\0' || parsed < 0) {
			/* TRANSLATORS: `all' must not be translated. */
			fprintf(stderr, _("%s must be `all' or a non-negative integer.\n"), name);
			return NULL;
		}

		array[i] = (size_t)parsed;
		begin = end + 1;
	}

	return array;
}

/* Parse an array for char *:s. */
static char **
parse_str_array(char *value, char delimiter, size_t *n_out)
{
	size_t n = 0;
	char *begin;
	char **array;

	/* Count number of specified values and split the values. */
	begin = value;
	while (1) {
		char *end = strchr(begin, delimiter);
		int last = end == NULL;
		if (last) end = strchr(begin, '\0');

		end[0] = '\0';
		n++;

		if (last) break;
		begin = end + 1;
	}

	/* Allocate array and store element count for caller. */
	array = malloc(n * sizeof(char *));
	if (array == NULL) {
		perror("malloc");
		return NULL;
	}
	*n_out = n;

	/* Convert to array. */
	begin = value;
	for (size_t i = 0; i < n; i++) {
		char *end = strchr(begin, '\0');
		array[i] = begin;
		begin = end + 1;
	}

	return array;
}


/* Parse CRTC selection option. */
int
gamma_select_crtcs(gamma_server_state_t *state, char *value, char delimiter,
		   ssize_t section, const char *name)
{
	if (strcasecmp(value, "all") == 0) {
		on_selections({
			sel->crtcs = NULL;
			sel->crtcs_count = 0;
		});
	} else {
		size_t n;
		size_t *values = parse_size_t_array(value, delimiter, name, &n);
		if (values == NULL)
			return -1;
		on_selections({
			sel->crtcs = memdup(values, n * sizeof(size_t));
			if (sel->crtcs == NULL) {
				perror("memdup");
				free(values);
				return -1;
			}
			sel->crtcs_count = n;
		});
		free(values);
	}
	return 0;
}


/* Parse partition (e.g. screen) selection option. */
int
gamma_select_partitions(gamma_server_state_t *state, char *value, char delimiter,
			ssize_t section, const char *name)
{
	if (strcasecmp(value, "all") == 0) {
		on_selections({
			sel->partitions = NULL;
			sel->partitions_count = 0;
		});
	} else {
		size_t n;
		size_t *values = parse_size_t_array(value, delimiter, name, &n);
		if (values == NULL)
			return -1;
		on_selections({
			sel->partitions = memdup(values, n * sizeof(size_t));
			if (sel->partitions == NULL) {
				perror("memdup");
				free(values);
				return -1;
			}
			sel->partitions_count = n;
		});
		free(values);
	}
	return 0;
}


/* Parse site (e.g. display) selection option. */
int
gamma_select_sites(gamma_server_state_t *state, char *value, char delimiter, ssize_t section)
{
	size_t n;
	char **values = parse_str_array(value, delimiter, &n);
	if (values == NULL)
		  return -1;
	on_selections({
		sel->sites = malloc(n * sizeof(char *));
		if (sel->sites == NULL) {
			perror("malloc");
			goto fail;
		}
		sel->sites_count = 0;
		for (size_t i = 0; i < n; i++) {
			sel->sites[i] = strdup(values[i]);
			if (sel->sites[i] == NULL) {
				perror("strdup");
				goto fail;
			}
			sel->sites_count++;
		}
	});
	free(values);
	return 0;

fail:
	free(values);
	return -1;
}
