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
   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#ifndef REDSHIFT_GAMMA_RANDR_H
#define REDSHIFT_GAMMA_RANDR_H

#include "gamma-common.h"

#include <stdio.h>
#include <stdint.h>

#include <xcb/xcb.h>
#include <xcb/randr.h>


typedef struct {
	xcb_screen_t screen;
	xcb_randr_crtc_t *crtcs;
} randr_screen_data_t;


int randr_init(gamma_server_state_t *state);
int randr_start(gamma_server_state_t *state);

void randr_print_help(FILE *f);


#endif /* ! REDSHIFT_GAMMA_RANDR_H */
