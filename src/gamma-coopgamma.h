/* gamma-coopgamma.h -- coopgamma gamma adjustment header
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

   Copyright (c) 2016  Mattias Andrée <maandree@member.fsf.org>
*/

#ifndef REDSHIFT_GAMMA_COOPGAMMA_H
#define REDSHIFT_GAMMA_COOPGAMMA_H

#include <libcoopgamma.h>

#include "redshift.h"


typedef struct {
	char *edid;
	size_t index;
} coopgamma_output_id_t;  

typedef struct {
	libcoopgamma_filter_t filter;
	libcoopgamma_ramps_t plain_ramps;
	size_t rampsize;
} coopgamma_crtc_state_t;

typedef struct {
	libcoopgamma_context_t ctx;
	coopgamma_crtc_state_t *crtcs;
	size_t n_crtcs;
	char **methods;
	char *method;
	char *site;
	int64_t priority;
	int list_outputs;
	coopgamma_output_id_t *outputs;
	size_t n_outputs;
	size_t a_outputs;
} coopgamma_state_t;


int coopgamma_init(coopgamma_state_t *state);
int coopgamma_start(coopgamma_state_t *state, program_mode_t mode);
void coopgamma_free(coopgamma_state_t *state);

void coopgamma_print_help(FILE *f);
int coopgamma_set_option(coopgamma_state_t *state, const char *key, const char *value);

void coopgamma_restore(coopgamma_state_t *state);
int coopgamma_set_temperature(coopgamma_state_t *state,
			      const color_setting_t *setting);


#endif /* ! REDSHIFT_GAMMA_COOPGAMMA_H */
