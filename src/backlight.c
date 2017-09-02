
#include "backlight.h"
#include <stdio.h>
#include <string.h>

static
int read_unsigned_int(backlight_state_t *state, 
		const char *object_name, unsigned int *value);

static
int write_unsigned_int(backlight_state_t *state, 
		const char *object_name, unsigned int value);


void backlight_set_controller(backlight_state_t *state, const char *controller_path) {
	if (!controller_path) {
		state->controller_path[0] = 0;
		return;
	}

	size_t bufsz = sizeof(state->controller_path);

	strncpy(state->controller_path, controller_path, bufsz);
	state->controller_path[bufsz-1] = 0;
}

int backlight_init(backlight_state_t *state) {
	if (!backlight_is_enabled(state))
		return 0;

	/* read the maximum value that the backlight can be set */
	if (read_unsigned_int(state, "max_brightness", 
				&(state->maximum)) != 0)
		return -1;

	if (state->maximum <= 1)
		return -1;
	
	return 0;
}

int backlight_is_enabled(backlight_state_t *state) {
	return state->controller_path[0] != 0;
}

int backlight_set_brightness(backlight_state_t *state, float brightness) {
	if (!backlight_is_enabled(state))
		return -1;

	/* round the value and make sure that the value is in range */
	unsigned int scaled_brightness = (unsigned int)(brightness * 
							state->maximum + 0.5);

	if (write_unsigned_int(state, "brightness", scaled_brightness) != 0)
		return -1;

	return 0;
}


static
int read_unsigned_int(backlight_state_t *state, 
		const char *object_name, unsigned int *value) {
	char path[256];
	snprintf(path, 256, "%s/%s", state->controller_path, object_name);
	path[255] = 0;

	FILE *file = fopen(path, "rb");
	if (!file) 
		return -1;

	if (fscanf(file, "%u", value) != 1) {
		fclose(file);
		return -1;
	}

	fclose(file);
	return 0;
}

static
int write_unsigned_int(backlight_state_t *state, 
		const char *object_name, unsigned int value) {
	char path[256];
	snprintf(path, 256, "%s/%s", state->controller_path, object_name);
	path[255] = 0;

	FILE *file = fopen(path, "r+b");
	if (!file) 
		return -1;

	if (fprintf(file, "%u\n", value) <= 1) {
		fclose(file);
		return -1;
	}

	fclose(file);
	return 0;
}

