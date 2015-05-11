#ifndef TRANSITION_H
#define TRANSITION_H 

#include <stdio.h>
#include <stdlib.h>
#include "solar.h"
#include "redshift.h"

/* Bounds for parameters. */
#define MIN_LAT   -90.0
#define MAX_LAT    90.0
#define MIN_LON  -180.0
#define MAX_LON   180.0
#define MIN_TEMP   1000
#define MAX_TEMP  25000
#define MIN_BRIGHTNESS  0.1
#define MAX_BRIGHTNESS  1.0
#define MIN_GAMMA   0.1
#define MAX_GAMMA  10.0

/* Default values for parameters. */
#define DEFAULT_DAY_TEMP    5500
#define DEFAULT_NIGHT_TEMP  3500
#define DEFAULT_BRIGHTNESS   1.0
#define DEFAULT_GAMMA        1.0

/* The color temperature when no adjustment is applied. */
#define NEUTRAL_TEMP  6500

/* Angular elevation of the sun at which the color temperature
   transition period starts and ends (in degress).
   Transition during twilight, and while the sun is lower than
   3.0 degrees above the horizon. */
#define TRANSITION_LOW     SOLAR_CIVIL_TWILIGHT_ELEV
#define TRANSITION_HIGH    3.0

/* Transition scheme.
   The solar elevations at which the transition begins/ends,
   and the association color settings. */
typedef struct {
	double high;
	double low;
	color_setting_t day;
	color_setting_t night;
} transition_scheme_t;

void interpolate_color_settings(const transition_scheme_t *transition,
			   double elevation,
			   color_setting_t *result);

period_t get_period(const transition_scheme_t *transition,
	   double elevation);

double get_transition_progress(const transition_scheme_t *transition,
			double elevation);

void print_period(period_t period, double transition);

void print_location(const location_t *location);

#endif /* TRANSITION_H */
