/* options.h -- Program options header
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

   Copyright (c) 2017  Jon Lund Steffensen <jonlst@gmail.com>
*/

#ifndef REDSHIFT_OPTIONS_H
#define REDSHIFT_OPTIONS_H

#include "redshift.h"

typedef struct {
	/* Path to config file */
	char *config_filepath;

	transition_scheme_t scheme;
	program_mode_t mode;
	int verbose;

	/* Temperature to set in manual mode. */
	int temp_set;
	/* Whether to fade between large skips in color temperature. */
	int use_fade;
	/* Whether to preserve gamma ramps if supported by gamma method. */
	int preserve_gamma;

	/* Selected gamma method. */
	const gamma_method_t *method;
    /* Options for adjustment methods */
    unsigned short method_crtc;
    unsigned short method_screen;
    unsigned short method_drm_card;

	/* Selected location provider. */
	const location_provider_t *provider;
	/* Lat, lon for location provider. */
	float provider_manual_arg_lat;
    float provider_manual_arg_lon;
} options_t;


int
options_load_from_elektra(
        options_t *options,
        Elektra *elektra,
        const gamma_method_t *gamma_methods,
        const location_provider_t *location_providers);
void options_init(options_t *options);

#endif /* ! REDSHIFT_OPTIONS_H */
