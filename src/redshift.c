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

   Copyright (c) 2009-2017  Jon Lund Steffensen <jonlst@gmail.com>
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

/* poll.h is not available on Windows but there is no Windows location provider
   using polling. On Windows, we just define some stubs to make things compile.
   */
#ifndef _WIN32
# include <poll.h>
#else
#define POLLIN 0
struct pollfd {
	int fd;
	short events;
	short revents;
};
int poll(struct pollfd *fds, int nfds, int timeout) { abort(); return -1; }
#endif

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
#include "signals.h"

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

#ifdef ENABLE_GEOCLUE2
# include "location-geoclue2.h"
#endif

#ifdef ENABLE_CORELOCATION
# include "location-corelocation.h"
#endif

#undef CLAMP
#define CLAMP(lo,mid,up)  (((lo) > (mid)) ? (lo) : (((mid) < (up)) ? (mid) : (up)))

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


/* Union of state data for location providers */
typedef union {
	location_manual_state_t manual;
#ifdef ENABLE_GEOCLUE2
	location_geoclue2_state_t geoclue2;
#endif
#ifdef ENABLE_CORELOCATION
	location_corelocation_state_t corelocation;
#endif
} location_state_t;


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
#define DEFAULT_DAY_TEMP    6500
#define DEFAULT_NIGHT_TEMP  4500
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

/* Length of fade in numbers of short sleep durations. */
#define FADE_LENGTH  40

/* Program modes. */
typedef enum {
	PROGRAM_MODE_CONTINUAL,
	PROGRAM_MODE_ONE_SHOT,
	PROGRAM_MODE_PRINT,
	PROGRAM_MODE_RESET,
	PROGRAM_MODE_MANUAL
} program_mode_t;

/* Time range.
   Fields are offsets from midnight in seconds. */
typedef struct {
	int start;
	int end;
} time_range_t;

/* Transition scheme.
   The solar elevations at which the transition begins/ends,
   and the association color settings. */
typedef struct {
	double high;
	double low;
	int use_time; /* When enabled, ignore elevation and use time ranges. */
	time_range_t dawn;
	time_range_t dusk;
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


/* Determine which period we are currently in based on time offset. */
static period_t
get_period_from_time(const transition_scheme_t *transition, int time_offset)
{
	if (time_offset < transition->dawn.start ||
	    time_offset >= transition->dusk.end) {
		return PERIOD_NIGHT;
	} else if (time_offset >= transition->dawn.end &&
		   time_offset < transition->dusk.start) {
		return PERIOD_DAYTIME;
	} else {
		return PERIOD_TRANSITION;
	}
}

/* Determine which period we are currently in based on solar elevation. */
static period_t
get_period_from_elevation(
	const transition_scheme_t *transition, double elevation)
{
	if (elevation < transition->low) {
		return PERIOD_NIGHT;
	} else if (elevation < transition->high) {
		return PERIOD_TRANSITION;
	} else {
		return PERIOD_DAYTIME;
	}
}

/* Determine how far through the transition we are based on time offset. */
static double
get_transition_progress_from_time(
	const transition_scheme_t *transition, int time_offset)
{
	if (time_offset < transition->dawn.start ||
	    time_offset >= transition->dusk.end) {
		return 0.0;
	} else if (time_offset < transition->dawn.end) {
		return (transition->dawn.start - time_offset) /
			(double)(transition->dawn.start -
				transition->dawn.end);
	} else if (time_offset > transition->dusk.start) {
		return (transition->dusk.end - time_offset) /
			(double)(transition->dusk.end -
				transition->dusk.start);
	} else {
		return 1.0;
	}
}

/* Determine how far through the transition we are based on elevation. */
static double
get_transition_progress_from_elevation(
	const transition_scheme_t *transition, double elevation)
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

/* Return number of seconds since midnight from timestamp. */
static int
get_seconds_since_midnight(double timestamp)
{
	time_t t = (time_t)timestamp;
	struct tm tm;
	localtime_r(&t, &tm);
	return tm.tm_sec + tm.tm_min * 60 + tm.tm_hour * 3600;
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

/* Interpolate color setting structs given alpha. */
static void
interpolate_color_settings(
	const color_setting_t *first,
	const color_setting_t *second,
	double alpha,
	color_setting_t *result)
{
	alpha = CLAMP(0.0, alpha, 1.0);

	result->temperature = (1.0-alpha)*first->temperature +
		alpha*second->temperature;
	result->brightness = (1.0-alpha)*first->brightness +
		alpha*second->brightness;
	for (int i = 0; i < 3; i++) {
		result->gamma[i] = (1.0-alpha)*first->gamma[i] +
			alpha*second->gamma[i];
	}
}

/* Interpolate color setting structs transition scheme. */
static void
interpolate_transition_scheme(
	const transition_scheme_t *transition,
	double alpha,
	color_setting_t *result)
{
	const color_setting_t *day = &transition->day;
	const color_setting_t *night = &transition->night;

	alpha = CLAMP(0.0, alpha, 1.0);
	interpolate_color_settings(night, day, alpha, result);
}

/* Return 1 if color settings have major differences, otherwise 0.
   Used to determine if a fade should be applied in continual mode. */
static int
color_setting_diff_is_major(
	const color_setting_t *first,
	const color_setting_t *second)
{
	return (abs(first->temperature - second->temperature) > 25 ||
		fabsf(first->brightness - second->brightness) > 0.1 ||
		fabsf(first->gamma[0] - second->gamma[0]) > 0.1 ||
		fabsf(first->gamma[1] - second->gamma[1]) > 0.1 ||
		fabsf(first->gamma[2] - second->gamma[2]) > 0.1);
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
		"  -r\t\tDisable fading between color temperatures\n"
		"  -t DAY:NIGHT\tColor temperature to set at daytime/night\n"),
	      stdout);
	fputs("\n", stdout);

	/* TRANSLATORS: help output 5 */
	printf(_("The neutral temperature is %uK. Using this value will not change "
		 "the color\ntemperature of the display. Setting the color temperature "
		 "to a value higher\nthan this results in more blue light, and setting "
		 "a lower value will result in\nmore red light.\n"),
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
print_method_list(const gamma_method_t *gamma_methods)
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
print_provider_list(const location_provider_t location_providers[])
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

/* Parse transition time string e.g. "04:50". Returns negative on failure,
   otherwise the parsed time is returned as seconds since midnight. */
static int
parse_transition_time(const char *str, const char **end)
{
	const char *min = NULL;
	errno = 0;
	long hours = strtol(str, (char **)&min, 10);
	if (errno != 0 || min == str || min[0] != ':' ||
	    hours < 0 || hours >= 24) {
		return -1;
	}

	min += 1;
	errno = 0;
	long minutes = strtol(min, (char **)end, 10);
	if (errno != 0 || *end == min || minutes < 0 || minutes >= 60) {
		return -1;
	}

	return minutes * 60 + hours * 3600;
}

/* Parse transition range string e.g. "04:50-6:20". Returns negative on
   failure, otherwise zero. Parsed start and end times are returned as seconds
   since midnight. */
static int
parse_transition_range(const char *str, time_range_t *range)
{
	const char *next = NULL;
	int start_time = parse_transition_time(str, &next);
	if (start_time < 0) return -1;

	int end_time;
	if (next[0] == '\0') {
		end_time = start_time;
	} else if (next[0] == '-') {
		next += 1;
		const char *end = NULL;
		end_time = parse_transition_time(next, &end);
		if (end_time < 0 || end[0] != '\0') return -1;
	} else {
		return -1;
	}

	range->start = start_time;
	range->end = end_time;

	return 0;
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

/* Check whether location is valid.
   Prints error message on stderr and returns 0 if invalid, otherwise
   returns 1. */
static int
location_is_valid(const location_t *location)
{
	/* Latitude */
	if (location->lat < MIN_LAT || location->lat > MAX_LAT) {
		/* TRANSLATORS: Append degree symbols if possible. */
		fprintf(stderr,
			_("Latitude must be between %.1f and %.1f.\n"),
			MIN_LAT, MAX_LAT);
		return 0;
	}

	/* Longitude */
	if (location->lon < MIN_LON || location->lon > MAX_LON) {
		/* TRANSLATORS: Append degree symbols if possible. */
		fprintf(stderr,
			_("Longitude must be between"
			  " %.1f and %.1f.\n"), MIN_LON, MAX_LON);
		return 0;
	}

	return 1;
}

static const gamma_method_t *
find_gamma_method(const gamma_method_t gamma_methods[], const char *name)
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
find_location_provider(
	const location_provider_t location_providers[], const char *name)
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

/* Wait for location to become available from provider.
   Waits until timeout (milliseconds) has elapsed or forever if timeout
   is -1. Writes location to loc. Returns -1 on error,
   0 if timeout was reached, 1 if location became available. */
static int
provider_get_location(
	const location_provider_t *provider, location_state_t *state,
	int timeout, location_t *loc)
{
	int available = 0;
	struct pollfd pollfds[1];
	while (!available) {
		int loc_fd = provider->get_fd(state);
		if (loc_fd >= 0) {
			/* Provider is dynamic. */
			/* TODO: This should use a monotonic time source. */
			double now;
			int r = systemtime_get_time(&now);
			if (r < 0) {
				fputs(_("Unable to read system time.\n"),
				      stderr);
				return -1;
			}

			/* Poll on file descriptor until ready. */
			pollfds[0].fd = loc_fd;
			pollfds[0].events = POLLIN;
			r = poll(pollfds, 1, timeout);
			if (r < 0) {
				perror("poll");
				return -1;
			} else if (r == 0) {
				return 0;
			}

			double later;
			r = systemtime_get_time(&later);
			if (r < 0) {
				fputs(_("Unable to read system time.\n"),
				      stderr);
				return -1;
			}

			/* Adjust timeout by elapsed time */
			if (timeout >= 0) {
				timeout -= (later - now) * 1000;
				timeout = timeout < 0 ? 0 : timeout;
			}
		}


		int r = provider->handle(state, loc, &available);
		if (r < 0) return -1;
	}

	return 1;
}

/* Easing function for fade.
   See https://github.com/mietek/ease-tween */
static double
ease_fade(double t)
{
	if (t <= 0) return 0;
	if (t >= 1) return 1;
	return 1.0042954579734844 * exp(
		-6.4041738958415664 * exp(-7.2908241330981340 * t));
}


/* Run continual mode loop
   This is the main loop of the continual mode which keeps track of the
   current time and continuously updates the screen to the appropriate
   color temperature. */
static int
run_continual_mode(const location_provider_t *provider,
		   location_state_t *location_state,
		   const transition_scheme_t *scheme,
		   const gamma_method_t *method,
		   gamma_state_t *state,
		   int use_fade, int verbose)
{
	int r;

	/* Short fade parameters */
	int fade_length = 0;
	int fade_time = 0;
	color_setting_t fade_start_interp;

	r = signals_install_handlers();
	if (r < 0) {
		return r;
	}

	/* Save previous parameters so we can avoid printing status updates if
	   the values did not change. */
	period_t prev_period = PERIOD_NONE;

	/* Previous target color setting and current actual color setting.
	   Actual color setting takes into account the current color fade. */
	color_setting_t prev_target_interp =
		{ NEUTRAL_TEMP, { 1.0, 1.0, 1.0 }, 1.0 };
	color_setting_t interp =
		{ NEUTRAL_TEMP, { 1.0, 1.0, 1.0 }, 1.0 };

	location_t loc = { NAN, NAN };
	int need_location = !scheme->use_time;
	if (need_location) {
		fputs(_("Waiting for initial location"
			" to become available...\n"), stderr);

		/* Get initial location from provider */
		r = provider_get_location(provider, location_state, -1, &loc);
		if (r < 0) {
			fputs(_("Unable to get location"
				" from provider.\n"), stderr);
			return -1;
		}

		if (!location_is_valid(&loc)) {
			fputs(_("Invalid location returned from provider.\n"),
			      stderr);
			return -1;
		}

		print_location(&loc);
	}

	if (verbose) {
		printf(_("Color temperature: %uK\n"), interp.temperature);
		printf(_("Brightness: %.2f\n"), interp.brightness);
	}

	/* Continuously adjust color temperature */
	int done = 0;
	int prev_disabled = 1;
	int disabled = 0;
	int location_available = 1;
	while (1) {
		/* Check to see if disable signal was caught */
		if (disable && !done) {
			disabled = !disabled;
			disable = 0;
		}

		/* Check to see if exit signal was caught */
		if (exiting) {
			if (done) {
				/* On second signal stop the ongoing fade. */
				break;
			} else {
				done = 1;
				disabled = 1;
			}
			exiting = 0;
		}

		/* Print status change */
		if (verbose && disabled != prev_disabled) {
			printf(_("Status: %s\n"), disabled ?
			       _("Disabled") : _("Enabled"));
		}

		prev_disabled = disabled;

		/* Read timestamp */
		double now;
		r = systemtime_get_time(&now);
		if (r < 0) {
			fputs(_("Unable to read system time.\n"), stderr);
			return -1;
		}

		period_t period;
		double transition_prog;
		if (scheme->use_time) {
			int time_offset = get_seconds_since_midnight(now);

			period = get_period_from_time(scheme, time_offset);
			transition_prog = get_transition_progress_from_time(
				scheme, time_offset);
		} else {
			/* Current angular elevation of the sun */
			double elevation = solar_elevation(
				now, loc.lat, loc.lon);

			period = get_period_from_elevation(scheme, elevation);
			transition_prog =
				get_transition_progress_from_elevation(
					scheme, elevation);
		}

		/* Use transition progress to get target color
		   temperature. */
		color_setting_t target_interp;
		interpolate_transition_scheme(
			scheme, transition_prog, &target_interp);

		if (disabled) {
			/* Reset to neutral */
			target_interp.temperature = NEUTRAL_TEMP;
			target_interp.brightness = 1.0;
			target_interp.gamma[0] = 1.0;
			target_interp.gamma[1] = 1.0;
			target_interp.gamma[2] = 1.0;
		}

		if (done) {
			period = PERIOD_NONE;
		}

		/* Print period if it changed during this update,
		   or if we are in the transition period. In transition we
		   print the progress, so we always print it in
		   that case. */
		if (verbose && (period != prev_period ||
				period == PERIOD_TRANSITION)) {
			print_period(period, transition_prog);
		}

		/* Activate hooks if period changed */
		if (period != prev_period) {
			hooks_signal_period_change(prev_period, period);
		}

		/* Start fade if the parameter differences are too big to apply
		   instantly. */
		if (use_fade) {
			if ((fade_length == 0 &&
			     color_setting_diff_is_major(
				     &interp,
				     &target_interp)) ||
			    (fade_length != 0 &&
			     color_setting_diff_is_major(
				     &target_interp,
				     &prev_target_interp))) {
				fade_length = FADE_LENGTH;
				fade_time = 0;
				fade_start_interp = interp;
			}
		}

		/* Handle ongoing fade */
		if (fade_length != 0) {
			fade_time += 1;
			double frac = fade_time / (double)fade_length;
			double alpha = CLAMP(0.0, ease_fade(frac), 1.0);

			interpolate_color_settings(
				&fade_start_interp, &target_interp, alpha,
				&interp);

			if (fade_time > fade_length) {
				fade_time = 0;
				fade_length = 0;
			}
		} else {
			interp = target_interp;
		}

		/* Break loop when done and final fade is over */
		if (done && fade_length == 0) break;

		if (verbose) {
			if (prev_target_interp.temperature !=
			    target_interp.temperature) {
				printf(_("Color temperature: %uK\n"),
				       target_interp.temperature);
			}
			if (prev_target_interp.brightness !=
			    target_interp.brightness) {
				printf(_("Brightness: %.2f\n"),
				       target_interp.brightness);
			}
		}

		/* Adjust temperature */
		r = method->set_temperature(state, &interp);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"),
			      stderr);
			return -1;
		}

		/* Save period and target color setting as previous */
		prev_period = period;
		prev_target_interp = target_interp;

		/* Sleep length depends on whether a fade is ongoing. */
		int delay = SLEEP_DURATION;
		if (fade_length != 0) {
			delay = SLEEP_DURATION_SHORT;
		}

		/* Update location. */
		int loc_fd = -1;
		if (need_location) {
			loc_fd = provider->get_fd(location_state);
		}

		if (loc_fd >= 0) {
			/* Provider is dynamic. */
			struct pollfd pollfds[1];
			pollfds[0].fd = loc_fd;
			pollfds[0].events = POLLIN;
			int r = poll(pollfds, 1, delay);
			if (r < 0) {
				if (errno == EINTR) continue;
				perror("poll");
				fputs(_("Unable to get location"
					" from provider.\n"), stderr);
				return -1;
			} else if (r == 0) {
				continue;
			}

			/* Get new location and availability
			   information. */
			location_t new_loc;
			int new_available;
			r = provider->handle(
				location_state, &new_loc,
				&new_available);
			if (r < 0) {
				fputs(_("Unable to get location"
					" from provider.\n"), stderr);
				return -1;
			}

			if (!new_available &&
			    new_available != location_available) {
				fputs(_("Location is temporarily"
				        " unavailable; Using previous"
					" location until it becomes"
					" available...\n"), stderr);
			}

			if (new_available &&
			    (new_loc.lat != loc.lat ||
			     new_loc.lon != loc.lon ||
			     new_available != location_available)) {
				loc = new_loc;
				print_location(&loc);
			}

			location_available = new_available;

			if (!location_is_valid(&loc)) {
				fputs(_("Invalid location returned"
					" from provider.\n"), stderr);
				return -1;
			}
		} else {
			systemtime_msleep(delay);
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

	/* Settings for day, night and transition period.
	   Initialized to indicate that the values are not set yet. */
	transition_scheme_t scheme =
		{ TRANSITION_HIGH, TRANSITION_LOW };

	scheme.use_time = 0;
	scheme.dawn.start = -1;
	scheme.dawn.end = -1;
	scheme.dusk.start = -1;
	scheme.dusk.end = -1;

	scheme.day.temperature = -1;
	scheme.day.gamma[0] = NAN;
	scheme.day.brightness = NAN;

	scheme.night.temperature = -1;
	scheme.night.gamma[0] = NAN;
	scheme.night.brightness = NAN;

	/* Temperature for manual mode */
	int temp_set = -1;

	/* List of gamma methods. */
	const gamma_method_t gamma_methods[] = {
#ifdef ENABLE_DRM
		drm_gamma_method,
#endif
#ifdef ENABLE_RANDR
		randr_gamma_method,
#endif
#ifdef ENABLE_VIDMODE
		vidmode_gamma_method,
#endif
#ifdef ENABLE_QUARTZ
		quartz_gamma_method,
#endif
#ifdef ENABLE_WINGDI
		w32gdi_gamma_method,
#endif
		dummy_gamma_method,
		{ NULL }
	};

	const gamma_method_t *method = NULL;
	char *method_args = NULL;

	/* List of location providers. */
	const location_provider_t location_providers[] = {
#ifdef ENABLE_GEOCLUE2
		geoclue2_location_provider,
#endif
#ifdef ENABLE_CORELOCATION
		corelocation_location_provider,
#endif
		manual_location_provider,
		{ NULL }
	};

	const location_provider_t *provider = NULL;
	char *provider_args = NULL;

	int use_fade = -1;
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
			free(config_filepath);
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
				print_provider_list(location_providers);
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
			provider = find_location_provider(
				location_providers, provider_name);
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
				print_method_list(gamma_methods);
				exit(EXIT_SUCCESS);
			}

			/* Split off method arguments. */
			s = strchr(optarg, ':');
			if (s != NULL) {
				*(s++) = '\0';
				method_args = s;
			}

			/* Find adjustment method by name. */
			method = find_gamma_method(gamma_methods, optarg);
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
			use_fade = 0;
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

	free(config_filepath);

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
			} else if (strcasecmp(
					setting->name, "transition") == 0 ||
				   strcasecmp(setting->name, "fade") == 0) {
				/* "fade" is preferred, "transition" is
				   deprecated as the setting key. */
				if (use_fade < 0) {
					use_fade = !!atoi(setting->value);
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
						gamma_methods, setting->value);
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
						location_providers,
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
			} else if (strcasecmp(setting->name,
					      "dawn-time") == 0) {
				if (scheme.dawn.start < 0) {
					int r = parse_transition_range(
						setting->value, &scheme.dawn);
					if (r < 0) {
						fprintf(stderr, _("Malformed"
								  " dawn-time"
								  " setting"
								  " `%s'.\n"),
							setting->value);
						exit(EXIT_FAILURE);
					}
				}
			} else if (strcasecmp(setting->name,
					      "dusk-time") == 0) {
				if (scheme.dusk.start < 0) {
					int r = parse_transition_range(
						setting->value, &scheme.dusk);
					if (r < 0) {
						fprintf(stderr, _("Malformed"
								  " dusk-time"
								  " setting"
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

	if (use_fade < 0) use_fade = 1;

	if (scheme.dawn.start >= 0 || scheme.dawn.end >= 0 ||
	    scheme.dusk.start >= 0 || scheme.dusk.end >= 0) {
		if (scheme.dawn.start < 0 || scheme.dawn.end < 0 ||
		    scheme.dusk.start < 0 || scheme.dusk.end < 0) {
			fputs(_("Partitial time-configuration not"
				" supported!\n"), stderr);
			exit(EXIT_FAILURE);
		}

		if (scheme.dawn.start > scheme.dawn.end ||
		    scheme.dawn.end > scheme.dusk.start ||
		    scheme.dusk.start > scheme.dusk.end) {
			fputs(_("Invalid dawn/dusk time configuration!\n"),
			      stderr);
			exit(EXIT_FAILURE);
		}

		scheme.use_time = 1;
	}

	/* Initialize location provider if needed. If provider is NULL
	   try all providers until one that works is found. */
	location_state_t location_state;

	/* Location is not needed for reset mode and manual mode. */
	int need_location =
		mode != PROGRAM_MODE_RESET &&
		mode != PROGRAM_MODE_MANUAL &&
		!scheme.use_time;
	if (need_location) {
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

		/* Solar elevations */
		if (scheme.high < scheme.low) {
			fprintf(stderr,
				_("High transition elevation cannot be lower than"
				  " the low transition elevation.\n"));
			exit(EXIT_FAILURE);
		}

		if (verbose) {
			/* TRANSLATORS: Append degree symbols if possible. */
			printf(_("Solar elevations: day above %.1f, night below %.1f\n"),
			       scheme.high, scheme.low);
		}
	}

	if (mode != PROGRAM_MODE_RESET &&
	    mode != PROGRAM_MODE_MANUAL) {
		if (verbose) {
			printf(_("Temperatures: %dK at day, %dK at night\n"),
			       scheme.day.temperature,
			       scheme.night.temperature);
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
		location_t loc = { NAN, NAN };
		if (need_location) {
			fputs(_("Waiting for current location"
				" to become available...\n"), stderr);

			/* Wait for location provider. */
			int r = provider_get_location(
				provider, &location_state, -1, &loc);
			if (r < 0) {
				fputs(_("Unable to get location"
					" from provider.\n"), stderr);
				exit(EXIT_FAILURE);
			}

			if (!location_is_valid(&loc)) {
				exit(EXIT_FAILURE);
			}

			print_location(&loc);
		}

		double now;
		r = systemtime_get_time(&now);
		if (r < 0) {
			fputs(_("Unable to read system time.\n"), stderr);
			method->free(&state);
			exit(EXIT_FAILURE);
		}

		period_t period;
		double transition_prog;
		if (scheme.use_time) {
			int time_offset = get_seconds_since_midnight(now);
			period = get_period_from_time(&scheme, time_offset);
			transition_prog = get_transition_progress_from_time(
				&scheme, time_offset);
		} else {
			/* Current angular elevation of the sun */
			double elevation = solar_elevation(
				now, loc.lat, loc.lon);
			if (verbose) {
				/* TRANSLATORS: Append degree symbol if
				   possible. */
				printf(_("Solar elevation: %f\n"), elevation);
			}

			period = get_period_from_elevation(&scheme, elevation);
			transition_prog =
				get_transition_progress_from_elevation(
					&scheme, elevation);
		}

		/* Use transition progress to set color temperature */
		color_setting_t interp;
		interpolate_transition_scheme(
			&scheme, transition_prog, &interp);

		if (verbose || mode == PROGRAM_MODE_PRINT) {
			print_period(period, transition_prog);
			printf(_("Color temperature: %uK\n"),
			       interp.temperature);
			printf(_("Brightness: %.2f\n"),
			       interp.brightness);
		}

		if (mode != PROGRAM_MODE_PRINT) {
			/* Adjust temperature */
			r = method->set_temperature(&state, &interp);
			if (r < 0) {
				fputs(_("Temperature adjustment failed.\n"),
				      stderr);
				method->free(&state);
				exit(EXIT_FAILURE);
			}

			/* In Quartz (macOS) the gamma adjustments will
			   automatically revert when the process exits.
			   Therefore, we have to loop until CTRL-C is received.
			   */
			if (strcmp(method->name, "quartz") == 0) {
				fputs(_("Press ctrl-c to stop...\n"), stderr);
				pause();
			}
		}
	}
	break;
	case PROGRAM_MODE_MANUAL:
	{
		if (verbose) printf(_("Color temperature: %uK\n"), temp_set);

		/* Adjust temperature */
		color_setting_t manual = scheme.day;
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
		r = run_continual_mode(provider, &location_state, &scheme,
				       method, &state,
				       use_fade, verbose);
		if (r < 0) exit(EXIT_FAILURE);
	}
	break;
	}

	/* Clean up gamma adjustment state */
	if (mode != PROGRAM_MODE_PRINT) {
		method->free(&state);
	}

	/* Clean up location provider state */
	if (need_location) {
		provider->free(&location_state);
	}

	return EXIT_SUCCESS;
}
