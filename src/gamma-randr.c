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

#include "gamma-randr.h"

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


#define RANDR_VERSION_MAJOR  1
#define RANDR_VERSION_MINOR  3



static void
randr_free_site(void *data)
{
	/* Close connection. */
	xcb_disconnect((xcb_connection_t *)data);
}

static void
randr_free_partition(void *data)
{
	if (((randr_screen_data_t *)data)->crtcs)
		free(((randr_screen_data_t *)data)->crtcs);
	free(data);
}

static void
randr_free_crtc(void *data)
{
	(void) data;
}

static int
randr_open_site(gamma_server_state_t *state, char *site, gamma_site_state_t *site_out)
{
	(void) state;

	xcb_generic_error_t *error;

	/* Open X server connection. */
	xcb_connection_t *connection = xcb_connect(site, NULL);

	/* Query RandR version. */
	xcb_randr_query_version_cookie_t ver_cookie =
		xcb_randr_query_version(connection, RANDR_VERSION_MAJOR,
					RANDR_VERSION_MINOR);
	xcb_randr_query_version_reply_t *ver_reply =
		xcb_randr_query_version_reply(connection, ver_cookie, &error);

	/* TODO What does it mean when both error and ver_reply is NULL?
	   Apparently, we have to check both to avoid seg faults. */
	if (error || ver_reply == NULL) {
		int ec = (error != 0) ? error->error_code : -1;
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Query Version", ec);
		xcb_disconnect(connection);
		return -1;
	}

	if (ver_reply->major_version != RANDR_VERSION_MAJOR ||
	    ver_reply->minor_version < RANDR_VERSION_MINOR) {
		fprintf(stderr, _("Unsupported RANDR version (%u.%u)\n"),
			ver_reply->major_version, ver_reply->minor_version);
		free(ver_reply);
		xcb_disconnect(connection);
		return -1;
	}

	site_out->data = connection;

	/* Get the number of available screens. */
	const xcb_setup_t *setup = xcb_get_setup(connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	site_out->partitions_available = (size_t)(iter.rem);

	free(ver_reply);
	return 0;
}

static int
randr_open_partition(gamma_server_state_t *state, gamma_site_state_t *site,
		     size_t partition, gamma_partition_state_t *partition_out)
{
	(void) state;

	randr_screen_data_t *data = malloc(sizeof(randr_screen_data_t));
	partition_out->data = data;
	if (data == NULL) {
		perror("malloc");
		return -1;
	}
	data->crtcs = NULL;

	xcb_connection_t *connection = site->data;

	/* Get screen. */
	const xcb_setup_t *setup = xcb_get_setup(connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xcb_screen_t *screen = NULL;

	for (size_t i = 0; iter.rem > 0; i++) {
		if (i == partition) {
			screen = iter.data;
			break;
		}
		xcb_screen_next(&iter);
	}

	if (screen == NULL) {
		fprintf(stderr, _("Screen %ld could not be found.\n"),
			partition);
		free(data);
		return -1;
	}

	data->screen = *screen;

	xcb_generic_error_t *error;

	/* Get list of CRTCs for the screen. */
	xcb_randr_get_screen_resources_current_cookie_t res_cookie =
		xcb_randr_get_screen_resources_current(connection, screen->root);
	xcb_randr_get_screen_resources_current_reply_t *res_reply =
		xcb_randr_get_screen_resources_current_reply(connection,
							     res_cookie,
							     &error);

	if (error) {
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Get Screen Resources Current",
			error->error_code);
		free(data);
		return -1;
	}

	partition_out->crtcs_available = res_reply->num_crtcs;

	xcb_randr_crtc_t *crtcs =
		xcb_randr_get_screen_resources_current_crtcs(res_reply);

	data->crtcs = malloc(res_reply->num_crtcs * sizeof(xcb_randr_crtc_t));
	if (data->crtcs == NULL) {
		perror("malloc");
		free(res_reply);
		free(data);
		return -1;
	}
	memcpy(data->crtcs, crtcs, res_reply->num_crtcs * sizeof(xcb_randr_crtc_t));

	free(res_reply);
	return 0;
}

static int
randr_open_crtc(gamma_server_state_t *state, gamma_site_state_t *site,
		gamma_partition_state_t *partition, size_t crtc, gamma_crtc_state_t *crtc_out)
{
	(void) state;

	randr_screen_data_t *screen_data = partition->data;
	xcb_randr_crtc_t *crtc_id = screen_data->crtcs + crtc;
	xcb_connection_t *connection = site->data;
	xcb_generic_error_t *error;

	crtc_out->data = crtc_id;

	/* Request size of gamma ramps. */
	xcb_randr_get_crtc_gamma_size_cookie_t gamma_size_cookie =
		xcb_randr_get_crtc_gamma_size(connection, *crtc_id);
	xcb_randr_get_crtc_gamma_size_reply_t *gamma_size_reply =
		xcb_randr_get_crtc_gamma_size_reply(connection,
						    gamma_size_cookie,
						    &error);

	if (error) {
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Get CRTC Gamma Size",
			error->error_code);
		return -1;
	}

	ssize_t ramp_size = gamma_size_reply->size;
	size_t ramp_memsize = (size_t)ramp_size * sizeof(uint16_t);
	free(gamma_size_reply);

	if (ramp_size < 2) {
		fprintf(stderr, _("Gamma ramp size too small: %ld\n"),
			ramp_size);
		return -1;
	}

	crtc_out->saved_ramps.red_size   = (size_t)ramp_size;
	crtc_out->saved_ramps.green_size = (size_t)ramp_size;
	crtc_out->saved_ramps.blue_size  = (size_t)ramp_size;

	/* Allocate space for saved gamma ramps. */
	crtc_out->saved_ramps.red   = malloc(3 * ramp_memsize);
	crtc_out->saved_ramps.green = crtc_out->saved_ramps.red   + ramp_size;
	crtc_out->saved_ramps.blue  = crtc_out->saved_ramps.green + ramp_size;
	if (crtc_out->saved_ramps.red == NULL) {
		perror("malloc");
		return -1;
	}

	/* Request current gamma ramps. */
	xcb_randr_get_crtc_gamma_cookie_t gamma_get_cookie =
		xcb_randr_get_crtc_gamma(connection, *crtc_id);
	xcb_randr_get_crtc_gamma_reply_t *gamma_get_reply =
		xcb_randr_get_crtc_gamma_reply(connection,
					       gamma_get_cookie,
					       &error);

	if (error) {
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Get CRTC Gamma", error->error_code);
		return -1;
	}

	uint16_t *gamma_r = xcb_randr_get_crtc_gamma_red(gamma_get_reply);
	uint16_t *gamma_g = xcb_randr_get_crtc_gamma_green(gamma_get_reply);
	uint16_t *gamma_b = xcb_randr_get_crtc_gamma_blue(gamma_get_reply);

	/* Copy gamma ramps into CRTC state. */
	memcpy(crtc_out->saved_ramps.red,   gamma_r, ramp_memsize);
	memcpy(crtc_out->saved_ramps.green, gamma_g, ramp_memsize);
	memcpy(crtc_out->saved_ramps.blue,  gamma_b, ramp_memsize);

	free(gamma_get_reply);
	return 0;
}

static void
randr_invalid_partition(const gamma_site_state_t *site, size_t partition)
{
	fprintf(stderr, _("Screen %ld does not exist. "),
		partition);
	if (site->partitions_available > 1) {
		fprintf(stderr, _("Valid screens are [0-%ld].\n"),
			site->partitions_available - 1);
	} else {
		fprintf(stderr, _("Only screen 0 exists, did you mean CRTC %ld?\n"),
			partition);
	}
}

static int
randr_set_ramps(gamma_server_state_t *state, gamma_crtc_state_t *crtc, gamma_ramps_t ramps)
{
	xcb_connection_t *connection = state->sites[crtc->site_index].data;
	xcb_generic_error_t *error;

	/* Set new gamma ramps */
	xcb_void_cookie_t gamma_set_cookie =
		xcb_randr_set_crtc_gamma_checked(connection, *(xcb_randr_crtc_t *)(crtc->data),
						 ramps.red_size, ramps.red, ramps.green, ramps.blue);
	error = xcb_request_check(connection, gamma_set_cookie);

	if (error) {
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Set CRTC Gamma", error->error_code);
		return -1;
	}

	return 0;
}

static int
randr_set_option(gamma_server_state_t *state, const char *key, char *value, ssize_t section)
{
	if (strcasecmp(key, "screen") == 0) {
		return gamma_select_partitions(state, value, ',', section, _("Screen"));
	} else if (strcasecmp(key, "crtc") == 0) {
		return gamma_select_crtcs(state, value, ',', section, _("CRTC"));
	} else if (strcasecmp(key, "display") == 0) {
		return gamma_select_sites(state, value, ',', section);
	}
	return 1;

strdup_fail:
	perror("strdup");
	return -1;
}


int
randr_init(gamma_server_state_t *state)
{
	int r;
	r = gamma_init(state);
	if (r != 0) return r;

	state->free_site_data      = randr_free_site;
	state->free_partition_data = randr_free_partition;
	state->free_crtc_data      = randr_free_crtc;
	state->open_site           = randr_open_site;
	state->open_partition      = randr_open_partition;
	state->open_crtc           = randr_open_crtc;
	state->invalid_partition   = randr_invalid_partition;
	state->set_ramps           = randr_set_ramps;
	state->set_option          = randr_set_option;

	state->selections->sites = malloc(1 * sizeof(char *));
	if (state->selections->sites == NULL) {
		perror("malloc");
		return -1;
	}

	if (getenv("DISPLAY") != NULL) {
		state->selections->sites[0] = strdup(getenv("DISPLAY"));
		if (state->selections->sites[0] == NULL) {
			perror("strdup");
			return -1;
		}
	} else {
		state->selections->sites[0] = NULL;
	}
	state->selections->sites_count = 1;

	return 0;
}

int
randr_start(gamma_server_state_t *state)
{
	return gamma_resolve_selections(state);
}

void
randr_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with the X RANDR extension.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: RANDR help output
	   left column must not be translated */
	fputs(_("  crtc=N\tList of comma separated CRTCs to apply adjustments to\n"
		"  screen=N\tList of comma separated X screens to apply adjustments to\n"
		"  display=NAME\tList of comma separated X displays to apply adjustments to\n"), f);
	fputs("\n", f);
}
