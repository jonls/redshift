#ifndef REDSHIFT_BACKLIGHT_H
#define REDSHIFT_BACKLIGHT_H

#define BACKLIGHT_BRIGHTNESS_MIN_FRACTION (0.001)

typedef struct {
	const char *controller_path;
	unsigned int maximum;
	unsigned int minimum;
} backlight_state_t;

int backlight_init(backlight_state_t *state, const char *controller_path);

int backlight_is_enabled(backlight_state_t *state);

int backlight_set_brightness(backlight_state_t *state, float backlight);


#endif /* ! REDSHIFT_BACKLIGHT_H */
