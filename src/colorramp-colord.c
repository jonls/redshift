/* colorramp-colord.c -- calibrated color temperature calculation source
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

   Copyright (c) 2013  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <glib.h>
#include <colord.h>
#include "colorramp-common.h"

void
colorramp_colord_init() {
	if (!GLIB_CHECK_VERSION (2, 36, 0)) {
		g_type_init();
	}
}

int
colorramp_colord_fill(uint16_t *gamma_r, uint16_t *gamma_g,
			  uint16_t *gamma_b, int size, int temp,
			  float brightness, float gamma[3],
			  const char *xrandr_name)
{

	CdClient *client = NULL;
	CdDevice *device = NULL;
	CdProfile *profile = NULL;
	CdIcc *icc = NULL;
	GFile *file = NULL;
	GPtrArray *vcgt = NULL;
	const gchar *filename;
	GError *error = NULL;
	gboolean ret;
	int retval = -1;

	if (brightness != 0.0 || gamma[0] != 1.0 || 
			gamma[1] != 1.0 || gamma[2] != 1.0) {
		retval = -2;
		goto out;
	}
	/* Connect to colord */
	client = cd_client_new();
	ret = cd_client_connect_sync(client, NULL, &error);
	if (!ret) {
		g_error_free(error);
		goto out;
	}

	/* Get the device from colord */
	device = cd_client_find_device_by_property_sync(client,
		CD_DEVICE_METADATA_XRANDR_NAME, xrandr_name, NULL, &error);

	if (device == NULL) {
		g_error_free(error);
		goto out;
	}
	
	/* Get details about the device */
	ret = cd_device_connect_sync(device, NULL, &error);
	if (!ret) {
		g_error_free(error);
		goto out;
	}

	/* Get the profile from colord */
	profile = cd_device_get_default_profile(device);
	if (profile == NULL) goto out; 

	/* Get details about the profile */

	ret = cd_profile_connect_sync(profile, NULL, &error);
	if (!ret) {
		g_error_free(error);
		goto out;
	}

	/* Make sure the profile has a VCGT */
	ret = cd_profile_get_has_vcgt(profile);
	if (!ret) goto out;

	/* Get the filename of the profile */
	filename = cd_profile_get_filename(profile);
	if (filename == NULL) goto out;

	/* Load the icc file from the filename*/
	icc = cd_icc_new();
	file = g_file_new_for_path(filename);
	ret = cd_icc_load_file(icc, file, CD_ICC_LOAD_FLAGS_ALL, NULL, &error);
	if (!ret) {
		g_error_free(error);
		goto out;
	}

	/* Get the vcgt from the profile */
	vcgt = cd_icc_get_vcgt(icc, size, NULL);
	if (!vcgt) goto out;
	if (vcgt->len == 0) goto out;

	float white_point[3];
	float alpha = (temp % 100) / 100.0;
	int temp_index = ((temp - 1000) / 100)*3;
	interpolate_color(alpha, &blackbody_color[temp_index],
			&blackbody_color[temp_index+3], white_point);

	CdColorRGB *p1;
	CdColorRGB *p2;
	CdColorRGB result;
	gdouble mix;

	cd_color_rgb_set(&result, 1.0, 1.0, 1.0);
	for (int i = 0; i < size; i++) {
		mix = (gdouble) (vcgt->len - 1) /
			(gdouble) (size - 1) *
			(gdouble) i;
		p1 = g_ptr_array_index(vcgt, (guint) floor(mix));
		p2 = g_ptr_array_index(vcgt, (guint) ceil(mix));
		cd_color_rgb_interpolate(p1, p2, mix - (gint) mix, &result);
		gamma_r[i] = result.R * 0xffff * white_point[0];
		gamma_g[i] = result.G * 0xffff * white_point[1];
		gamma_b[i] = result.B * 0xffff * white_point[2];
	}

	retval = 0;

out:
	if (device != NULL) g_object_unref(device);
	if (profile != NULL) g_object_unref(profile);
	if (client != NULL) g_object_unref(client);
	if (file != NULL) g_object_unref(file);
	if (icc != NULL) g_object_unref(icc);
	return retval;
}
