#ifndef REDSHIFT_BACKLIGHT_H
#define REDSHIFT_BACKLIGHT_H

typedef struct {
	char controller_path[255];
	unsigned int maximum;
} backlight_state_t;

void backlight_set_controller(backlight_state_t *state, const char *controller_path);

int backlight_init(backlight_state_t *state);

int backlight_is_enabled(backlight_state_t *state);

int backlight_set_brightness(backlight_state_t *state, float brightness);


#endif /* ! REDSHIFT_BACKLIGHT_H */
