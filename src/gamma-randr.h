/* gamma-randr.h -- X RANDR gamma adjustment header
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
*/

#ifndef REDSHIFT_GAMMA_RANDR_H
#define REDSHIFT_GAMMA_RANDR_H

#include <stdio.h>
#include <stdint.h>

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include "redshift.h"


typedef struct {
	xcb_randr_crtc_t crtc;
	unsigned int ramp_size;
	uint16_t *saved_ramps;
	float gamma[3];
	uint16_t *gamma_r;
	uint16_t *gamma_g;
	uint16_t *gamma_b;
} randr_crtc_state_t;

typedef struct {
	int screen_num;
	int crtc_num;
	float gamma[3];
} randr_selection_t;

typedef struct {
	xcb_connection_t *conn;
	int selection_count;
	randr_selection_t *selections;
	int preferred_screen;
	int crtcs_used;
	randr_crtc_state_t *crtcs;
} randr_state_t;


int randr_init(randr_state_t *state);
int randr_start(randr_state_t *state);
void randr_free(randr_state_t *state);

void randr_print_help(FILE *f);
int randr_set_option(randr_state_t *state, const char *key,
		     const char *value, int section);

void randr_restore(randr_state_t *state);
int randr_set_temperature(randr_state_t *state, int temp, float brightness);


#endif /* ! REDSHIFT_GAMMA_RANDR_H */
