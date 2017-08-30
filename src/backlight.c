
#include "backlight.h"
#include <stdio.h>

static
int read_unsigned_int(backlight_state_t *state, 
		const char *object_name, unsigned int *value);

static
int write_unsigned_int(backlight_state_t *state, 
		const char *object_name, unsigned int value);

static
unsigned int in_range(backlight_state_t *state, unsigned int value);


int backlight_init(backlight_state_t *state, const char *controller_path) {
	state->controller_path = controller_path;
	if (!state->controller_path) {
		return 0;
	}

	/* read the maximum value that the backlight can be set */
	if (read_unsigned_int(state, "max_brightness", 
				&(state->maximum)) != 0)
		return -1;

	if (state->maximum <= 1)
		return -1;
	
	/* define a minimum value based on a fraction of the maximum
	 * this is a safeguard to prevent the user to set a backlight
	 * brightness of zero and put his monitor black.
	 * */
	state->minimum = (state->maximum * 
			BACKLIGHT_BRIGHTNESS_MIN_FRACTION);
	if (state->minimum == 0)
		state->minimum = 1;
	
	return 0;
}

int backlight_is_enabled(backlight_state_t *state) {
	return state->controller_path != 0;
}

int backlight_set_brightness(backlight_state_t *state, float backlight) {
	if (!state->controller_path)
		return -1;

	/* round the value and make sure that the value is in range */
	unsigned int brightness = (unsigned int)(backlight *
					state->maximum + 0.5);
	brightness = in_range(state, brightness);

	if (write_unsigned_int(state, "brightness", brightness) != 0)
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

static
unsigned int in_range(backlight_state_t *state, unsigned int value) {
	value = value > state->maximum ? state->maximum : value;
	value = value < state->minimum ? state->minimum : value;
	return value;
}
