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

#ifndef _REDSHIFT_GAMMA_RANDR_H
#define _REDSHIFT_GAMMA_RANDR_H

#include <stdio.h>
#include <stdint.h>

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include "redshift.h"


typedef struct {
	xcb_randr_crtc_t crtc;
	unsigned int ramp_size;
	uint16_t *saved_ramps;
} randr_crtc_state_t;

typedef struct {
	xcb_connection_t *conn;
	xcb_screen_t *screen;
	int crtc_num;
	unsigned int crtc_count;
	randr_crtc_state_t *crtcs;
} randr_state_t;


int randr_init(randr_state_t *state, char *args);
void randr_free(randr_state_t *state);
void randr_print_help(FILE *f);
void randr_restore(randr_state_t *state);
int randr_set_temperature(randr_state_t *state, int temp, float gamma[3]);


#endif /* ! _REDSHIFT_GAMMA_RANDR_H */
