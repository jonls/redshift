/* gamma-wl.h -- Wayland gamma adjustment header
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

   Copyright (c) 2015  Giulio Camuffo <giuliocamuffo@gmail.com>
*/

#ifndef REDSHIFT_GAMMA_WAYLAND_H
#define REDSHIFT_GAMMA_WAYLAND_H

#include <stdint.h>

#include <wayland-client.h>

#include "redshift.h"

typedef struct {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_callback *callback;
	uint32_t gamma_control_manager_id;
	struct gamma_control_manager *gamma_control_manager;
	int num_outputs;
	struct output *outputs;
	int authorized;
} wayland_state_t;


int wayland_init(wayland_state_t *state);
int wayland_start(wayland_state_t *state);
void wayland_free(wayland_state_t *state);

void wayland_print_help(FILE *f);
int wayland_set_option(wayland_state_t *state, const char *key, const char *value);

void wayland_restore(wayland_state_t *state);
int wayland_set_temperature(wayland_state_t *state,
			const color_setting_t *setting);


#endif /* ! REDSHIFT_GAMMA_DRM_H */
