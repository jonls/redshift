/* redshift.c -- Main program source
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

   Copyright (c) 2009-2015  Jon Lund Steffensen <jonlst@gmail.com>
   Copyright (c) 2014-2015  Mattias Andrée <maandree@member.fsf.org>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include <errno.h>

#if defined(HAVE_SIGNAL_H) && !defined(__WIN32__)
# include <signal.h>
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
# define N_(s) (s)
#else
# define _(s) s
# define N_(s) s
# define gettext(s) s
#endif

#include "redshift.h"
#include "config-ini.h"
#include "solar.h"
#include "systemtime.h"
#include "hooks.h"


#define MIN(x,y)        ((x) < (y) ? (x) : (y))
#define MAX(x,y)        ((x) > (y) ? (x) : (y))
#define CLAMP(lo,x,up)  (MAX((lo), MIN((x), (up))))

/* pause() is not defined on windows platform but is not needed either.
   Use a noop macro instead. */
#ifdef __WIN32__
# define pause()
#endif

#include "gamma-dummy.h"

#ifdef ENABLE_DRM
# include "gamma-drm.h"
#endif

#ifdef ENABLE_RANDR
# include "gamma-randr.h"
#endif

#ifdef ENABLE_VIDMODE
# include "gamma-vidmode.h"
#endif

#ifdef ENABLE_QUARTZ
# include "gamma-quartz.h"
#endif

#ifdef ENABLE_WINGDI
# include "gamma-w32gdi.h"
#endif


#include "location-manual.h"

#ifdef ENABLE_GEOCLUE
# include "location-geoclue.h"
#endif

#ifdef ENABLE_GEOCLUE2
# include "location-geoclue2.h"
#endif

#ifdef ENABLE_CORELOCATION
# include "location-corelocation.h"
#endif


/* Union of state data for gamma adjustment methods */
typedef union {
#ifdef ENABLE_DRM
	drm_state_t drm;
#endif
#ifdef ENABLE_RANDR
	randr_state_t randr;
#endif
#ifdef ENABLE_VIDMODE
	vidmode_state_t vidmode;
#endif
#ifdef ENABLE_QUARTZ
	quartz_state_t quartz;
#endif
#ifdef ENABLE_WINGDI
	w32gdi_state_t w32gdi;
#endif
} gamma_state_t;


/* Gamma adjustment method structs */
static const gamma_method_t gamma_methods[] = {
#ifdef ENABLE_DRM
	{
		"drm", 0,
		(gamma_method_init_func *)drm_init,
		(gamma_method_start_func *)drm_start,
		(gamma_method_free_func *)drm_free,
		(gamma_method_print_help_func *)drm_print_help,
		(gamma_method_set_option_func *)drm_set_option,
		(gamma_method_restore_func *)drm_restore,
		(gamma_method_set_temperature_func *)drm_set_temperature
	},
#endif
#ifdef ENABLE_RANDR
	{
		"randr", 1,
		(gamma_method_init_func *)randr_init,
		(gamma_method_start_func *)randr_start,
		(gamma_method_free_func *)randr_free,
		(gamma_method_print_help_func *)randr_print_help,
		(gamma_method_set_option_func *)randr_set_option,
		(gamma_method_restore_func *)randr_restore,
		(gamma_method_set_temperature_func *)randr_set_temperature
	},
#endif
#ifdef ENABLE_VIDMODE
	{
		"vidmode", 1,
		(gamma_method_init_func *)vidmode_init,
		(gamma_method_start_func *)vidmode_start,
		(gamma_method_free_func *)vidmode_free,
		(gamma_method_print_help_func *)vidmode_print_help,
		(gamma_method_set_option_func *)vidmode_set_option,
		(gamma_method_restore_func *)vidmode_restore,
		(gamma_method_set_temperature_func *)vidmode_set_temperature
	},
#endif
#ifdef ENABLE_QUARTZ
	{
		"quartz", 1,
		(gamma_method_init_func *)quartz_init,
		(gamma_method_start_func *)quartz_start,
		(gamma_method_free_func *)quartz_free,
		(gamma_method_print_help_func *)quartz_print_help,
		(gamma_method_set_option_func *)quartz_set_option,
		(gamma_method_restore_func *)quartz_restore,
		(gamma_method_set_temperature_func *)quartz_set_temperature
	},
#endif
#ifdef ENABLE_WINGDI
	{
		"wingdi", 1,
		(gamma_method_init_func *)w32gdi_init,
		(gamma_method_start_func *)w32gdi_start,
		(gamma_method_free_func *)w32gdi_free,
		(gamma_method_print_help_func *)w32gdi_print_help,
		(gamma_method_set_option_func *)w32gdi_set_option,
		(gamma_method_restore_func *)w32gdi_restore,
		(gamma_method_set_temperature_func *)w32gdi_set_temperature
	},
#endif
	{
		"dummy", 0,
		(gamma_method_init_func *)gamma_dummy_init,
		(gamma_method_start_func *)gamma_dummy_start,
		(gamma_method_free_func *)gamma_dummy_free,
		(gamma_method_print_help_func *)gamma_dummy_print_help,
		(gamma_method_set_option_func *)gamma_dummy_set_option,
		(gamma_method_restore_func *)gamma_dummy_restore,
		(gamma_method_set_temperature_func *)gamma_dummy_set_temperature
	},
	{ NULL }
};


/* Union of state data for location providers */
typedef union {
	location_manual_state_t manual;
#ifdef ENABLE_GEOCLUE
	location_geoclue_state_t geoclue;
#endif
} location_state_t;


/* Location provider method structs */
static const location_provider_t location_providers[] = {
#ifdef ENABLE_GEOCLUE
	{
		"geoclue",
		(location_provider_init_func *)location_geoclue_init,
		(location_provider_start_func *)location_geoclue_start,
		(location_provider_free_func *)location_geoclue_free,
		(location_provider_print_help_func *)
		location_geoclue_print_help,
		(location_provider_set_option_func *)
		location_geoclue_set_option,
		(location_provider_get_location_func *)
		location_geoclue_get_location
	},
#endif
#ifdef ENABLE_GEOCLUE2
	{
		"geoclue2",
		(location_provider_init_func *)location_geoclue2_init,
		(location_provider_start_func *)location_geoclue2_start,
		(location_provider_free_func *)location_geoclue2_free,
		(location_provider_print_help_func *)
		location_geoclue2_print_help,
		(location_provider_set_option_func *)
		location_geoclue2_set_option,
		(location_provider_get_location_func *)
		location_geoclue2_get_location
	},
#endif
#ifdef ENABLE_CORELOCATION
	{
		"corelocation",
		(location_provider_init_func *)location_corelocation_init,
		(location_provider_start_func *)location_corelocation_start,
		(location_provider_free_func *)location_corelocation_free,
		(location_provider_print_help_func *)
		location_corelocation_print_help,
		(location_provider_set_option_func *)
		location_corelocation_set_option,
		(location_provider_get_location_func *)
		location_corelocation_get_location
	},
#endif
	{
		"manual",
		(location_provider_init_func *)location_manual_init,
		(location_provider_start_func *)location_manual_start,
		(location_provider_free_func *)location_manual_free,
		(location_provider_print_help_func *)
		location_manual_print_help,
		(location_provider_set_option_func *)
		location_manual_set_option,
		(location_provider_get_location_func *)
		location_manual_get_location
	},
	{ NULL }
};

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

/* Duration of sleep between screen updates (milliseconds). */
#define SLEEP_DURATION        5000
#define SLEEP_DURATION_SHORT  100

/* Program modes. */
typedef enum {
	PROGRAM_MODE_CONTINUAL,
	PROGRAM_MODE_ONE_SHOT,
	PROGRAM_MODE_PRINT,
	PROGRAM_MODE_RESET,
	PROGRAM_MODE_MANUAL
} program_mode_t;

/* Transition scheme.
   The solar elevations at which the transition begins/ends,
   and the association color settings. */
typedef struct {
	double high;
	double low;
	color_setting_t day;
	color_setting_t night;
} transition_scheme_t;

/* Names of periods of day */
static const char *period_names[] = {
	/* TRANSLATORS: Name printed when period of day is unknown */
	N_("None"),
	N_("Daytime"),
	N_("Night"),
	N_("Transition")
};

#if defined(HAVE_SIGNAL_H) && !defined(__WIN32__)

static volatile sig_atomic_t exiting = 0;
static volatile sig_atomic_t disable = 0;

/* Signal handler for exit signals */
static void
sigexit(int signo)
{
	exiting = 1;
}

/* Signal handler for disable signal */
static void
sigdisable(int signo)
{
	disable = 1;
}

#else /* ! HAVE_SIGNAL_H || __WIN32__ */

static int exiting = 0;
static int disable = 0;

#endif /* ! HAVE_SIGNAL_H || __WIN32__ */


/* Determine which period we are currently in. */
static period_t
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
static double
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
static void
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

static void
twilight_print(const char* string, double value)
{
	if (isnan(value)) {
		printf(_("%s: never\n"), string);
	} else {
		printf(_("%s: %i\n"), string, (int)(value + 0.5));
	}
}

static void
print_twilight_period(double now, double elevation, const location_t *loc, const transition_scheme_t *scheme)
{
#define __less(a, b)  ((isnan(a) || isnan(b)) ? (isnan(b) && !isnan(a)) : (a < b))
#define __min(a, b)  (__less(a, b) ? a : b)
#define __max(a, b)  (__less(a, b) ? b : a)

	double next_low, next_high, prev_low, prev_high;
	double next_sunset_sunset, prev_sunset_sunset;

	next_low  = future_elevation(now, loc->lat, loc->lon, scheme->low);
	next_high = future_elevation(now, loc->lat, loc->lon, scheme->high);
	prev_low  =   past_elevation(now, loc->lat, loc->lon, scheme->low);
	prev_high =   past_elevation(now, loc->lat, loc->lon, scheme->high);
	next_sunset_sunset = future_elevation(now, loc->lat, loc->lon, 0.0);
	prev_sunset_sunset =   past_elevation(now, loc->lat, loc->lon, 0.0);

	if (elevation > 0.0)  twilight_print(_("Previous sunrise"), prev_sunset_sunset);
	else                  twilight_print(_("Previous sunset"),  prev_sunset_sunset);

	if (elevation > 0.0)  twilight_print(_("Next sunset"),  next_sunset_sunset);
	else                  twilight_print(_("Next sunrise"), next_sunset_sunset);

	if (elevation >= scheme->high) {
		twilight_print(_("Twilight ended"),  prev_high);
		twilight_print(_("Twilight starts"), next_high);
	} else if (elevation >= scheme->low) {
		twilight_print(_("Twilight ended"),  prev_low);
		twilight_print(_("Twilight starts"), next_low);
	} else {
		twilight_print(_("Twilight started"), __max(prev_high, prev_low));
		twilight_print(_("Twilight ends"),    __min(prev_high, prev_low));
	}

#undef __max
#undef __min
#undef __more
}

/* Print location */
static void
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

/* Interpolate color setting structs based on solar elevation */
static void
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


static void
print_help(const char *program_name)
{
	/* TRANSLATORS: help output 1
	   LAT is latitude, LON is longitude,
	   DAY is temperature at daytime,
	   NIGHT is temperature at night
	   no-wrap */
	printf(_("Usage: %s -l LAT:LON -t DAY:NIGHT [OPTIONS...]\n"),
		program_name);
	fputs("\n", stdout);

	/* TRANSLATORS: help output 2
	   no-wrap */
	fputs(_("Set color temperature of display"
		" according to time of day.\n"), stdout);
	fputs("\n", stdout);

	/* TRANSLATORS: help output 3
	   no-wrap */
	fputs(_("  -h\t\tDisplay this help message\n"
		"  -v\t\tVerbose output\n"
                "  -V\t\tShow program version\n"), stdout);
	fputs("\n", stdout);

	/* TRANSLATORS: help output 4
	   `list' must not be translated
	   no-wrap */
	fputs(_("  -b DAY:NIGHT\tScreen brightness to apply (between 0.1 and 1.0)\n"
                "  -c FILE\tLoad settings from specified configuration file\n"
		"  -g R:G:B\tAdditional gamma correction to apply\n"
		"  -l LAT:LON\tYour current location\n"
		"  -l PROVIDER\tSelect provider for automatic"
		" location updates\n"
		"  \t\t(Type `list' to see available providers)\n"
		"  -m METHOD\tMethod to use to set color temperature\n"
		"  \t\t(Type `list' to see available methods)\n"
		"  -o\t\tOne shot mode (do not continuously adjust"
		" color temperature)\n"
		"  -O TEMP\tOne shot manual mode (set color temperature)\n"
		"  -p\t\tPrint mode (only print parameters and exit)\n"
		"  -x\t\tReset mode (remove adjustment from screen)\n"
		"  -r\t\tDisable temperature transitions\n"
		"  -t DAY:NIGHT\tColor temperature to set at daytime/night\n"),
	      stdout);
	fputs("\n", stdout);

	/* TRANSLATORS: help output 5 */
	printf(_("The neutral temperature is %uK. Using this value will not\n"
		 "change the color temperature of the display. Setting the\n"
		 "color temperature to a value higher than this results in\n"
		 "more blue light, and setting a lower value will result in\n"
		 "more red light.\n"),
		 NEUTRAL_TEMP);

	fputs("\n", stdout);

	/* TRANSLATORS: help output 6 */
	printf(_("Default values:\n\n"
		 "  Daytime temperature: %uK\n"
		 "  Night temperature: %uK\n"),
	       DEFAULT_DAY_TEMP, DEFAULT_NIGHT_TEMP);

	fputs("\n", stdout);

	/* TRANSLATORS: help output 7 */
	printf(_("Please report bugs to <%s>\n"), PACKAGE_BUGREPORT);
}

static void
print_method_list()
{
	fputs(_("Available adjustment methods:\n"), stdout);
	for (int i = 0; gamma_methods[i].name != NULL; i++) {
		printf("  %s\n", gamma_methods[i].name);
	}

	fputs("\n", stdout);
	fputs(_("Specify colon-separated options with"
		" `-m METHOD:OPTIONS'.\n"), stdout);
	/* TRANSLATORS: `help' must not be translated. */
	fputs(_("Try `-m METHOD:help' for help.\n"), stdout);
}

static void
print_provider_list()
{
	fputs(_("Available location providers:\n"), stdout);
	for (int i = 0; location_providers[i].name != NULL; i++) {
		printf("  %s\n", location_providers[i].name);
	}

	fputs("\n", stdout);
	fputs(_("Specify colon-separated options with"
		"`-l PROVIDER:OPTIONS'.\n"), stdout);
	/* TRANSLATORS: `help' must not be translated. */
	fputs(_("Try `-l PROVIDER:help' for help.\n"), stdout);
}


static int
provider_try_start(const location_provider_t *provider,
		   location_state_t *state,
		   config_ini_state_t *config, char *args)
{
	int r;

	r = provider->init(state);
	if (r < 0) {
		fprintf(stderr, _("Initialization of %s failed.\n"),
			provider->name);
		return -1;
	}

	/* Set provider options from config file. */
	config_ini_section_t *section =
		config_ini_get_section(config, provider->name);
	if (section != NULL) {
		config_ini_setting_t *setting = section->settings;
		while (setting != NULL) {
			r = provider->set_option(state, setting->name,
						 setting->value);
			if (r < 0) {
				provider->free(state);
				fprintf(stderr, _("Failed to set %s"
						  " option.\n"),
					provider->name);
				/* TRANSLATORS: `help' must not be
				   translated. */
				fprintf(stderr, _("Try `-l %s:help' for more"
						  " information.\n"),
					provider->name);
				return -1;
			}
			setting = setting->next;
		}
	}

	/* Set provider options from command line. */
	const char *manual_keys[] = { "lat", "lon" };
	int i = 0;
	while (args != NULL) {
		char *next_arg = strchr(args, ':');
		if (next_arg != NULL) *(next_arg++) = '\0';

		const char *key = args;
		char *value = strchr(args, '=');
		if (value == NULL) {
			/* The options for the "manual" method can be set
			   without keys on the command line for convencience
			   and for backwards compatability. We add the proper
			   keys here before calling set_option(). */
			if (strcmp(provider->name, "manual") == 0 &&
			    i < sizeof(manual_keys)/sizeof(manual_keys[0])) {
				key = manual_keys[i];
				value = args;
			} else {
				fprintf(stderr, _("Failed to parse option `%s'.\n"),
					args);
				return -1;
			}
		} else {
			*(value++) = '\0';
		}

		r = provider->set_option(state, key, value);
		if (r < 0) {
			provider->free(state);
			fprintf(stderr, _("Failed to set %s option.\n"),
				provider->name);
			/* TRANSLATORS: `help' must not be translated. */
			fprintf(stderr, _("Try `-l %s:help' for more"
					  " information.\n"), provider->name);
			return -1;
		}

		args = next_arg;
		i += 1;
	}

	/* Start provider. */
	r = provider->start(state);
	if (r < 0) {
		provider->free(state);
		fprintf(stderr, _("Failed to start provider %s.\n"),
			provider->name);
		return -1;
	}

	return 0;
}

static int
method_try_start(const gamma_method_t *method,
		 gamma_state_t *state,
		 config_ini_state_t *config, char *args)
{
	int r;

	r = method->init(state);
	if (r < 0) {
		fprintf(stderr, _("Initialization of %s failed.\n"),
			method->name);
		return -1;
	}

	/* Set method options from config file. */
	config_ini_section_t *section =
		config_ini_get_section(config, method->name);
	if (section != NULL) {
		config_ini_setting_t *setting = section->settings;
		while (setting != NULL) {
			r = method->set_option(state, setting->name,
						 setting->value);
			if (r < 0) {
				method->free(state);
				fprintf(stderr, _("Failed to set %s"
						  " option.\n"),
					method->name);
				/* TRANSLATORS: `help' must not be
				   translated. */
				fprintf(stderr, _("Try `-m %s:help' for more"
						  " information.\n"),
					method->name);
				return -1;
			}
			setting = setting->next;
		}
	}

	/* Set method options from command line. */
	while (args != NULL) {
		char *next_arg = strchr(args, ':');
		if (next_arg != NULL) *(next_arg++) = '\0';

		const char *key = args;
		char *value = strchr(args, '=');
		if (value == NULL) {
			fprintf(stderr, _("Failed to parse option `%s'.\n"),
				args);
			return -1;
		} else {
			*(value++) = '\0';
		}

		r = method->set_option(state, key, value);
		if (r < 0) {
			method->free(state);
			fprintf(stderr, _("Failed to set %s option.\n"),
				method->name);
			/* TRANSLATORS: `help' must not be translated. */
			fprintf(stderr, _("Try -m %s:help' for more"
					  " information.\n"), method->name);
			return -1;
		}

		args = next_arg;
	}

	/* Start method. */
	r = method->start(state);
	if (r < 0) {
		method->free(state);
		fprintf(stderr, _("Failed to start adjustment method %s.\n"),
			method->name);
		return -1;
	}

	return 0;
}

/* A gamma string contains either one floating point value,
   or three values separated by colon. */
static int
parse_gamma_string(const char *str, float gamma[])
{
	char *s = strchr(str, ':');
	if (s == NULL) {
		/* Use value for all channels */
		float g = atof(str);
		gamma[0] = gamma[1] = gamma[2] = g;
	} else {
		/* Parse separate value for each channel */
		*(s++) = '\0';
		char *g_s = s;
		s = strchr(s, ':');
		if (s == NULL) return -1;

		*(s++) = '\0';
		gamma[0] = atof(str); /* Red */
		gamma[1] = atof(g_s); /* Blue */
		gamma[2] = atof(s); /* Green */
	}

	return 0;
}

/* A brightness string contains either one floating point value,
   or two values separated by a colon. */
static void
parse_brightness_string(const char *str, float *bright_day, float *bright_night)
{
	char *s = strchr(str, ':');
	if (s == NULL) {
		/* Same value for day and night. */
		*bright_day = *bright_night = atof(str);
	} else {
		*(s++) = '\0';
		*bright_day = atof(str);
		*bright_night = atof(s);
	}
}

/* Check whether gamma is within allowed levels. */
static int
gamma_is_valid(const float gamma[3])
{
	return !(gamma[0] < MIN_GAMMA ||
		 gamma[0] > MAX_GAMMA ||
		 gamma[1] < MIN_GAMMA ||
		 gamma[1] > MAX_GAMMA ||
		 gamma[2] < MIN_GAMMA ||
		 gamma[2] > MAX_GAMMA);

}

static const gamma_method_t *
find_gamma_method(const char *name)
{
	const gamma_method_t *method = NULL;
	for (int i = 0; gamma_methods[i].name != NULL; i++) {
		const gamma_method_t *m = &gamma_methods[i];
		if (strcasecmp(name, m->name) == 0) {
		        method = m;
			break;
		}
	}

	return method;
}

static const location_provider_t *
find_location_provider(const char *name)
{
	const location_provider_t *provider = NULL;
	for (int i = 0; location_providers[i].name != NULL; i++) {
		const location_provider_t *p = &location_providers[i];
		if (strcasecmp(name, p->name) == 0) {
			provider = p;
			break;
		}
	}

	return provider;
}


/* Run continual mode loop
   This is the main loop of the continual mode which keeps track of the
   current time and continuously updates the screen to the appropriate
   color temperature. */
static int
run_continual_mode(const location_t *loc,
		   const transition_scheme_t *scheme,
		   const gamma_method_t *method,
		   gamma_state_t *state,
		   int transition, int verbose)
{
	int r;

	/* Make an initial transition from 6500K */
	int short_trans_delta = -1;
	int short_trans_len = 10;

	/* Amount of adjustment to apply. At zero the color
	   temperature will be exactly as calculated, and at one it
	   will be exactly 6500K. */
	double adjustment_alpha = 1.0;

#if defined(HAVE_SIGNAL_H) && !defined(__WIN32__)
	struct sigaction sigact;
	sigset_t sigset;
	sigemptyset(&sigset);

	/* Install signal handler for INT and TERM signals */
	sigact.sa_handler = sigexit;
	sigact.sa_mask = sigset;
	sigact.sa_flags = 0;

	r = sigaction(SIGINT, &sigact, NULL);
	if (r < 0) {
		perror("sigaction");
		return -1;
	}

	r = sigaction(SIGTERM, &sigact, NULL);
	if (r < 0) {
		perror("sigaction");
		return -1;
	}

	/* Install signal handler for USR1 signal */
	sigact.sa_handler = sigdisable;
	sigact.sa_mask = sigset;
	sigact.sa_flags = 0;

	r = sigaction(SIGUSR1, &sigact, NULL);
	if (r < 0) {
		perror("sigaction");
		return -1;
	}

	/* Ignore CHLD signal. This causes child processes
	   (hooks) to be reaped automatically. */
	sigact.sa_handler = SIG_IGN;
	sigact.sa_mask = sigset;
	sigact.sa_flags = 0;

	r = sigaction(SIGCHLD, &sigact, NULL);
	if (r < 0) {
		perror("sigaction");
		return -1;
	}
#endif /* HAVE_SIGNAL_H && ! __WIN32__ */

	if (verbose) {
		printf(_("Status: %s\n"), _("Enabled"));
	}

	/* Save previous colors so we can avoid
	   printing status updates if the values
	   did not change. */
	period_t prev_period = PERIOD_NONE;
	color_setting_t prev_interp =
		{ -1, { NAN, NAN, NAN }, NAN };

	/* Continuously adjust color temperature */
	int done = 0;
	int disabled = 0;
	while (1) {
		/* Check to see if disable signal was caught */
		if (disable) {
			short_trans_len = 2;
			if (!disabled) {
				/* Transition to disabled state */
				short_trans_delta = 1;
			} else {
				/* Transition back to enabled */
				short_trans_delta = -1;
			}
			disabled = !disabled;
			disable = 0;

			if (verbose) {
				printf(_("Status: %s\n"), disabled ?
				       _("Disabled") : _("Enabled"));
			}
		}

		/* Check to see if exit signal was caught */
		if (exiting) {
			if (done) {
				/* On second signal stop the ongoing
				   transition */
				short_trans_delta = 0;
				adjustment_alpha = 0.0;
			} else {
				if (!disabled) {
					/* Make a short transition
					   back to 6500K */
					short_trans_delta = 1;
					short_trans_len = 2;
				}

				done = 1;
			}
			exiting = 0;
		}

		/* Read timestamp */
		double now;
		r = systemtime_get_time(&now);
		if (r < 0) {
			fputs(_("Unable to read system time.\n"), stderr);
			return -1;
		}

		/* Skip over transition if transitions are disabled */
		int set_adjustments = 0;
		if (!transition) {
			if (short_trans_delta) {
				adjustment_alpha = short_trans_delta < 0 ?
					0.0 : 1.0;
				short_trans_delta = 0;
				set_adjustments = 1;
			}
		}

		/* Current angular elevation of the sun */
		double elevation = solar_elevation(now, loc->lat,
						   loc->lon);

		/* Use elevation of sun to set color temperature */
		color_setting_t interp;
		interpolate_color_settings(scheme, elevation, &interp);

		/* Print period if it changed during this update,
		   or if we are in transition. In transition we
		   print the progress, so we always print it in
		   that case. */
		period_t period = get_period(scheme, elevation);
		if (verbose && (period != prev_period ||
				period == PERIOD_TRANSITION)) {
			double transition =
				get_transition_progress(scheme,
							elevation);
			print_period(period, transition);
		}

		/* Activate hooks and print twilight period if period changed */
		if (period != prev_period) {
			hooks_signal_period_change(prev_period, period);
			if (verbose) {
				print_twilight_period(now, elevation, loc, scheme);
			}
		}

		/* Ongoing short transition */
		if (short_trans_delta) {
			/* Calculate alpha */
			adjustment_alpha += short_trans_delta * 0.1 /
				(float)short_trans_len;

			/* Stop transition when done */
			if (adjustment_alpha <= 0.0 ||
			    adjustment_alpha >= 1.0) {
				short_trans_delta = 0;
			}

			/* Clamp alpha value */
			adjustment_alpha =
				MAX(0.0, MIN(adjustment_alpha, 1.0));
		}

		/* Interpolate between 6500K and calculated
		   temperature */
		interp.temperature = adjustment_alpha*6500 +
			(1.0-adjustment_alpha)*interp.temperature;

		interp.brightness = adjustment_alpha*1.0 +
			(1.0-adjustment_alpha)*interp.brightness;

		/* Quit loop when done */
		if (done && !short_trans_delta) break;

		if (verbose) {
			if (interp.temperature !=
			    prev_interp.temperature) {
				printf(_("Color temperature: %uK\n"),
				       interp.temperature);
			}
			if (interp.brightness !=
			    prev_interp.brightness) {
				printf(_("Brightness: %.2f\n"),
				       interp.brightness);
			}
		}

		/* Adjust temperature */
		if (!disabled || short_trans_delta || set_adjustments) {
			r = method->set_temperature(state, &interp);
			if (r < 0) {
				fputs(_("Temperature adjustment"
					" failed.\n"), stderr);
				return -1;
			}
		}

		/* Save temperature as previous */
		prev_period = period;
		memcpy(&prev_interp, &interp,
		       sizeof(color_setting_t));

		/* Sleep for 5 seconds or 0.1 second. */
		if (short_trans_delta) {
			systemtime_msleep(SLEEP_DURATION_SHORT);
		} else {
			systemtime_msleep(SLEEP_DURATION);
		}
	}

	/* Restore saved gamma ramps */
	method->restore(state);

	return 0;
}

int
main(int argc, char *argv[])
{
	int r;

#ifdef ENABLE_NLS
	/* Init locale */
	setlocale(LC_CTYPE, "");
	setlocale(LC_MESSAGES, "");

	/* Internationalisation */
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	/* Initialize settings to NULL values. */
	char *config_filepath = NULL;

	/* Settings for day, night and transition.
	   Initialized to indicate that the values are not set yet. */
	transition_scheme_t scheme =
		{ TRANSITION_HIGH, TRANSITION_LOW };

	scheme.day.temperature = -1;
	scheme.day.gamma[0] = NAN;
	scheme.day.brightness = NAN;

	scheme.night.temperature = -1;
	scheme.night.gamma[0] = NAN;
	scheme.night.brightness = NAN;

	/* Temperature for manual mode */
	int temp_set = -1;

	const gamma_method_t *method = NULL;
	char *method_args = NULL;

	const location_provider_t *provider = NULL;
	char *provider_args = NULL;

	int transition = -1;
	program_mode_t mode = PROGRAM_MODE_CONTINUAL;
	int verbose = 0;
	char *s;

	/* Flush messages consistently even if redirected to a pipe or
	   file.  Change the flush behaviour to line-buffered, without
	   changing the actual buffers being used. */
	setvbuf(stdout, NULL, _IOLBF, 0);
	setvbuf(stderr, NULL, _IOLBF, 0);

	/* Parse command line arguments. */
	int opt;
	while ((opt = getopt(argc, argv, "b:c:g:hl:m:oO:prt:vVx")) != -1) {
		switch (opt) {
		case 'b':
			parse_brightness_string(optarg,
						&scheme.day.brightness,
						&scheme.night.brightness);
			break;
		case 'c':
			if (config_filepath != NULL) free(config_filepath);
			config_filepath = strdup(optarg);
			break;
		case 'g':
			r = parse_gamma_string(optarg, scheme.day.gamma);
			if (r < 0) {
				fputs(_("Malformed gamma argument.\n"),
				      stderr);
				fputs(_("Try `-h' for more"
					" information.\n"), stderr);
				exit(EXIT_FAILURE);
			}

			/* Set night gamma to the same value as day gamma.
			   To set these to distinct values use the config
			   file. */
			memcpy(scheme.night.gamma, scheme.day.gamma,
			       sizeof(scheme.night.gamma));
			break;
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		case 'l':
			/* Print list of providers if argument is `list' */
			if (strcasecmp(optarg, "list") == 0) {
				print_provider_list();
				exit(EXIT_SUCCESS);
			}

			char *provider_name = NULL;

			/* Don't save the result of strtof(); we simply want
			   to know if optarg can be parsed as a float. */
			errno = 0;
			char *end;
			strtof(optarg, &end);
			if (errno == 0 && *end == ':') {
				/* Use instead as arguments to `manual'. */
				provider_name = "manual";
				provider_args = optarg;
			} else {
				/* Split off provider arguments. */
				s = strchr(optarg, ':');
				if (s != NULL) {
					*(s++) = '\0';
					provider_args = s;
				}

				provider_name = optarg;
			}

			/* Lookup provider from name. */
			provider = find_location_provider(provider_name);
			if (provider == NULL) {
				fprintf(stderr, _("Unknown location provider"
						  " `%s'.\n"), provider_name);
				exit(EXIT_FAILURE);
			}

			/* Print provider help if arg is `help'. */
			if (provider_args != NULL &&
			    strcasecmp(provider_args, "help") == 0) {
				provider->print_help(stdout);
				exit(EXIT_SUCCESS);
			}
			break;
		case 'm':
			/* Print list of methods if argument is `list' */
			if (strcasecmp(optarg, "list") == 0) {
				print_method_list();
				exit(EXIT_SUCCESS);
			}

			/* Split off method arguments. */
			s = strchr(optarg, ':');
			if (s != NULL) {
				*(s++) = '\0';
				method_args = s;
			}

			/* Find adjustment method by name. */
			method = find_gamma_method(optarg);
			if (method == NULL) {
				/* TRANSLATORS: This refers to the method
				   used to adjust colors e.g VidMode */
				fprintf(stderr, _("Unknown adjustment method"
						  " `%s'.\n"), optarg);
				exit(EXIT_FAILURE);
			}

			/* Print method help if arg is `help'. */
			if (method_args != NULL &&
			    strcasecmp(method_args, "help") == 0) {
				method->print_help(stdout);
				exit(EXIT_SUCCESS);
			}
			break;
		case 'o':
			mode = PROGRAM_MODE_ONE_SHOT;
			break;
		case 'O':
			mode = PROGRAM_MODE_MANUAL;
			temp_set = atoi(optarg);
			break;
		case 'p':
			mode = PROGRAM_MODE_PRINT;
			break;
		case 'r':
			transition = 0;
			break;
		case 't':
			s = strchr(optarg, ':');
			if (s == NULL) {
				fputs(_("Malformed temperature argument.\n"),
				      stderr);
				fputs(_("Try `-h' for more information.\n"),
				      stderr);
				exit(EXIT_FAILURE);
			}
			*(s++) = '\0';
			scheme.day.temperature = atoi(optarg);
			scheme.night.temperature = atoi(s);
			break;
		case 'v':
			verbose = 1;
			break;
                case 'V':
                        printf("%s\n", PACKAGE_STRING);
                        exit(EXIT_SUCCESS);
                        break;
		case 'x':
			mode = PROGRAM_MODE_RESET;
			break;
		case '?':
			fputs(_("Try `-h' for more information.\n"), stderr);
			exit(EXIT_FAILURE);
			break;
		}
	}

	/* Load settings from config file. */
	config_ini_state_t config_state;
	r = config_ini_init(&config_state, config_filepath);
	if (r < 0) {
		fputs("Unable to load config file.\n", stderr);
		exit(EXIT_FAILURE);
	}

	if (config_filepath != NULL) free(config_filepath);

	/* Read global config settings. */
	config_ini_section_t *section = config_ini_get_section(&config_state,
							       "redshift");
	if (section != NULL) {
		config_ini_setting_t *setting = section->settings;
		while (setting != NULL) {
			if (strcasecmp(setting->name, "temp-day") == 0) {
				if (scheme.day.temperature < 0) {
					scheme.day.temperature =
						atoi(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "temp-night") == 0) {
				if (scheme.night.temperature < 0) {
					scheme.night.temperature =
						atoi(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "transition") == 0) {
				if (transition < 0) {
					transition = !!atoi(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "brightness") == 0) {
				if (isnan(scheme.day.brightness)) {
					scheme.day.brightness =
						atof(setting->value);
				}
				if (isnan(scheme.night.brightness)) {
					scheme.night.brightness =
						atof(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "brightness-day") == 0) {
				if (isnan(scheme.day.brightness)) {
					scheme.day.brightness =
						atof(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "brightness-night") == 0) {
				if (isnan(scheme.night.brightness)) {
					scheme.night.brightness =
						atof(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "elevation-high") == 0) {
				scheme.high = atof(setting->value);
			} else if (strcasecmp(setting->name,
					      "elevation-low") == 0) {
				scheme.low = atof(setting->value);
			} else if (strcasecmp(setting->name, "gamma") == 0) {
				if (isnan(scheme.day.gamma[0])) {
					r = parse_gamma_string(setting->value,
							       scheme.day.gamma);
					if (r < 0) {
						fputs(_("Malformed gamma"
							" setting.\n"),
						      stderr);
						exit(EXIT_FAILURE);
					}
					memcpy(scheme.night.gamma, scheme.day.gamma,
					       sizeof(scheme.night.gamma));
				}
			} else if (strcasecmp(setting->name, "gamma-day") == 0) {
				if (isnan(scheme.day.gamma[0])) {
					r = parse_gamma_string(setting->value,
							       scheme.day.gamma);
					if (r < 0) {
						fputs(_("Malformed gamma"
							" setting.\n"),
						      stderr);
						exit(EXIT_FAILURE);
					}
				}
			} else if (strcasecmp(setting->name, "gamma-night") == 0) {
				if (isnan(scheme.night.gamma[0])) {
					r = parse_gamma_string(setting->value,
							       scheme.night.gamma);
					if (r < 0) {
						fputs(_("Malformed gamma"
							" setting.\n"),
						      stderr);
						exit(EXIT_FAILURE);
					}
				}
			} else if (strcasecmp(setting->name,
					      "adjustment-method") == 0) {
				if (method == NULL) {
					method = find_gamma_method(
						setting->value);
					if (method == NULL) {
						fprintf(stderr, _("Unknown"
								  " adjustment"
								  " method"
								  " `%s'.\n"),
							setting->value);
						exit(EXIT_FAILURE);
					}
				}
			} else if (strcasecmp(setting->name,
					      "location-provider") == 0) {
				if (provider == NULL) {
					provider = find_location_provider(
						setting->value);
					if (provider == NULL) {
						fprintf(stderr, _("Unknown"
								  " location"
								  " provider"
								  " `%s'.\n"),
							setting->value);
						exit(EXIT_FAILURE);
					}
				}
			} else {
				fprintf(stderr, _("Unknown configuration"
						  " setting `%s'.\n"),
					setting->name);
			}
			setting = setting->next;
		}
	}

	/* Use default values for settings that were neither defined in
	   the config file nor on the command line. */
	if (scheme.day.temperature < 0) {
		scheme.day.temperature = DEFAULT_DAY_TEMP;
	}
	if (scheme.night.temperature < 0) {
		scheme.night.temperature = DEFAULT_NIGHT_TEMP;
	}

	if (isnan(scheme.day.brightness)) {
		scheme.day.brightness = DEFAULT_BRIGHTNESS;
	}
	if (isnan(scheme.night.brightness)) {
		scheme.night.brightness = DEFAULT_BRIGHTNESS;
	}

	if (isnan(scheme.day.gamma[0])) {
		scheme.day.gamma[0] = DEFAULT_GAMMA;
		scheme.day.gamma[1] = DEFAULT_GAMMA;
		scheme.day.gamma[2] = DEFAULT_GAMMA;
	}
	if (isnan(scheme.night.gamma[0])) {
		scheme.night.gamma[0] = DEFAULT_GAMMA;
		scheme.night.gamma[1] = DEFAULT_GAMMA;
		scheme.night.gamma[2] = DEFAULT_GAMMA;
	}

	if (transition < 0) transition = 1;

	location_t loc = { NAN, NAN };

	/* Initialize location provider. If provider is NULL
	   try all providers until one that works is found. */
	location_state_t location_state;

	/* Location is not needed for reset mode and manual mode. */
	if (mode != PROGRAM_MODE_RESET &&
	    mode != PROGRAM_MODE_MANUAL) {
		if (provider != NULL) {
			/* Use provider specified on command line. */
			r = provider_try_start(provider, &location_state,
					       &config_state, provider_args);
			if (r < 0) exit(EXIT_FAILURE);
		} else {
			/* Try all providers, use the first that works. */
			for (int i = 0;
			     location_providers[i].name != NULL; i++) {
				const location_provider_t *p =
					&location_providers[i];
				fprintf(stderr,
					_("Trying location provider `%s'...\n"),
					p->name);
				r = provider_try_start(p, &location_state,
						       &config_state, NULL);
				if (r < 0) {
					fputs(_("Trying next provider...\n"),
					      stderr);
					continue;
				}

				/* Found provider that works. */
				printf(_("Using provider `%s'.\n"), p->name);
				provider = p;
				break;
			}

			/* Failure if no providers were successful at this
			   point. */
			if (provider == NULL) {
				fputs(_("No more location providers"
					" to try.\n"), stderr);
				exit(EXIT_FAILURE);
			}
		}

		/* Get current location. */
		r = provider->get_location(&location_state, &loc);
		if (r < 0) {
		        fputs(_("Unable to get location from provider.\n"),
		              stderr);
		        exit(EXIT_FAILURE);
		}
	
		provider->free(&location_state);
	
		if (verbose) {
			print_location(&loc);

			printf(_("Temperatures: %dK at day, %dK at night\n"),
			       scheme.day.temperature,
			       scheme.night.temperature);

		        /* TRANSLATORS: Append degree symbols if possible. */
			printf(_("Solar elevations: day above %.1f, night below %.1f\n"),
			       scheme.high, scheme.low);
		}

		/* Latitude */
		if (loc.lat < MIN_LAT || loc.lat > MAX_LAT) {
		        /* TRANSLATORS: Append degree symbols if possible. */
		        fprintf(stderr,
		                _("Latitude must be between %.1f and %.1f.\n"),
		                MIN_LAT, MAX_LAT);
		        exit(EXIT_FAILURE);
		}
	
		/* Longitude */
		if (loc.lon < MIN_LON || loc.lon > MAX_LON) {
		        /* TRANSLATORS: Append degree symbols if possible. */
		        fprintf(stderr,
		                _("Longitude must be between"
		                  " %.1f and %.1f.\n"), MIN_LON, MAX_LON);
		        exit(EXIT_FAILURE);
		}

		/* Color temperature */
		if (scheme.day.temperature < MIN_TEMP ||
		    scheme.day.temperature > MAX_TEMP ||
		    scheme.night.temperature < MIN_TEMP ||
		    scheme.night.temperature > MAX_TEMP) {
			fprintf(stderr,
				_("Temperature must be between %uK and %uK.\n"),
				MIN_TEMP, MAX_TEMP);
			exit(EXIT_FAILURE);
		}

		/* Solar elevations */
		if (scheme.high < scheme.low) {
		        fprintf(stderr,
		                _("High transition elevation cannot be lower than"
				  " the low transition elevation.\n"));
		        exit(EXIT_FAILURE);
		}
	}

	if (mode == PROGRAM_MODE_MANUAL) {
		/* Check color temperature to be set */
		if (temp_set < MIN_TEMP || temp_set > MAX_TEMP) {
			fprintf(stderr,
				_("Temperature must be between %uK and %uK.\n"),
				MIN_TEMP, MAX_TEMP);
			exit(EXIT_FAILURE);
		}
	}

	/* Brightness */
	if (scheme.day.brightness < MIN_BRIGHTNESS ||
	    scheme.day.brightness > MAX_BRIGHTNESS ||
	    scheme.night.brightness < MIN_BRIGHTNESS ||
	    scheme.night.brightness > MAX_BRIGHTNESS) {
		fprintf(stderr,
			_("Brightness values must be between %.1f and %.1f.\n"),
			MIN_BRIGHTNESS, MAX_BRIGHTNESS);
		exit(EXIT_FAILURE);
	}

	if (verbose) {
		printf(_("Brightness: %.2f:%.2f\n"),
		       scheme.day.brightness, scheme.night.brightness);
	}

	/* Gamma */
	if (!gamma_is_valid(scheme.day.gamma) ||
	    !gamma_is_valid(scheme.night.gamma)) {
		fprintf(stderr,
			_("Gamma value must be between %.1f and %.1f.\n"),
			MIN_GAMMA, MAX_GAMMA);
		exit(EXIT_FAILURE);
	}

	if (verbose) {
		/* TRANSLATORS: The string in parenthesis is either
		   Daytime or Night (translated). */
		printf(_("Gamma (%s): %.3f, %.3f, %.3f\n"),
		       _("Daytime"), scheme.day.gamma[0],
		       scheme.day.gamma[1], scheme.day.gamma[2]);
		printf(_("Gamma (%s): %.3f, %.3f, %.3f\n"),
		       _("Night"), scheme.night.gamma[0],
		       scheme.night.gamma[1], scheme.night.gamma[2]);
	}

	/* Initialize gamma adjustment method. If method is NULL
	   try all methods until one that works is found. */
	gamma_state_t state;

	/* Gamma adjustment not needed for print mode */
	if (mode != PROGRAM_MODE_PRINT) {
		if (method != NULL) {
			/* Use method specified on command line. */
			r = method_try_start(method, &state, &config_state,
					     method_args);
			if (r < 0) exit(EXIT_FAILURE);
		} else {
			/* Try all methods, use the first that works. */
			for (int i = 0; gamma_methods[i].name != NULL; i++) {
				const gamma_method_t *m = &gamma_methods[i];
				if (!m->autostart) continue;

				r = method_try_start(m, &state, &config_state, NULL);
				if (r < 0) {
					fputs(_("Trying next method...\n"), stderr);
					continue;
				}

				/* Found method that works. */
				printf(_("Using method `%s'.\n"), m->name);
				method = m;
				break;
			}

			/* Failure if no methods were successful at this point. */
			if (method == NULL) {
				fputs(_("No more methods to try.\n"), stderr);
				exit(EXIT_FAILURE);
			}
		}
	}

	config_ini_free(&config_state);

	switch (mode) {
	case PROGRAM_MODE_ONE_SHOT:
	case PROGRAM_MODE_PRINT:
	{
		/* Current angular elevation of the sun */
		double now;
		r = systemtime_get_time(&now);
		if (r < 0) {
			fputs(_("Unable to read system time.\n"), stderr);
			method->free(&state);
			exit(EXIT_FAILURE);
		}

		double elevation = solar_elevation(now, loc.lat, loc.lon);

		if (verbose) {
			/* TRANSLATORS: Append degree symbol if possible. */
			printf(_("Solar elevation: %f\n"), elevation);
		}

		/* Use elevation of sun to set color temperature */
		color_setting_t interp;
		interpolate_color_settings(&scheme, elevation, &interp);

		if (verbose || mode == PROGRAM_MODE_PRINT) {
			period_t period = get_period(&scheme,
						     elevation);
			double transition =
				get_transition_progress(&scheme,
							elevation);
			print_period(period, transition);
			printf(_("Color temperature: %uK\n"),
			       interp.temperature);
			printf(_("Brightness: %.2f\n"),
			       interp.brightness);
		}

		if (mode == PROGRAM_MODE_PRINT) {
			exit(EXIT_SUCCESS);
		}

		/* Adjust temperature */
		r = method->set_temperature(&state, &interp);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			method->free(&state);
			exit(EXIT_FAILURE);
		}

		/* In Quartz (OSX) the gamma adjustments will automatically
		   revert when the process exits. Therefore, we have to loop
		   until CTRL-C is received. */
		if (strcmp(method->name, "quartz") == 0) {
			fputs(_("Press ctrl-c to stop...\n"), stderr);
			pause();
		}
	}
	break;
	case PROGRAM_MODE_MANUAL:
	{
		if (verbose) printf(_("Color temperature: %uK\n"), temp_set);

		/* Adjust temperature */
		color_setting_t manual;
		memcpy(&manual, &scheme.day, sizeof(color_setting_t));
		manual.temperature = temp_set;
		r = method->set_temperature(&state, &manual);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			method->free(&state);
			exit(EXIT_FAILURE);
		}

		/* In Quartz (OSX) the gamma adjustments will automatically
		   revert when the process exits. Therefore, we have to loop
		   until CTRL-C is received. */
		if (strcmp(method->name, "quartz") == 0) {
			fputs(_("Press ctrl-c to stop...\n"), stderr);
			pause();
		}
	}
	break;
	case PROGRAM_MODE_RESET:
	{
		/* Reset screen */
		color_setting_t reset = { NEUTRAL_TEMP, { 1.0, 1.0, 1.0 }, 1.0 };
		r = method->set_temperature(&state, &reset);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			method->free(&state);
			exit(EXIT_FAILURE);
		}

		/* In Quartz (OSX) the gamma adjustments will automatically
		   revert when the process exits. Therefore, we have to loop
		   until CTRL-C is received. */
		if (strcmp(method->name, "quartz") == 0) {
			fputs(_("Press ctrl-c to stop...\n"), stderr);
			pause();
		}
	}
	break;
	case PROGRAM_MODE_CONTINUAL:
	{
		r = run_continual_mode(&loc, &scheme,
				       method, &state,
				       transition, verbose);
		if (r < 0) exit(EXIT_FAILURE);
	}
	break;
	}

	/* Clean up gamma adjustment state */
	method->free(&state);

	return EXIT_SUCCESS;
}
