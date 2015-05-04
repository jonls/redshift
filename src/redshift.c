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
#include "redshift-gamma.h"
#include "redshift-location.h"

#define SET_FLAG(N, F) ((N) |= (F)) 
#define TEST_FLAG(N, F) ((N) & (F))
#define CLR_FLAG(N, F) ((N) &= ~(F))
#define MIN(x,y)        ((x) < (y) ? (x) : (y))
#define MAX(x,y)        ((x) > (y) ? (x) : (y))
#define CLAMP(lo,x,up)  (MAX((lo), MIN((x), (up))))

/* pause() is not defined on windows platform but is not needed either.
   Use a noop macro instead. */
#ifdef __WIN32__
# define pause()
#endif


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
//#define PROGRAM_MODE_CONTINUAL - redundant with ONE_SHOT
#define PROGRAM_MODE_ONE_SHOT 1<<0
#define	PROGRAM_MODE_PRINT 1<<1
#define	PROGRAM_MODE_RESET 1<<2
#define	PROGRAM_MODE_MANUAL 1<<3
#define PROGRAM_PARSE_OPT 1<<4
#define PROGRAM_USE_CONF 1<<5
#define APPLY_GAMMA	1<<6
#define APPLY_TEMPERATURE 1<<7
#define APPLY_BRIGHTNESS 1<<8
#define APPLY_TRANSITION 1<<9
#define APPLY_SCHEME 1<<10

static int verbose = 0;
static int reload = 0;

/* Transition scheme.
   The solar elevations at which the transition begins/ends,
   and the association color settings. */
typedef struct {
	double high;
	double low;
	color_setting_t day;
	color_setting_t night;
} transition_scheme_t;

typedef struct {
	transition_scheme_t *scheme;
	const gamma_method_t *method;
	gamma_state_t *gamma_state;
	const location_provider_t *provider;
	location_t *loc; 
	uint mode;
	int temp_set; 
	int transition;  
} redshift_state_t; 


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

/* Signal handler for reload config signal */
static void
sigreload(int signo) 
{
	reload = 1;
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
		   int transition)
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
	
	/* Install signal handler for USR2 signal */
	sigact.sa_handler = sigreload; 
	sigact.sa_mask = sigset;
	sigact.sa_flags =0;
	
	r = sigaction(SIGUSR2, &sigact, NULL); 
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
	int reload = 0;
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
		
		/* Check whether a signal to reload the config was passed */
		if (reload) {
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

		/* Activate hooks if period changed */
		if (period != prev_period) {
			hooks_signal_period_change(prev_period, period);
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

static int 
redshift_state_set_from_config(config_ini_state_t *config_state, redshift_state_t *state) 
{
	/* Read global config settings. */
	config_ini_section_t *section = config_ini_get_section(config_state, "redshift");
	if (section == NULL) {
		return 0; //Not sure if we should fail here or not?
	}
	config_ini_setting_t *setting = section->settings;
	transition_scheme_t *scheme = state->scheme;
	int r;
	while (setting != NULL) {
		if (strcasecmp(setting->name, "temp-day") == 0) {
			if (scheme->day.temperature < 0) {
				scheme->day.temperature = atoi(setting->value);
			}
		} else if (strcasecmp(setting->name, "temp-night") == 0) {
			if (scheme->night.temperature < 0) {
				scheme->night.temperature =
					atoi(setting->value);
			}
		} else if (strcasecmp(setting->name, "transition") == 0) {
			if (state->transition < 0) {
				state->transition = !!atoi(setting->value);
			}
		} else if (strcasecmp(setting->name,
					  "brightness") == 0) {
			if (isnan(scheme->day.brightness)) {
				scheme->day.brightness =
					atof(setting->value);
			}
			if (isnan(scheme->night.brightness)) {
				scheme->night.brightness =
					atof(setting->value);
			}
		} else if (strcasecmp(setting->name,
					  "brightness-day") == 0) {
			if (isnan(scheme->day.brightness)) {
				scheme->day.brightness =
					atof(setting->value);
			}
		} else if (strcasecmp(setting->name,
					  "brightness-night") == 0) {
			if (isnan(scheme->night.brightness)) {
				scheme->night.brightness =
					atof(setting->value);
			}
		} else if (strcasecmp(setting->name,
					  "elevation-high") == 0) {
			scheme->high = atof(setting->value);
		} else if (strcasecmp(setting->name,
					  "elevation-low") == 0) {
			scheme->low = atof(setting->value);
		} else if (strcasecmp(setting->name, "gamma") == 0) {
			if (isnan(scheme->day.gamma[0])) {
				r = parse_gamma_string(setting->value,
							   scheme->day.gamma);
				if (r < 0) {
					fputs(_("Malformed gamma"
						" setting.\n"),
						  stderr);
					return -1;
				}
				memcpy(scheme->night.gamma, scheme->day.gamma,
					   sizeof(scheme->night.gamma));
			}
		} else if (strcasecmp(setting->name, "gamma-day") == 0) {
			if (isnan(scheme->day.gamma[0])) {
				r = parse_gamma_string(setting->value,
							   scheme->day.gamma);
				if (r < 0) {
					fputs(_("Malformed gamma"
						" setting.\n"),
						  stderr);
					return -1;
				}
			}
		} else if (strcasecmp(setting->name, "gamma-night") == 0) {
			if (isnan(scheme->night.gamma[0])) {
				r = parse_gamma_string(setting->value,
							   scheme->night.gamma);
				if (r < 0) {
					fputs(_("Malformed gamma"
						" setting.\n"),
						  stderr);
					return -1;
				}
			}
		} else if (strcasecmp(setting->name, "adjustment-method") == 0) {
			if (state->method == NULL) {
				state->method = find_gamma_method(setting->value);
				if (state->method == NULL) {
					fprintf(stderr, _("Unknown"
							  " adjustment"
							  " method"
							  " `%s'.\n"),
						setting->value);
					return -1;
				}
			}
		} else if (strcasecmp(setting->name, "location-provider") == 0) {
			if (state->provider == NULL) {
				state->provider = find_location_provider(
					setting->value);
				if (state->provider == NULL) {
					fprintf(stderr, _("Unknown"
							  " location"
							  " provider"
							  " `%s'.\n"),
						setting->value);
					return -1;
				}
			}
		} else {
			fprintf(stderr, _("Unknown configuration"
					  " setting `%s'.\n"),
				setting->name);
		}
		setting = setting->next;
	}
	return 0;
}

static int
redshift_gamma_state_init(redshift_state_t *state, config_ini_state_t *config_state, char *method_args) 
{
	/* Initialize gamma adjustment method. If method is NULL
	   try all methods until one that works is found. */
	int r;
	if (state->method != NULL) {
		/* Use method specified on command line. */
		r = method_try_start(state->method, state->gamma_state, config_state,
					 method_args);
		if (r < 0) {
			return -1;
		}
		return 0; 
	}
	/* Try all methods, use the first that works. */
	for (int i = 0; gamma_methods[i].name != NULL; i++) {
		const gamma_method_t *m = &gamma_methods[i];
		if (!m->autostart) continue;
		r = method_try_start(m, state->gamma_state, config_state, NULL);
		//r=-1;
		if (r < 0) {
			fputs(_("Trying next method...\n"), stderr);
			continue;
		}

		/* Found method that works. */
		printf(_("Using method `%s'.\n"), m->name);
		state->method = m;
		return 0;
	}
	/* Failure if no methods were successful at this point. */
	if (state->method == NULL) {
		fputs(_("No more methods to try.\n"), stderr);
		return -1;
	}
}

static int 
location_provider_setup(config_ini_state_t *config_state, transition_scheme_t *scheme, const location_provider_t *provider, location_t *loc, char *provider_args)
{
	int r;

	/* Initialize location provider. If provider is NULL
	   try all providers until one that works is found. */
	location_state_t location_state;
	
	if (provider != NULL) {
		/* Use provider specified on command line. */
		r = provider_try_start(provider, &location_state,
					   config_state, provider_args);
		if (r < 0) return -1;
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
						   config_state, NULL);
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
			return -1;
		}
	}

	/* Get current location. */
	r = provider->get_location(&location_state, loc);
	if (r < 0) {
			fputs(_("Unable to get location from provider.\n"),
				  stderr);
			return -1;
	}

	provider->free(&location_state);

	if (verbose) {
		print_location(loc);

		printf(_("Temperatures: %dK at day, %dK at night\n"),
			   scheme->day.temperature,
			   scheme->night.temperature);

			/* TRANSLATORS: Append degree symbols if possible. */
		printf(_("Solar elevations: day above %.1f, night below %.1f\n"),
			   scheme->high, scheme->low);
	}

	/* Latitude */
	if (loc->lat < MIN_LAT || loc->lat > MAX_LAT) {
			/* TRANSLATORS: Append degree symbols if possible. */
			fprintf(stderr,
					_("Latitude must be between %.1f and %.1f.\n"),
					MIN_LAT, MAX_LAT);
			return -1;
	}

	/* Longitude */
	if (loc->lon < MIN_LON || loc->lon > MAX_LON) {
			/* TRANSLATORS: Append degree symbols if possible. */
			fprintf(stderr,
					_("Longitude must be between"
					  " %.1f and %.1f.\n"), MIN_LON, MAX_LON);
			return -1;
	}
}



int 
transition_scheme_validate(transition_scheme_t *scheme) 
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

void gamma_state_finalize(gamma_state_t *state) 
{
	if (state != NULL) {
		free(state);
	}
}

static void redshift_state_init(redshift_state_t *redshift_state) 
{
	/* const gamma_method_t */
	redshift_state->method = NULL; 
	/* const location_provider_t */
	redshift_state->provider = NULL; 
	/* transition_scheme_t */
	redshift_state->scheme = transition_scheme_new(); 
	/* const gamme_state_t */
	redshift_state->gamma_state = malloc(sizeof(gamma_state_t));
	/* init mode ( PROGRAM_MODE_CONTINUAL ) */
	redshift_state->mode = 0;
	redshift_state->transition = -1;
	redshift_state->temp_set = -1;
	redshift_state->loc = malloc(sizeof(location_t));
	redshift_state->loc->lat = NAN;
	redshift_state->loc->lon = NAN;
}

static void redshift_state_finalize(redshift_state_t *redshift_state) {
	gamma_state_finalize(redshift_state->gamma_state);
	transition_scheme_finalize(redshift_state->scheme);
	if (redshift_state->loc != NULL) {
		free(redshift_state->loc);
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

static int 
redshift_parse_opt(int argc, char *argv[], redshift_state_t *state, char *method_args, char *provider_args, char *config_filepath)
{
	transition_scheme_t *scheme= state->scheme;
	char *s;
	/* Parse command line arguments. */
	int opt, r;
	while ((opt = getopt(argc, argv, "b:c:g:hl:m:oO:prt:vVx")) != -1) {
		switch (opt) {
		case 'b':
			parse_brightness_string(optarg,
						&scheme->day.brightness,
						&scheme->night.brightness);
			SET_FLAG(state->mode, APPLY_BRIGHTNESS);
			break;
		case 'c':
			if (config_filepath != NULL) free(config_filepath);
			config_filepath = strdup(optarg);
			break;
		case 'g':
			r = parse_gamma_string(optarg, scheme->day.gamma);
			if (r < 0) {
				fputs(_("Malformed gamma argument.\n"),
					  stderr);
				fputs(_("Try `-h' for more"
					" information.\n"), stderr);
				return -1;
			}
			
			SET_FLAG(state->mode, APPLY_GAMMA);
			/* Set night gamma to the same value as day gamma.
			   To set these to distinct values use the config
			   file. */
			memcpy(scheme->night.gamma, scheme->day.gamma,
				   sizeof(scheme->night.gamma));
			break;
		case 'h':
			print_help(argv[0]);
			return 0;
			break;
		case 'l':
			/* Print list of providers if argument is `list' */
			if (strcasecmp(optarg, "list") == 0) {
				print_provider_list();
				return 0;
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
				SET_FLAG(state->mode, PROGRAM_MODE_MANUAL); 
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
			state->provider = find_location_provider(provider_name);
			if (state->provider == NULL) {
				fprintf(stderr, _("Unknown location provider"
						  " `%s'.\n"), provider_name);
				return -1;
			}

			/* Print provider help if arg is `help'. */
			if (provider_args != NULL &&
				strcasecmp(provider_args, "help") == 0) {
				state->provider->print_help(stdout);
				return 0;
			}
			break;
		case 'm':
			/* Print list of methods if argument is `list' */
			if (strcasecmp(optarg, "list") == 0) {
				print_method_list();
				return 0;
			}

			/* Split off method arguments. */
			s = strchr(optarg, ':');
			if (s != NULL) {
				*(s++) = '\0';
				method_args = s;
			}

			/* Find adjustment method by name. */
			state->method = find_gamma_method(optarg);
			if (state->method == NULL) {
				/* TRANSLATORS: This refers to the method
				   used to adjust colors e.g VidMode */
				fprintf(stderr, _("Unknown adjustment method"
						  " `%s'.\n"), optarg);
				return -1;
			}

			/* Print method help if arg is `help'. */
			if (method_args != NULL &&
				strcasecmp(method_args, "help") == 0) {
				state->method->print_help(stdout);
				return 0;
			}
			break;
		case 'o':
			SET_FLAG(state->mode, PROGRAM_MODE_ONE_SHOT);
			break;
		case 'O':
			SET_FLAG(state->mode, PROGRAM_MODE_MANUAL);
			SET_FLAG(state->mode, PROGRAM_MODE_ONE_SHOT);
			SET_FLAG(state->mode, APPLY_TEMPERATURE);
			//CLR_FLAG(state->mode, PROGRAM_USE_CONF);
			state->temp_set = atoi(optarg);
			break;
		case 'p':
			SET_FLAG(state->mode, PROGRAM_MODE_PRINT);
			SET_FLAG(state->mode, PROGRAM_MODE_ONE_SHOT);
			break;
		case 'r':
			state->transition = 0;
			SET_FLAG(state->mode, APPLY_TRANSITION);
			break;
		case 't':
			s = strchr(optarg, ':');
			if (s == NULL) {
				fputs(_("Malformed temperature argument.\n"),
					  stderr);
				fputs(_("Try `-h' for more information.\n"),
					  stderr);
				return -1;
			}
			*(s++) = '\0';
			scheme->day.temperature = atoi(optarg);
			scheme->night.temperature = atoi(s);
			SET_FLAG(state->mode, APPLY_SCHEME);
			break;
		case 'v':
			verbose = 1;
			break;
				case 'V':
						printf("%s\n", PACKAGE_STRING);
						return 0;
						break;
		case 'x':
			SET_FLAG(state->mode, PROGRAM_MODE_RESET);
			SET_FLAG(state->mode, APPLY_TEMPERATURE);
			SET_FLAG(state->mode, APPLY_GAMMA); //For now it's default
			break;
		case '?':
			fputs(_("Try `-h' for more information.\n"), stderr);
			return -1;
			break;
		}
	}
	return 1;
}

static int
redshift_state_setup(redshift_state_t *state, int argc, char *argv[]) {
	transition_scheme_t *scheme= state->scheme;
	config_ini_state_t config_state; 
	char *method_args = NULL;
	char *provider_args = NULL;
	char *config_filepath = NULL;
	int r; 
	
	/* Parse Options */ 
	if (TEST_FLAG(state->mode, PROGRAM_PARSE_OPT)) {
		r = redshift_parse_opt(argc, argv, state, method_args, provider_args, config_filepath);
		/* Check for exit_failure or exit_success */
		if (r == 0) {
			return r;
		}
		else if (r == -1) {
			return r;
		}
	}
	
	/* Setup Config Stuff */ 
	//Currently not running config_init breaks somethings since it sets defaults as well
	CLR_FLAG(state->mode, PROGRAM_USE_CONF);
	if (TEST_FLAG(state->mode, PROGRAM_USE_CONF)) { 
		r = config_ini_init(&config_state, config_filepath);
		if (r < 0) {
			fputs("Unable to load config file.\n", stderr);
			return r;
		}
		if (config_filepath != NULL) {
			free(config_filepath); 
		}
		r = redshift_state_set_from_config(&config_state, state);
		if (r < 0) {
			return r;
		}
	}
	else {
		/* Phony init */
		config_ini_init(&config_state, NULL);
	}
	
	/* Set unset values to default in scheme */
	transition_scheme_set_default(state->scheme);

	/* Transition set default -- should get moved */
	if (state->transition < 0) {
		state->transition = 1;
	}
	
	/* Location provider is not needed for reset mode and manual mode. */
	if (!TEST_FLAG(state->mode, PROGRAM_MODE_MANUAL) && !TEST_FLAG(state->mode, PROGRAM_MODE_RESET)) {
		r = location_provider_setup(&config_state, state->scheme, state->provider, state->loc, provider_args);
		printf("location_provider_setup r: %i \n", r);
		if (r < 0) {
			return r;
		}
	}
	
	/* Validate Scheme has been set correctly */ 
	r= transition_scheme_validate(state->scheme);
	if (r < 0) {
		return r;
	}
	
	/* If temp is not being set, we don't check it */
	if (TEST_FLAG(state->mode, APPLY_TEMPERATURE)) {
		if (state->temp_set < MIN_TEMP || state->temp_set > MAX_TEMP) {
			fprintf(stderr,
				_("Temperature must be between %uK and %uK.\n"),
				MIN_TEMP, MAX_TEMP);
			return -1;
		}
	}
	
	/* Init gamma settings */
	r = redshift_gamma_state_init(state, &config_state, method_args);
	if (r < 0) {
		return r;
	}
	
	return 1;
}

static int
redshift_state_apply(redshift_state_t *state)
{
	int r;
	transition_scheme_t *scheme= state->scheme;
	/* It's silly but to reset we actually need to parse config and get the right gamma method */
	/* The ideal thing might be to split LOADING into section based methods and add an APPLY set of flags instead of the MODE enum
	 * Then we can down through a switch and load only the things neccesary according to the APPLY flags set by command-line arguments
	 * */
	if (TEST_FLAG(state->mode, PROGRAM_MODE_RESET)) {
		color_setting_t reset = { NEUTRAL_TEMP, { 1.0, 1.0, 1.0 }, 1.0 };
		r = state->method->set_temperature(state->gamma_state, &reset);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			return r;
		}
		return 0;
	}
	else if (TEST_FLAG(state->mode, PROGRAM_MODE_MANUAL) && TEST_FLAG(state->mode, PROGRAM_MODE_ONE_SHOT)) {
		if (verbose) printf(_("Color temperature: %uK\n"), state->temp_set);

		/* Adjust temperature */
		color_setting_t manual;
		memcpy(&manual, &scheme->day, sizeof(color_setting_t));
		manual.temperature = state->temp_set;
		r = state->method->set_temperature(state->gamma_state, &manual);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			return -1;
		}
	}
	else if (TEST_FLAG(state->mode, PROGRAM_MODE_ONE_SHOT)) {
		/* Current angular elevation of the sun */
		double now;
		r = systemtime_get_time(&now);
		if (r < 0) {
			fputs(_("Unable to read system time.\n"), stderr);
			redshift_state_finalize(state);
			return r;
		}

		double elevation = solar_elevation(now, state->loc->lat, state->loc->lon);

		if (verbose) {
			/* TRANSLATORS: Append degree symbol if possible. */
			printf(_("Solar elevation: %f\n"), elevation);
		}
		
		/* Use elevation of sun to set color temperature */
		color_setting_t interp;
		interpolate_color_settings(scheme, elevation, &interp);
		
		//Split these into different print functions later
		if (TEST_FLAG(state->mode, PROGRAM_MODE_PRINT) || verbose) {
			period_t period = get_period(scheme, elevation);
			double transition = get_transition_progress(scheme, elevation);
			
			print_period(period, transition);
			printf(_("Color temperature: %uK\n"), interp.temperature);
			printf(_("Brightness: %.2f\n"), interp.brightness);
			//If printing we are done here
			if TEST_FLAG(state->mode, PROGRAM_MODE_PRINT) return 0;
		}
		
		/* Adjust temperature */
		r = state->method->set_temperature(state->gamma_state, &interp);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			return -1;
		}
	}
	else {
		return 1;
	}
	
	/* In Quartz (OSX) the gamma adjustments will automatically
	revert when the process exits. Therefore, we have to loop
	until CTRL-C is received. */
	if (strcmp(state->method->name, "quartz") == 0) {
		fputs(_("Press ctrl-c to stop...\n"), stderr);
		pause();
	}
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

	/* Flush messages consistently even if redirected to a pipe or
	   file.  Change the flush behaviour to line-buffered, without
	   changing the actual buffers being used. */
	setvbuf(stdout, NULL, _IOLBF, 0);
	setvbuf(stderr, NULL, _IOLBF, 0);

	redshift_state_t state;
	redshift_state_init(&state);
	SET_FLAG(state.mode, PROGRAM_PARSE_OPT);
	SET_FLAG(state.mode, PROGRAM_USE_CONF);
	//SET_FLAG(state.mode, PROGRAM_MODE_CONTINUAL);
	
	/* Setup the state */
	r = redshift_state_setup(&state, argc, argv);
	printf("redshift_state_setup r: %i \n", r);
	if (r == -1) {
		redshift_state_finalize(&state);
		exit(EXIT_FAILURE);
	}
	else if (r == 0) {
		redshift_state_finalize(&state);
		exit(EXIT_SUCCESS);
	}
	
	/* Apply the state */
	r = redshift_state_apply(&state); 
	printf("redshift_state_apply r: %i \n", r);
	if (r == -1) {
		redshift_state_finalize(&state);
		exit(EXIT_FAILURE);
	}
	else if (r == 0) {
		redshift_state_finalize(&state);
		exit(EXIT_SUCCESS);
	}

	/* Run Daemon */
	if (!TEST_FLAG(state.mode, PROGRAM_MODE_ONE_SHOT)) {
		r = run_continual_mode(state.loc, state.scheme, state.method, state.gamma_state, state.transition);
		if (r < 0) {
			redshift_state_finalize(&state);
			exit(EXIT_FAILURE);
		}
	}

	/* Clean up redshift state */
	redshift_state_finalize(&state);

	return EXIT_SUCCESS;
}
