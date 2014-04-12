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
#include <alloca.h>

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
		ssize_t screen = strcasecmp(value, "all") ? (ssize_t)atoi(value) : -1;
		if (screen < 0 && strcasecmp(value, "all")) {
			/* TRANSLATORS: `all' must not be translated. */
			fprintf(stderr, _("Screen must be `all' or a non-negative integer.\n"));
			return -1;
		}
		on_selections({ sel->partition = screen; });
		return 0;
	} else if (strcasecmp(key, "crtc") == 0) {
		ssize_t crtc = strcasecmp(value, "all") ? (ssize_t)atoi(value) : -1;
		if (crtc < 0 && strcasecmp(value, "all")) {
			/* TRANSLATORS: `all' must not be translated. */
			fprintf(stderr, _("CRTC must be `all' or a non-negative integer.\n"));
			return -1;
		}
		on_selections({ sel->crtc = crtc; });
		return 0;
	} else if (strcasecmp(key, "display") == 0) {
		on_selections({
			sel->site = strdup(value);
			if (sel->site == NULL)
				goto strdup_fail;
		});
		return 0;
	} else if (strcasecmp(key, "edid") == 0) {
		int edid_length = (int)(strlen(value));
		if (edid_length == 0 || edid_length % 2 != 0) {
			fputs(_("Malformated EDID, should be nonempty even-length hexadecimal.\n"), stderr);
			return -1;
		}
		if (edid_length > MAX_EDID_LENGTH * 2) {
			fprintf(stderr, _("EDID was too long, it may not be longer than %i characters.\n"),
				MAX_EDID_LENGTH * 2);
			return -1;
		}
#define __range(LOWER, TEST, UPPER)  ((LOWER) <= (TEST) && (TEST) <= (UPPER))
		/* Convert EDID to raw byte format, from hexadecimal. */
		unsigned char edid[MAX_EDID_LENGTH];
		for (int i = 0; i < edid_length; i += 2) {
			char a = value[i];
			char b = value[i + 1];
			if (__range('0', a, '9'))
				edid[i >> 1] = (unsigned char)((a & 15) << 4);
			else if (__range('a', a | ('a' ^ 'A'), 'f')) {
				a += 9; /* ('a' & 15) == 1*/
				edid[i >> 1] = (unsigned char)((a & 15) << 4);
			} else {
				fputs(_("Malformated EDID, should be nonempty even-length hexadecimal.\n"),
				      stderr);
				return -1;
			}
			if (__range('0', b, '9'))
				edid[i >> 1] |= (unsigned char)(b & 15);
			else if (__range('a', b | ('a' ^ 'A'), 'f')) {
				b += 9;
				edid[i >> 1] |= (unsigned char)(b & 15);
			} else {
				fputs(_("Malformated EDID, should be nonempty even-length hexadecimal.\n"),
				      stderr);
				return -1;
			}
		}
		edid_length /= 2;
#undef __range
		on_selections({
			randr_selection_data_t *sel_data = sel->data;
			if (sel_data == NULL) {
				sel_data = sel->data = malloc(sizeof(randr_selection_data_t));
				if (sel_data == NULL) {
					perror("malloc");
					return -1;
				}
			}
			sel_data->edid_length = edid_length;
			memcpy(sel_data->edid, edid, (size_t)edid_length);
		});
		return 0;
	}
	return 1;

strdup_fail:
	perror("strdup");
	return -1;
}


static int
randr_test_edid(xcb_connection_t *connection, xcb_randr_output_t output, xcb_atom_t atom,
		gamma_selection_state_t *selection, xcb_randr_crtc_t *crtcs, int crtc_n,
		xcb_randr_crtc_t output_crtc)
{
	randr_selection_data_t *selection_data = selection->data;
	unsigned char *edid_seeked = selection_data->edid;
	int edid_length = selection_data->edid_length;

	xcb_randr_get_output_property_cookie_t val_cookie;
	xcb_randr_get_output_property_reply_t *val_reply;
	xcb_generic_error_t *error;

	/* Acquire the property's value, we know that it is either 128 bytes
	   or 256 bytes long, and we have a limit defined in gamma-randr.h. */
	val_cookie = xcb_randr_get_output_property(connection, output, atom,
						   XCB_GET_PROPERTY_TYPE_ANY,
						   0, MAX_EDID_LENGTH, 0, 0);

	val_reply = xcb_randr_get_output_property_reply(connection, val_cookie, &error);

	if (error) {
		fprintf(stderr, _("`%s' returned error %d\n"),
			"RANDR Get Output Property", error->error_code);
		return -1;
	}

	unsigned char* value = xcb_randr_get_output_property_data(val_reply);
	int length = xcb_randr_get_output_property_data_length(val_reply);

	/* Compare found EDID against seeked EDID. */
	if ((length == edid_length) && !memcmp(value, edid_seeked, (size_t)length)) {
		int crtc;

		/* Find CRTC index. */
		for (crtc = 0; crtc < crtc_n; crtc++)
			if (crtcs[crtc] == output_crtc)
				break;

		free(val_reply);

		if (crtc < crtc_n) {
			selection->crtc = (ssize_t)crtc;
		} else {
			fputs(_("Monitor is not connected."), stderr);
			return -1;
		}

		return 0;
	}
	free(val_reply);
	return 1;
}


static int
randr_parse_selection(gamma_server_state_t *state, gamma_site_state_t *site,
		      gamma_selection_state_t *selection, enum gamma_selection_hook when) {
	(void) state;

	if (when != before_crtc)
		return 0;

	xcb_connection_t *connection = site->data;

	/* Select screen to look in, only one will be used. */
	gamma_partition_state_t *screen_start;
	gamma_partition_state_t *screen_end;
	if (selection->partition >= 0) {
		screen_start = site->partitions + selection->partition;
		screen_end = screen_start + 1;
	} else {
		screen_start = site->partitions;
		screen_end = screen_start + site->partitions_available;
	}

	for (gamma_partition_state_t *screen = screen_start; screen != screen_end; screen++) {
		if (screen->used == 0)
			continue;
		randr_screen_data_t *screen_data = screen->data;

		xcb_randr_get_screen_resources_current_cookie_t res_cookie;
		xcb_randr_get_screen_resources_current_reply_t *res_reply;
		xcb_generic_error_t *error;

		/* Acquire information about the screen. */
		res_cookie = xcb_randr_get_screen_resources_current(connection, screen_data->screen.root);
		res_reply = xcb_randr_get_screen_resources_current_reply(connection, res_cookie, &error);

		if (error) {
			fprintf(stderr, _("`%s' returned error %d\n"),
				"RANDR Get Screen Resources Current",
				error->error_code);
			return -1;
		}

		xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);

		/* Iterate over all connectors to and look for one that is connected
		   and whose monitor is has matching extended display identification data. */
		for (int output_i = 0; output_i < res_reply->num_outputs; output_i++) {
			xcb_randr_get_output_info_cookie_t out_cookie;
			xcb_randr_get_output_info_reply_t *out_reply;

			/* Acquire information about the output. */
			out_cookie = xcb_randr_get_output_info(connection, outputs[output_i],
							       res_reply->config_timestamp);
			out_reply = xcb_randr_get_output_info_reply(connection, out_cookie, &error);

			if (error) {
				fprintf(stderr, _("`%s' returned error %d\n"),
					"RANDR Get Output Information",
					error->error_code);
				free(res_reply);
				return -1;
			}

			/* Check connection status. */
			if (out_reply->connection != XCB_RANDR_CONNECTION_CONNECTED) {
				free(out_reply);
				continue;
			}

			xcb_randr_list_output_properties_cookie_t prop_cookie;
			xcb_randr_list_output_properties_reply_t *prop_reply;
			xcb_atom_t *atoms;
			xcb_atom_t *atoms_end;

			/* Acquire a list of all properties of the output. */
			prop_cookie = xcb_randr_list_output_properties(connection, outputs[output_i]);
			prop_reply = xcb_randr_list_output_properties_reply(connection, prop_cookie, &error);

			if (error) {
				fprintf(stderr, _("`%s' returned error %d\n"),
					"RANDR List Output Properties",
					error->error_code);
				free(out_reply);
				free(res_reply);
				return -1;
			}

			atoms = xcb_randr_list_output_properties_atoms(prop_reply);
			atoms_end = atoms + xcb_randr_list_output_properties_atoms_length(prop_reply);

			for (; atoms != atoms_end; atoms++) {
				xcb_get_atom_name_cookie_t atom_name_cookie;
				xcb_get_atom_name_reply_t *atom_name_reply;
				char *atom_name;
				char *atom_name_;
				int atom_name_len;

				/* Aqcuire the atom name. */
				atom_name_cookie = xcb_get_atom_name(connection, *atoms);
				atom_name_reply = xcb_get_atom_name_reply(connection, atom_name_cookie, &error);

				if (error) {
					fprintf(stderr, _("`%s' returned error %d\n"),
						"RANDR Get Atom Name",
						error->error_code);
					free(prop_reply);
					free(out_reply);
					free(res_reply);
					return -1;
				}

				atom_name_ = xcb_get_atom_name_name(atom_name_reply);
				atom_name_len = xcb_get_atom_name_name_length(atom_name_reply);

				/* NUL-terminate the atom name. */
				atom_name = alloca(((size_t)atom_name_len + 1) * sizeof(char));
				memcpy(atom_name, atom_name_, (size_t)atom_name_len * sizeof(char));
				*(atom_name + atom_name_len) = 0;

				if (!strcmp(atom_name, "EDID")) {
					int r = randr_test_edid(connection, outputs[output_i], *atoms,
								selection, screen_data->crtcs,
								(int)(screen->crtcs_available),
								out_reply->crtc);
					if (r != 1) {
						free(atom_name_reply);
						free(prop_reply);
						free(out_reply);
						free(res_reply);
						selection->partition = (ssize_t)(screen - screen_start);
						return r;
					}
				}
				free(atom_name_reply);
			}
			free(prop_reply);
			free(out_reply);
		}
		free(res_reply);
	}
	return -1;
}


int
randr_init(gamma_server_state_t *state)
{
	int r;
	r = gamma_init(state);
	if (r != 0) return r;

	state->selections->sizeof_data = sizeof(randr_selection_data_t);
	state->selections->site        = getenv("DISPLAY") ? strdup(getenv("DISPLAY")) : NULL;
	state->free_site_data          = randr_free_site;
	state->free_partition_data     = randr_free_partition;
	state->free_crtc_data          = randr_free_crtc;
	state->open_site               = randr_open_site;
	state->open_partition          = randr_open_partition;
	state->open_crtc               = randr_open_crtc;
	state->invalid_partition       = randr_invalid_partition;
	state->set_ramps               = randr_set_ramps;
	state->set_option              = randr_set_option;
	state->parse_selection         = randr_parse_selection;

	if (getenv("DISPLAY") != NULL && state->selections->site == NULL) {
		perror("strdup");
		return -1;
	}

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
	fputs(_("  crtc=N\tCRTC to apply adjustments to\n"
		"  edid=VALUE\tThe EDID of the monitor to apply adjustments to\n"
		"  screen=N\tX screen to apply adjustments to\n"
		"  display=NAME\tX display to apply adjustments to\n"), f);
	fputs("\n", f);
}
