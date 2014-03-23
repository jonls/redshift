/* gamma-vidmode.h -- X VidMode gamma adjustment header
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

#ifndef REDSHIFT_GAMMA_VIDMODE_H
#define REDSHIFT_GAMMA_VIDMODE_H

#include <stdio.h>
#include <stdint.h>

#include <X11/Xlib.h>

typedef struct {
	int screen_num;
	int ramp_size;
	uint16_t *saved_ramps;
	float gamma[3];
	uint16_t *gamma_r;
	uint16_t *gamma_g;
	uint16_t *gamma_b;
} vidmode_screen_state_t;

typedef struct {
	int screen_num;
	float gamma[3];
} vidmode_selection_t;

typedef struct {
	Display *display;
	int selection_count;
	vidmode_selection_t *selections;
	int screen_count;
	int screens_used;
	vidmode_screen_state_t *screens;
} vidmode_state_t;


int vidmode_init(vidmode_state_t *state);
int vidmode_start(vidmode_state_t *state);
void vidmode_free(vidmode_state_t *state);

void vidmode_print_help(FILE *f);
int vidmode_set_option(vidmode_state_t *state, const char *key,
		       const char *value, int section);

void vidmode_restore(vidmode_state_t *state);
int vidmode_set_temperature(vidmode_state_t *state, int temp, float brightness,
			    int calibrations);


#endif /* ! REDSHIFT_GAMMA_VIDMODE_H */
