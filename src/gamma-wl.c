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
#include <sys/mman.h>
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
#include "os-compatibility.h"
#include "colorramp.h"

#include "gamma-control-client-protocol.h"
#include "orbital-authorizer-client-protocol.h"

typedef struct {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_callback *callback;
	uint32_t gamma_control_manager_id;
	struct zwlr_gamma_control_manager_v1 *gamma_control_manager;
	int num_outputs;
	struct output *outputs;
	int authorized;
} wayland_state_t;

struct output {
	uint32_t global_id;
	struct wl_output *output;
	struct zwlr_gamma_control_v1 *gamma_control;
	uint32_t gamma_size;
};

static int
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
	fprintf(stderr, _("Fatal: redshift was not authorized to bind the 'zwlr_gamma_control_manager_v1' interface.\n"));
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

	if (strcmp(interface, "zwlr_gamma_control_manager_v1") == 0) {
		state->gamma_control_manager_id = id;
		state->gamma_control_manager = wl_registry_bind(registry, id, &zwlr_gamma_control_manager_v1_interface, 1);
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

		struct orbital_authorizer_feedback *feedback = orbital_authorizer_authorize(auth, "zwlr_gamma_control_manager_v1");
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
		fprintf(stderr, _("The zwlr_gamma_control_manager_v1 was removed\n"));
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < state->num_outputs; ++i) {
		struct output *output = &state->outputs[i];
		if (output->global_id == id) {
			if (output->gamma_control) {
				zwlr_gamma_control_v1_destroy(output->gamma_control);
				output->gamma_control = NULL;
			}
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
gamma_control_gamma_size(void *data, struct zwlr_gamma_control_v1 *control, uint32_t size)
{
	struct output *output = data;
	output->gamma_size = size;
}

static void
gamma_control_failed(void *data, struct zwlr_gamma_control_v1 *control)
{
}


static const struct zwlr_gamma_control_v1_listener gamma_control_listener = {
	gamma_control_gamma_size,
	gamma_control_failed
};

static int
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

static void
wayland_restore(wayland_state_t *state)
{
	for (int i = 0; i < state->num_outputs; ++i) {
		struct output *output = &state->outputs[i];
		if (output->gamma_control) {
			zwlr_gamma_control_v1_destroy(output->gamma_control);
			output->gamma_control = NULL;
		}
	}
	wl_display_flush(state->display);
}

static void
wayland_free(wayland_state_t *state)
{
	int ret = 0;
	/* Wait for the sync callback to destroy everything, otherwise
	 * we could destroy the gamma control before gamma has been set */
	while (state->callback && ret >= 0) {
		ret = wl_display_dispatch(state->display);
	}
	if (state->callback) {
		fprintf(stderr, _("Ignoring error on wayland connection while waiting to disconnect: %d\n"), ret);
		wl_callback_destroy(state->callback);
	}

	for (int i = 0; i < state->num_outputs; ++i) {
		struct output *output = &state->outputs[i];
		if (output->gamma_control) {
			zwlr_gamma_control_v1_destroy(output->gamma_control);
			output->gamma_control = NULL;
		}
		wl_output_destroy(output->output);
	}

	if (state->gamma_control_manager) {
		zwlr_gamma_control_manager_v1_destroy(state->gamma_control_manager);
	}
	if (state->registry) {
		wl_registry_destroy(state->registry);
	}
	if (state->display) {
		wl_display_disconnect(state->display);
	}

	free(state);
}

static void
wayland_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with a Wayland compositor.\n"), f);
	fputs("\n", f);
}

static int
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

static int
wayland_set_temperature(wayland_state_t *state, const color_setting_t *setting)
{
	int ret = 0, roundtrip = 0;

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
			output->gamma_control = zwlr_gamma_control_manager_v1_get_gamma_control(state->gamma_control_manager, output->output);
			zwlr_gamma_control_v1_add_listener(output->gamma_control, &gamma_control_listener, output);
			roundtrip = 1;
		}
	}
	if (roundtrip) {
		wl_display_roundtrip(state->display);
	}

	for (int i = 0; i < state->num_outputs; ++i) {
		struct output *output = &state->outputs[i];
		int size = output->gamma_size;
		size_t ramp_bytes = size * sizeof(uint16_t);
		size_t total_bytes = ramp_bytes * 3;

		int fd = os_create_anonymous_file(total_bytes);
		if (fd < 0) {
			return -1;
		}

		void *ptr = mmap(NULL, total_bytes,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (ptr == MAP_FAILED) {
			close(fd);
			return -1;
		}

		uint16_t *r_gamma = ptr;
		uint16_t *g_gamma = ptr + ramp_bytes;
		uint16_t *b_gamma = ptr + 2 * ramp_bytes;

		/* Initialize gamma ramps to pure state */
		for (int i = 0; i < size; i++) {
			uint16_t value = (double)i / size * (UINT16_MAX+1);
			r_gamma[i] = value;
			g_gamma[i] = value;
			b_gamma[i] = value;
		}

		colorramp_fill(r_gamma, g_gamma, b_gamma, size, setting);
		munmap(ptr, size);

		zwlr_gamma_control_v1_set_gamma(output->gamma_control, fd);
		close(fd);
	}

	state->callback = wl_display_sync(state->display);
	wl_callback_add_listener(state->callback, &callback_listener, state);
	wl_display_flush(state->display);

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
