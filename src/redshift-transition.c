#include "redshift-transition.h"
#include <math.h>

#define MIN(x,y)        ((x) < (y) ? (x) : (y))
#define MAX(x,y)        ((x) > (y) ? (x) : (y))
#define CLAMP(lo,x,up)  (MAX((lo), MIN((x), (up))))

/* Names of periods of day */
static const char *period_names[] = {
	/* TRANSLATORS: Name printed when period of day is unknown */
	N_("None"),
	N_("Daytime"),
	N_("Night"),
	N_("Transition")
};

/* Interpolate color setting structs based on solar elevation */
void
interpolate_color_settings(const transition_scheme_t *transition,
			   double elevation,
			   color_setting_t *result)
{
	const color_setting_t *day = &transition->day;
	const color_setting_t *night = &transition->night;

	double alpha = (transition->low - elevation) /
		(transition->low - transition->high);
	alpha = CLAMP(0.0, alpha, 1.0);

	result->temperature = (1.0-alpha)*night->temperature +
		alpha*day->temperature;
	result->brightness = (1.0-alpha)*night->brightness +
		alpha*day->brightness;
	for (int i = 0; i < 3; i++) {
		result->gamma[i] = (1.0-alpha)*night->gamma[i] +
			alpha*day->gamma[i];
	}
}

/* Determine which period we are currently in. */
period_t
get_period(const transition_scheme_t *transition,
	   double elevation)
{
	if (elevation < transition->low) {
		return PERIOD_NIGHT;
	} else if (elevation < transition->high) {
		return PERIOD_TRANSITION;
	} else {
		return PERIOD_DAYTIME;
	}
}

/* Determine how far through the transition we are. */
double
get_transition_progress(const transition_scheme_t *transition,
			double elevation)
{
	if (elevation < transition->low) {
		return 0.0;
	} else if (elevation < transition->high) {
		return (transition->low - elevation) /
			(transition->low - transition->high);
	} else {
		return 1.0;
	}
}

/* Print verbose description of the given period. */
void
print_period(period_t period, double transition)
{
	switch (period) {
	case PERIOD_NONE:
	case PERIOD_NIGHT:
	case PERIOD_DAYTIME:
		printf(_("Period: %s\n"), gettext(period_names[period]));
		break;
	case PERIOD_TRANSITION:
		printf(_("Period: %s (%.2f%% day)\n"),
			   gettext(period_names[period]),
			   transition*100);
		break;
	}
}

/* Check whether gamma is within allowed levels. */
int
gamma_is_valid(const float gamma[3])
{
	return !(gamma[0] < MIN_GAMMA ||
		 gamma[0] > MAX_GAMMA ||
		 gamma[1] < MIN_GAMMA ||
		 gamma[1] > MAX_GAMMA ||
		 gamma[2] < MIN_GAMMA ||
		 gamma[2] > MAX_GAMMA);

}

/* Print location */
void
print_location(const location_t *location)
{
	/* TRANSLATORS: Abbreviation for `north' */
	const char *north = _("N");
	/* TRANSLATORS: Abbreviation for `south' */
	const char *south = _("S");
	/* TRANSLATORS: Abbreviation for `east' */
	const char *east = _("E");
	/* TRANSLATORS: Abbreviation for `west' */
	const char *west = _("W");

	/* TRANSLATORS: Append degree symbols after %f if possible.
	   The string following each number is an abreviation for
	   north, source, east or west (N, S, E, W). */
	printf(_("Location: %.2f %s, %.2f %s\n"),
		   fabs(location->lat), location->lat >= 0.f ? north : south,
		   fabs(location->lon), location->lon >= 0.f ? east : west);
}

transition_scheme_t *
transition_scheme_new(void) 
{
	/* Settings for day, night and transition.
	 Initialized to indicate that the values are not set yet. */
	 
	transition_scheme_t * scheme = malloc(sizeof(transition_scheme_t));
	scheme->high = TRANSITION_HIGH;
	scheme->low = TRANSITION_LOW;
	scheme->day.temperature = -1;
	scheme->day.gamma[0] = NAN;
	scheme->day.brightness = NAN;

	scheme->night.temperature = -1;
	scheme->night.gamma[0] = NAN;
	scheme->night.brightness = NAN;
	return scheme; 
}

void transition_scheme_finalize(transition_scheme_t *scheme) 
{	
	if (scheme != NULL) {
		free(scheme);
	}
}

void transition_scheme_set_default(transition_scheme_t *scheme) 
{
	/* Use default values for settings that were neither defined in
	   the config file nor on the command line. */
	if (scheme->day.temperature < 0) {
		scheme->day.temperature = DEFAULT_DAY_TEMP;
	}
	if (scheme->night.temperature < 0) {
		scheme->night.temperature = DEFAULT_NIGHT_TEMP;
	}

	if (isnan(scheme->day.brightness)) {
		scheme->day.brightness = DEFAULT_BRIGHTNESS;
	}
	if (isnan(scheme->night.brightness)) {
		scheme->night.brightness = DEFAULT_BRIGHTNESS;
	}

	if (isnan(scheme->day.gamma[0])) {
		scheme->day.gamma[0] = DEFAULT_GAMMA;
		scheme->day.gamma[1] = DEFAULT_GAMMA;
		scheme->day.gamma[2] = DEFAULT_GAMMA;
	}
	if (isnan(scheme->night.gamma[0])) {
		scheme->night.gamma[0] = DEFAULT_GAMMA;
		scheme->night.gamma[1] = DEFAULT_GAMMA;
		scheme->night.gamma[2] = DEFAULT_GAMMA;
	}
}

int 
transition_scheme_validate(transition_scheme_t *scheme, int verbose) 
{
	
	/* Color temperature */
	if (scheme->day.temperature < MIN_TEMP ||
		scheme->day.temperature > MAX_TEMP ||
		scheme->night.temperature < MIN_TEMP ||
		scheme->night.temperature > MAX_TEMP) {
		fprintf(stderr,
			_("Temperature must be between %uK and %uK.\n"),
			MIN_TEMP, MAX_TEMP);
		return -1;
	}

	/* Solar elevations */
	if (scheme->high < scheme->low) {
			fprintf(stderr,
					_("High transition elevation cannot be lower than"
			  " the low transition elevation.\n"));
			return -1;
	}

	/* Brightness */
	if (scheme->day.brightness < MIN_BRIGHTNESS ||
		scheme->day.brightness > MAX_BRIGHTNESS ||
		scheme->night.brightness < MIN_BRIGHTNESS ||
		scheme->night.brightness > MAX_BRIGHTNESS) {
		fprintf(stderr,
			_("Brightness values must be between %.1f and %.1f.\n"),
			MIN_BRIGHTNESS, MAX_BRIGHTNESS);
		return -1;
	}

	if (verbose) {
		printf(_("Brightness: %.2f:%.2f\n"),
			   scheme->day.brightness, scheme->night.brightness);
	}

	/* Gamma */
	if (!gamma_is_valid(scheme->day.gamma) ||
		!gamma_is_valid(scheme->night.gamma)) {
		fprintf(stderr,
			_("Gamma value must be between %.1f and %.1f.\n"),
			MIN_GAMMA, MAX_GAMMA);
		return -1;
	}

	if (verbose) {
		/* TRANSLATORS: The string in parenthesis is either
		   Daytime or Night (translated). */
		printf(_("Gamma (%s): %.3f, %.3f, %.3f\n"),
			   _("Daytime"), scheme->day.gamma[0],
			   scheme->day.gamma[1], scheme->day.gamma[2]);
		printf(_("Gamma (%s): %.3f, %.3f, %.3f\n"),
			   _("Night"), scheme->night.gamma[0],
			   scheme->night.gamma[1], scheme->night.gamma[2]);
	}
	return 0;
}
