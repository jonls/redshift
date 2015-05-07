#ifndef SETTINGS_H
#define SETTINGS_H


/* Set to 1 if the settings are specified in the command line. */
extern int cmdline_brightness;
extern int cmdline_gamma;
extern int cmdline_transition;
extern int cmdline_temperature;
extern int cmdline_elevation; /* FIXME this is never set, there is no cmdline flag for these settings */


int parse_gamma_string(const char *str, float gamma[]);

void parse_brightness_string(const char *str, float *bright_day, float *bright_night);


#endif /* SETTINGS_H */
