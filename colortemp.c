/* colortemp.c -- X color temperature adjustment source
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
#include <stdint.h>
#include <math.h>

#include <xcb/xcb.h>
#include <xcb/randr.h>


/* Source: http://www.vendian.org/mncharity/dir3/blackbody/
   Rescaled to make exactly 6500K equal to full intensity in all channels. */
static const float blackbody_color[] = {
	1.0000, 0.0425, 0.0000, /* 1000K */
	1.0000, 0.0668, 0.0000, /* 1100K */
	1.0000, 0.0911, 0.0000, /* 1200K */
	1.0000, 0.1149, 0.0000, /* ... */
	1.0000, 0.1380, 0.0000,
	1.0000, 0.1604, 0.0000,
	1.0000, 0.1819, 0.0000,
	1.0000, 0.2024, 0.0000,
	1.0000, 0.2220, 0.0000,
	1.0000, 0.2406, 0.0000,
	1.0000, 0.2630, 0.0062,
	1.0000, 0.2868, 0.0155,
	1.0000, 0.3102, 0.0261,
	1.0000, 0.3334, 0.0379,
	1.0000, 0.3562, 0.0508,
	1.0000, 0.3787, 0.0650,
	1.0000, 0.4008, 0.0802,
	1.0000, 0.4227, 0.0964,
	1.0000, 0.4442, 0.1136,
	1.0000, 0.4652, 0.1316,
	1.0000, 0.4859, 0.1505,
	1.0000, 0.5062, 0.1702,
	1.0000, 0.5262, 0.1907,
	1.0000, 0.5458, 0.2118,
	1.0000, 0.5650, 0.2335,
	1.0000, 0.5839, 0.2558,
	1.0000, 0.6023, 0.2786,
	1.0000, 0.6204, 0.3018,
	1.0000, 0.6382, 0.3255,
	1.0000, 0.6557, 0.3495,
	1.0000, 0.6727, 0.3739,
	1.0000, 0.6894, 0.3986,
	1.0000, 0.7058, 0.4234,
	1.0000, 0.7218, 0.4485,
	1.0000, 0.7375, 0.4738,
	1.0000, 0.7529, 0.4992,
	1.0000, 0.7679, 0.5247,
	1.0000, 0.7826, 0.5503,
	1.0000, 0.7970, 0.5760,
	1.0000, 0.8111, 0.6016,
	1.0000, 0.8250, 0.6272,
	1.0000, 0.8384, 0.6529,
	1.0000, 0.8517, 0.6785,
	1.0000, 0.8647, 0.7040,
	1.0000, 0.8773, 0.7294,
	1.0000, 0.8897, 0.7548,
	1.0000, 0.9019, 0.7801,
	1.0000, 0.9137, 0.8051,
	1.0000, 0.9254, 0.8301,
	1.0000, 0.9367, 0.8550,
	1.0000, 0.9478, 0.8795,
	1.0000, 0.9587, 0.9040,
	1.0000, 0.9694, 0.9283,
	1.0000, 0.9798, 0.9524,
	1.0000, 0.9900, 0.9763,
	1.0000, 1.0000, 1.0000, /* 6500K */
	0.9917, 1.0014, 1.0149,
	0.9696, 0.9885, 1.0149,
	0.9488, 0.9761, 1.0149,
	0.9290, 0.9642, 1.0149,
	0.9102, 0.9529, 1.0149,
	0.8923, 0.9420, 1.0149,
	0.8753, 0.9316, 1.0149,
	0.8591, 0.9215, 1.0149,
	0.8437, 0.9120, 1.0149,
	0.8289, 0.9028, 1.0149,
	0.8149, 0.8939, 1.0149,
	0.8014, 0.8854, 1.0149,
	0.7885, 0.8772, 1.0149,
	0.7762, 0.8693, 1.0149,
	0.7644, 0.8617, 1.0149,
	0.7531, 0.8543, 1.0149,
	0.7423, 0.8472, 1.0149,
	0.7319, 0.8404, 1.0149,
	0.7219, 0.8338, 1.0149,
	0.7123, 0.8274, 1.0149,
	0.7030, 0.8213, 1.0149,
	0.6941, 0.8152, 1.0149,
	0.6856, 0.8094, 1.0149,
	0.6773, 0.8039, 1.0149,
	0.6693, 0.7984, 1.0149,
	0.6617, 0.7932, 1.0149,
	0.6543, 0.7881, 1.0149,
	0.6471, 0.7832, 1.0149,
	0.6402, 0.7784, 1.0149,
	0.6335, 0.7737, 1.0149,
	0.6271, 0.7692, 1.0149,
	0.6208, 0.7648, 1.0149,
	0.6148, 0.7605, 1.0149,
	0.6089, 0.7564, 1.0149,
	0.6033, 0.7524, 1.0149  /* 10000K */
};


static void
interpolate_color(float a, const float *c1, const float *c2, float *c)
{
	c[0] = (1.0-a)*c1[0] + a*c2[0];
	c[1] = (1.0-a)*c1[1] + a*c2[1];
	c[2] = (1.0-a)*c1[2] + a*c2[2];
}

int
colortemp_check_extension()
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

int
colortemp_set_temperature(int temp, float gamma[3])
{
	xcb_generic_error_t *error;

	/* Open X server connection */
	xcb_connection_t *conn = xcb_connect(NULL, NULL);

	/* Get first screen */
	const xcb_setup_t *setup = xcb_get_setup(conn);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xcb_screen_t *screen = iter.data;

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
	xcb_randr_crtc_t crtc = crtcs[0];

	free(res_reply);

	/* Request size of gamma ramps */
	xcb_randr_get_crtc_gamma_size_cookie_t gamma_size_cookie =
		xcb_randr_get_crtc_gamma_size(conn, crtc);
	xcb_randr_get_crtc_gamma_size_reply_t *gamma_size_reply =
		xcb_randr_get_crtc_gamma_size_reply(conn, gamma_size_cookie,
						    &error);

	if (error) {
		fprintf(stderr, "RANDR Get CRTC Gamma Size, error: %d\n",
			error->error_code);
		xcb_disconnect(conn);
		return -1;
	}

	int gamma_ramp_size = gamma_size_reply->size;

	free(gamma_size_reply);

	if (gamma_ramp_size == 0) {
		fprintf(stderr, "Error: Gamma ramp size too small, %i\n",
			gamma_ramp_size);
		xcb_disconnect(conn);
		return -1;
	}

	/* Calculate white point */
	float white_point[3];
	float alpha = (temp % 100) / 100.0;
	int temp_index = ((temp - 1000) / 100)*3;
	interpolate_color(alpha, &blackbody_color[temp_index],
			  &blackbody_color[temp_index+3], white_point);

	/* Create new gamma ramps */
	uint16_t *gamma_ramps = malloc(3*gamma_ramp_size*sizeof(uint16_t));
	if (gamma_ramps == NULL) abort();

	uint16_t *gamma_r = &gamma_ramps[0*gamma_ramp_size];
	uint16_t *gamma_g = &gamma_ramps[1*gamma_ramp_size];
	uint16_t *gamma_b = &gamma_ramps[2*gamma_ramp_size];

	for (int i = 0; i < gamma_ramp_size; i++) {
		gamma_r[i] = pow((float)i/gamma_ramp_size, 1.0/gamma[0]) *
			UINT16_MAX * white_point[0];
		gamma_g[i] = pow((float)i/gamma_ramp_size, 1.0/gamma[1]) *
			UINT16_MAX * white_point[1];
		gamma_b[i] = pow((float)i/gamma_ramp_size, 1.0/gamma[2]) *
			UINT16_MAX * white_point[2];
	}

	/* Set new gamma ramps */
	xcb_void_cookie_t gamma_set_cookie =
		xcb_randr_set_crtc_gamma_checked(conn, crtc, gamma_ramp_size,
						 gamma_r, gamma_g, gamma_b);
	error = xcb_request_check(conn, gamma_set_cookie);

	if (error) {
		fprintf(stderr, "RANDR Set CRTC Gamma, error: %d\n",
			error->error_code);
		free(gamma_ramps);
		xcb_disconnect(conn);
		return -1;
	}

	free(gamma_ramps);

	/* Close connection */
	xcb_disconnect(conn);

	return 0;
}
