/* gamma-wl.c -- Wayland gamma adjustment header
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <alloca.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "gamma-wl.h"
#include "colorramp.h"

#include "gamma-control-client-protocol.h"
#include "orbital-authorizer-client-protocol.h"

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

struct output {
	uint32_t global_id;
	struct wl_output *output;
	struct gamma_control *gamma_control;
	uint32_t gamma_size;
};

int
wayland_init(wayland_state_t **state)
{
	/* Initialize state. */
	*state = malloc(sizeof(**state));
	if (*state == NULL) return -1;

	memset(*state, 0, sizeof **state);
	return 0;
}

static void
authorizer_feedback_granted(void *data, struct orbital_authorizer_feedback *feedback)
{
	wayland_state_t *state = data;
	state->authorized = 1;
}

static void
authorizer_feedback_denied(void *data, struct orbital_authorizer_feedback *feedback)
{
	fprintf(stderr, _("Fatal: redshift was not authorized to bind the 'gamma_control_manager' interface.\n"));
	exit(EXIT_FAILURE);
}

static const struct orbital_authorizer_feedback_listener authorizer_feedback_listener = {
	authorizer_feedback_granted,
	authorizer_feedback_denied
};

static void
registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
	wayland_state_t *state = data;

	if (strcmp(interface, "gamma_control_manager") == 0) {
		state->gamma_control_manager_id = id;
		state->gamma_control_manager = wl_registry_bind(registry, id, &gamma_control_manager_interface, 1);
	} else if (strcmp(interface, "wl_output") == 0) {
		state->num_outputs++;
		if (!(state->outputs = realloc(state->outputs, state->num_outputs * sizeof(struct output)))) {
			fprintf(stderr, _("Failed to allcate memory\n"));
			return;
		}

		struct output *output = &state->outputs[state->num_outputs - 1];
		output->global_id = id;
		output->output = wl_registry_bind(registry, id, &wl_output_interface, 1);
		output->gamma_control = NULL;
	} else if (strcmp(interface, "orbital_authorizer") == 0) {
		struct wl_event_queue *queue = wl_display_create_queue(state->display);

		struct orbital_authorizer *auth = wl_registry_bind(registry, id, &orbital_authorizer_interface, 1u);
		wl_proxy_set_queue((struct wl_proxy *)auth, queue);

		struct orbital_authorizer_feedback *feedback = orbital_authorizer_authorize(auth, "gamma_control_manager");
		orbital_authorizer_feedback_add_listener(feedback, &authorizer_feedback_listener, state);

		int ret = 0;
		while (!state->authorized && ret >= 0) {
			ret = wl_display_dispatch_queue(state->display, queue);
		}

		orbital_authorizer_feedback_destroy(feedback);
		orbital_authorizer_destroy(auth);
		wl_event_queue_destroy(queue);
	}
}

static void
registry_global_remove(void *data, struct wl_registry *registry, uint32_t id)
{
	wayland_state_t *state = data;

	if (state->gamma_control_manager_id == id) {
		fprintf(stderr, _("The gamma_control_manager was removed\n"));
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < state->num_outputs; ++i) {
		struct output *output = &state->outputs[i];
		if (output->global_id == id) {
			gamma_control_destroy(output->gamma_control);
			wl_output_destroy(output->output);

			/* If the removed output is not the last one in the array move the last one
			 * in the now empty slot. Then shrink the array */
			if (i < --state->num_outputs) {
				memcpy(output, &state->outputs[state->num_outputs], sizeof(struct output));
			}
			state->outputs = realloc(state->outputs, state->num_outputs * sizeof(struct output));

			return;
		}
	}
}

static const struct wl_registry_listener registry_listener = {
	registry_global,
	registry_global_remove
};

static void
gamma_control_gamma_size(void *data, struct gamma_control *control, uint32_t size)
{
	struct output *output = data;
	output->gamma_size = size;
}

static const struct gamma_control_listener gamma_control_listener = {
	gamma_control_gamma_size
};

int
wayland_start(wayland_state_t *state)
{
	state->display = wl_display_connect(NULL);
	if (!state->display) {
		fputs(_("Could not connect to wayland display, exiting.\n"), stderr);
		return -1;
	}
	state->registry = wl_display_get_registry(state->display);

	wl_registry_add_listener(state->registry, &registry_listener, state);

	wl_display_roundtrip(state->display);
	if (!state->gamma_control_manager) {
		return -1;
	}
	if (state->num_outputs > 0 && !state->outputs) {
		return -1;
	}

	return 0;
}

void
wayland_restore(wayland_state_t *state)
{
	for (int i = 0; i < state->num_outputs; ++i) {
		struct output *output = &state->outputs[i];
		gamma_control_reset_gamma(output->gamma_control);
	}
	wl_display_flush(state->display);
}

void
wayland_free(wayland_state_t *state)
{

	if (state->callback) {
		wl_callback_destroy(state->callback);
	}

	for (int i = 0; i < state->num_outputs; ++i) {
		struct output *output = &state->outputs[i];
		gamma_control_destroy(output->gamma_control);
		wl_output_destroy(output->output);
	}

	if (state->gamma_control_manager) {
		gamma_control_manager_destroy(state->gamma_control_manager);
	}
	if (state->registry) {
		wl_registry_destroy(state->registry);
	}
	if (state->display) {
		wl_display_disconnect(state->display);
	}

	free(state);
}

void
wayland_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with a Wayland compositor.\n"), f);
	fputs("\n", f);
}

int
wayland_set_option(wayland_state_t *state, const char *key, const char *value)
{
	return 0;
}

static void
callback_done(void *data, struct wl_callback *cb, uint32_t t)
{
	wayland_state_t *state = data;
	state->callback = NULL;
	wl_callback_destroy(cb);
}

static const struct wl_callback_listener callback_listener = {
	callback_done
};

int
wayland_set_temperature(wayland_state_t *state, const color_setting_t *setting)
{
	int ret = 0, roundtrip = 0;
	struct wl_array red;
	struct wl_array green;
	struct wl_array blue;
	uint16_t *r_gamma = NULL;
	uint16_t *g_gamma = NULL;
	uint16_t *b_gamma = NULL;

	/* We wait for the sync callback to throttle a bit and not send more
	 * requests than the compositor can manage, otherwise we'd get disconnected.
	 * This also allows us to dispatch other incoming events such as
	 * wl_registry.global_remove. */
	while (state->callback && ret >= 0) {
		ret = wl_display_dispatch(state->display);
	}
	if (ret < 0) {
		fprintf(stderr, _("The Wayland connection experienced a fatal error: %d\n"), ret);
		return ret;
	}

	for (int i = 0; i < state->num_outputs; ++i) {
		struct output *output = &state->outputs[i];
		if (!output->gamma_control) {
			output->gamma_control = gamma_control_manager_get_gamma_control(state->gamma_control_manager, output->output);
			gamma_control_add_listener(output->gamma_control, &gamma_control_listener, output);
			roundtrip = 1;
		}
	}
	if (roundtrip) {
		wl_display_roundtrip(state->display);
	}

	wl_array_init(&red);
	wl_array_init(&green);
	wl_array_init(&blue);

	for (int i = 0; i < state->num_outputs; ++i) {
		struct output *output = &state->outputs[i];
		int size = output->gamma_size;
		size_t byteSize = size * sizeof(uint16_t);

		if (red.size < byteSize) {
			wl_array_add(&red, byteSize - red.size);
		}
		if (green.size < byteSize) {
			wl_array_add(&green, byteSize - green.size);
		}
		if (blue.size < byteSize) {
			wl_array_add(&blue, byteSize - blue.size);
		}

		r_gamma = red.data;
		g_gamma = green.data;
		b_gamma = blue.data;

		if (!r_gamma || !g_gamma || !b_gamma) {
			return -1;
		}

		/* Initialize gamma ramps to pure state */
		for (int i = 0; i < size; i++) {
			uint16_t value = (double)i / size * (UINT16_MAX+1);
			r_gamma[i] = value;
			g_gamma[i] = value;
			b_gamma[i] = value;
		}

		colorramp_fill(r_gamma, g_gamma, b_gamma, size, setting);

		gamma_control_set_gamma(output->gamma_control, &red, &green, &blue);
	}

	state->callback = wl_display_sync(state->display);
	wl_callback_add_listener(state->callback, &callback_listener, state);
	wl_display_flush(state->display);

	wl_array_release(&red);
	wl_array_release(&green);
	wl_array_release(&blue);

	return 0;
}

const gamma_method_t wl_gamma_method = {
	"wayland",
	1,
	(gamma_method_init_func *) wayland_init,
	(gamma_method_start_func *) wayland_start,
	(gamma_method_free_func *) wayland_free,
	(gamma_method_print_help_func *) wayland_print_help,
	(gamma_method_set_option_func *) wayland_set_option,
	(gamma_method_restore_func *) wayland_restore,
	(gamma_method_set_temperature_func *) wayland_set_temperature,
};
