#include "transition.h"
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
