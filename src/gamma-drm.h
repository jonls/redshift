/* gamma-drm.h -- DRM gamma adjustment header
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

   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#ifndef _REDSHIFT_GAMMA_DRM_H
#define _REDSHIFT_GAMMA_DRM_H

#include <stdint.h>

#include <xf86drm.h>
#include <xf86drmMode.h>


typedef struct {
	int crtc_num;
	int crtc_id;
	int gamma_size;
	uint16_t* r_gamma;
	uint16_t* g_gamma;
	uint16_t* b_gamma;
} drm_crtc_state_t;

typedef struct {
	int card_num;
	int crtc_num;
	int fd;
	drmModeRes* res;
	drm_crtc_state_t* crtcs;
} drm_state_t;


int drm_init(drm_state_t *state);
int drm_start(drm_state_t *state);
void drm_free(drm_state_t *state);

void drm_print_help(FILE *f);
int drm_set_option(drm_state_t *state, const char *key, const char *value);

void drm_restore(drm_state_t *state);
int drm_set_temperature(drm_state_t *state, int temp, float brightness, const float gamma[3]);


#endif /* ! _REDSHIFT_GAMMA_DRM_H */
