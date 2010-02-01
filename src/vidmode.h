/* vidmode.h -- X VidMode gamma adjustment header
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

#ifndef _REDSHIFT_VIDMODE_H
#define _REDSHIFT_VIDMODE_H

#include <stdint.h>

#include <X11/Xlib.h>

typedef struct {
	Display *display;
	int screen_num;
	int ramp_size;
	uint16_t *saved_ramps;
} vidmode_state_t;

int vidmode_init(vidmode_state_t *state, int screen_num);
void vidmode_free(vidmode_state_t *state);
void vidmode_restore(vidmode_state_t *state);
int vidmode_set_temperature(vidmode_state_t *state, int temp, float gamma[3]);

#endif /* ! _REDSHIFT_VIDMODE_H */
