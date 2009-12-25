/* randr.c -- X RandR gamma adjustment source
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

   Copyright (c) 2009  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include "colorramp.h"


int
randr_check_extension()
{
	xcb_generic_error_t *error;

	/* Open X server connection */
	xcb_connection_t *conn = xcb_connect(NULL, NULL);

	/* Query RandR version */
	xcb_randr_query_version_cookie_t ver_cookie =
		xcb_randr_query_version(conn, 1, 3);
	xcb_randr_query_version_reply_t *ver_reply =
		xcb_randr_query_version_reply(conn, ver_cookie, &error);

	if (error) {
		fprintf(stderr, "RANDR Query Version, error: %d\n",
			error->error_code);
		xcb_disconnect(conn);
		return -1;
	}

	if (ver_reply->major_version < 1 || ver_reply->minor_version < 3) {
		fprintf(stderr, "Unsupported RANDR version (%u.%u)\n",
			ver_reply->major_version, ver_reply->minor_version);
		free(ver_reply);
		xcb_disconnect(conn);
		return -1;
	}

	free(ver_reply);

	/* Close connection */
	xcb_disconnect(conn);

	return 0;
}

static int
randr_crtc_set_temperature(xcb_connection_t *conn, xcb_randr_crtc_t crtc,
			   int temp, float gamma[3])
{
	xcb_generic_error_t *error;

	/* Request size of gamma ramps */
	xcb_randr_get_crtc_gamma_size_cookie_t gamma_size_cookie =
		xcb_randr_get_crtc_gamma_size(conn, crtc);
	xcb_randr_get_crtc_gamma_size_reply_t *gamma_size_reply =
		xcb_randr_get_crtc_gamma_size_reply(conn, gamma_size_cookie,
						    &error);

	if (error) {
		fprintf(stderr, "RANDR Get CRTC Gamma Size, error: %d\n",
			error->error_code);
		return -1;
	}

	int gamma_ramp_size = gamma_size_reply->size;

	free(gamma_size_reply);

	if (gamma_ramp_size == 0) {
		fprintf(stderr, "Gamma ramp size too small: %i\n",
			gamma_ramp_size);
		return -1;
	}

	/* Create new gamma ramps */
	uint16_t *gamma_ramps = malloc(3*gamma_ramp_size*sizeof(uint16_t));
	if (gamma_ramps == NULL) abort();

	uint16_t *gamma_r = &gamma_ramps[0*gamma_ramp_size];
	uint16_t *gamma_g = &gamma_ramps[1*gamma_ramp_size];
	uint16_t *gamma_b = &gamma_ramps[2*gamma_ramp_size];

	colorramp_fill(gamma_r, gamma_g, gamma_b, gamma_ramp_size,
		       temp, gamma);

	/* Set new gamma ramps */
	xcb_void_cookie_t gamma_set_cookie =
		xcb_randr_set_crtc_gamma_checked(conn, crtc, gamma_ramp_size,
						 gamma_r, gamma_g, gamma_b);
	error = xcb_request_check(conn, gamma_set_cookie);

	if (error) {
		fprintf(stderr, "RANDR Set CRTC Gamma, error: %d\n",
			error->error_code);
		free(gamma_ramps);
		return -1;
	}

	free(gamma_ramps);

	return 0;
}

int
randr_set_temperature(int screen_num, int temp, float gamma[3])
{
	xcb_generic_error_t *error;

	/* Open X server connection */
	int preferred_screen;
	xcb_connection_t *conn = xcb_connect(NULL, &preferred_screen);

	if (screen_num < 0) screen_num = preferred_screen;

	/* Get screen */
	const xcb_setup_t *setup = xcb_get_setup(conn);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xcb_screen_t *screen = NULL;

	for (int i = 0; iter.rem > 0; i++) {
		if (i == screen_num) {
			screen = iter.data;
			break;
		}
		xcb_screen_next(&iter);
	}

	if (screen == NULL) {
		fprintf(stderr, "Screen %i could not be found.\n",
			screen_num);
		xcb_disconnect(conn);
		return -1;
	}

	/* Get list of CRTCs for the screen */
	xcb_randr_get_screen_resources_current_cookie_t res_cookie =
		xcb_randr_get_screen_resources_current(conn, screen->root);
	xcb_randr_get_screen_resources_current_reply_t *res_reply =
		xcb_randr_get_screen_resources_current_reply(conn, res_cookie,
							     &error);

	if (error) {
		fprintf(stderr, "RANDR Get Screen Resources Current,"
			" error: %d\n", error->error_code);
		xcb_disconnect(conn);
		return -1;
	}

	xcb_randr_crtc_t *crtcs =
		xcb_randr_get_screen_resources_current_crtcs(res_reply);

	for (int i = 0; i < res_reply->num_crtcs; i++) {
		int r = randr_crtc_set_temperature(conn, crtcs[i],
						   temp, gamma);
		if (r < 0) {
			fprintf(stderr, "WARNING: Unable to adjust"
				" CRTC %i.\n", i);
		}
	}

	free(res_reply);

	/* Close connection */
	xcb_disconnect(conn);

	return 0;
}
