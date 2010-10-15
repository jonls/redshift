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

   Copyright (c) 2010  Jon Lund Steffensen <jonlst@gmail.com>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <locale.h>
#include <errno.h>

#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "redshift.h"
#include "config-ini.h"
#include "solar.h"
#include "systemtime.h"


#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#define MAX(x,y)  ((x) > (y) ? (x) : (y))


#if !(defined(ENABLE_RANDR) ||			\
      defined(ENABLE_VIDMODE) ||		\
      defined(ENABLE_WINGDI))
# error "At least one of RANDR, VidMode or WinGDI must be enabled."
#endif

#ifdef ENABLE_RANDR
# include "gamma-randr.h"
#endif

#ifdef ENABLE_VIDMODE
# include "gamma-vidmode.h"
#endif

#ifdef ENABLE_WINGDI
# include "gamma-w32gdi.h"
#endif


#include "location-manual.h"

#ifdef ENABLE_GNOME_CLOCK
# include "location-gnome-clock.h"
#endif


/* Union of state data for gamma adjustment methods */
typedef union {
#ifdef ENABLE_RANDR
	randr_state_t randr;
#endif
#ifdef ENABLE_VIDMODE
	vidmode_state_t vidmode;
#endif
#ifdef ENABLE_WINGDI
	w32gdi_state_t w32gdi;
#endif
} gamma_state_t;


/* Gamma adjustment method structs */
static const gamma_method_t gamma_methods[] = {
#ifdef ENABLE_RANDR
	{
		"randr",
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
		"vidmode",
		(gamma_method_init_func *)vidmode_init,
		(gamma_method_start_func *)vidmode_start,
		(gamma_method_free_func *)vidmode_free,
		(gamma_method_print_help_func *)vidmode_print_help,
		(gamma_method_set_option_func *)vidmode_set_option,
		(gamma_method_restore_func *)vidmode_restore,
		(gamma_method_set_temperature_func *)vidmode_set_temperature
	},
#endif
#ifdef ENABLE_WINGDI
	{
		"wingdi",
		(gamma_method_init_func *)w32gdi_init,
		(gamma_method_start_func *)w32gdi_start,
		(gamma_method_free_func *)w32gdi_free,
		(gamma_method_print_help_func *)w32gdi_print_help,
		(gamma_method_set_option_func *)w32gdi_set_option,
		(gamma_method_restore_func *)w32gdi_restore,
		(gamma_method_set_temperature_func *)w32gdi_set_temperature
	},
#endif
	{ NULL }
};


/* Union of state data for location providers */
typedef union {
	location_manual_state_t manual;
#ifdef ENABLE_GNOME_CLOCK
	location_gnome_clock_state_t gnome_clock;
#endif
} location_state_t;


/* Location provider method structs */
static const location_provider_t location_providers[] = {
#ifdef ENABLE_GNOME_CLOCK
	{
		"gnome-clock",
		(location_provider_init_func *)location_gnome_clock_init,
		(location_provider_start_func *)location_gnome_clock_start,
		(location_provider_free_func *)location_gnome_clock_free,
		(location_provider_print_help_func *)
		location_gnome_clock_print_help,
		(location_provider_set_option_func *)
		location_gnome_clock_set_option,
		(location_provider_get_location_func *)
		location_gnome_clock_get_location
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
#define MAX_TEMP  10000
#define MIN_GAMMA   0.1
#define MAX_GAMMA  10.0

/* Default values for parameters. */
#define DEFAULT_DAY_TEMP    5500
#define DEFAULT_NIGHT_TEMP  3700
#define DEFAULT_GAMMA        1.0

/* The color temperature when no adjustment is applied. */
#define NEUTRAL_TEMP  6500

/* Angular elevation of the sun at which the color temperature
   transition period starts and ends (in degress).
   Transition during twilight, and while the sun is lower than
   3.0 degrees above the horizon. */
#define TRANSITION_LOW     SOLAR_CIVIL_TWILIGHT_ELEV
#define TRANSITION_HIGH    3.0

/* Program modes. */
typedef enum {
	PROGRAM_MODE_CONTINUAL,
	PROGRAM_MODE_ONE_SHOT,
	PROGRAM_MODE_RESET
} program_mode_t;


#ifdef HAVE_SYS_SIGNAL_H

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

#else /* ! HAVE_SYS_SIGNAL_H */

static int exiting = 0;
static int disable = 0;

#endif /* ! HAVE_SYS_SIGNAL_H */


/* Calculate color temperature for the specified solar elevation. */
static int
calculate_temp(double elevation, int temp_day, int temp_night,
	       int verbose)
{
	int temp = 0;
	if (elevation < TRANSITION_LOW) {
		temp = temp_night;
		if (verbose) printf(_("Period: Night\n"));
	} else if (elevation < TRANSITION_HIGH) {
		/* Transition period: interpolate */
		float a = (TRANSITION_LOW - elevation) /
			(TRANSITION_LOW - TRANSITION_HIGH);
		temp = (1.0-a)*temp_night + a*temp_day;
		if (verbose) {
			printf(_("Period: Transition (%.2f%% day)\n"), a*100);
		}
	} else {
		temp = temp_day;
		if (verbose) printf(_("Period: Daytime\n"));
	}

	return temp;
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
		"  -v\t\tVerbose output\n"), stdout);
	fputs("\n", stdout);	

	/* TRANSLATORS: help output 4
	   `list' must not be translated
	   no-wrap */
	fputs(_("  -g R:G:B\tAdditional gamma correction to apply\n"
		"  -l LAT:LON\tYour current location\n"
		"  -l PROVIDER\tSelect provider for automatic"
		" location updates\n"
		"  \t\t(Type `list' to see available providers)\n"
		"  -m METHOD\tMethod to use to set color temperature\n"
		"  \t\t(Type `list' to see available methods)\n"
		"  -o\t\tOne shot mode (do not continously adjust"
		" color temperature)\n"
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
	while (args != NULL) {
		char *next_arg = strchr(args, ':');
		if (next_arg != NULL) *(next_arg++) = '\0';

		char *key = NULL;
		char *value = strchr(args, '=');
		if (value != NULL) {
			key = args;
			*(value++) = '\0';
		} else {
			value = args;
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

		char *key = NULL;
		char *value = strchr(args, '=');
		if (value != NULL) {
			key = args;
			*(value++) = '\0';
		} else {
			value = args;
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

	/* Initialize to NULL values. */
	int temp_day = -1;
	int temp_night = -1;
	float gamma[3] = { NAN, NAN, NAN };

	const gamma_method_t *method = NULL;
	char *method_args = NULL;

	const location_provider_t *provider = NULL;
	char *provider_args = NULL;

	int transition = -1;
	program_mode_t mode = PROGRAM_MODE_CONTINUAL;
	int verbose = 0;
	char *s;

	/* Parse arguments. */
	int opt;
	while ((opt = getopt(argc, argv, "g:hl:m:ort:vx")) != -1) {
		switch (opt) {
		case 'g':
			r = parse_gamma_string(optarg, gamma);
			if (r < 0) {
				fputs(_("Malformed gamma argument.\n"),
				      stderr);
				fputs(_("Try `-h' for more"
					" information.\n"), stderr);
				exit(EXIT_FAILURE);
			}
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
			temp_day = atoi(optarg);
			temp_night = atoi(s);
			break;
		case 'v':
			verbose = 1;
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
	r = config_ini_init(&config_state, NULL);
	if (r < 0) {
		fputs("Unable to load config file.\n", stderr);
		exit(EXIT_FAILURE);
	}

	/* Read global config settings. */
	config_ini_section_t *section = config_ini_get_section(&config_state,
							       "redshift");
	if (section != NULL) {
		config_ini_setting_t *setting = section->settings;
		while (setting != NULL) {
			if (strcasecmp(setting->name, "temp-day") == 0) {
				if (temp_day < 0) {
					temp_day = atoi(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "temp-night") == 0) {
				if (temp_night < 0) {
					temp_night = atoi(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "transition") == 0) {
				if (transition < 0) {
					transition = !!atoi(setting->value);
				}
			} else if (strcasecmp(setting->name, "gamma") == 0) {
				if (isnan(gamma[0])) {
					r = parse_gamma_string(setting->value,
							       gamma);
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
	if (temp_day < 0) temp_day = DEFAULT_DAY_TEMP;
	if (temp_night < 0) temp_night = DEFAULT_NIGHT_TEMP;
	if (isnan(gamma[0])) gamma[0] = gamma[1] = gamma[2] = DEFAULT_GAMMA;
	if (transition < 0) transition = 1;

	/* Initialize location provider. If provider is NULL
	   try all providers until one that works is found. */
	location_state_t location_state;

	/* Location is not needed for reset mode. */
	if (mode != PROGRAM_MODE_RESET) {
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
	}

	float lat = NAN;
	float lon = NAN;

	if (mode != PROGRAM_MODE_RESET) {
		/* Get current location. */
		r = provider->get_location(&location_state, &lat, &lon);
		if (r < 0) {
			fputs(_("Unable to get location from provider.\n"),
			      stderr);
			exit(EXIT_FAILURE);
		}

		provider->free(&location_state);

		if (verbose) {
			/* TRANSLATORS: Append degree symbols if possible. */
			printf(_("Location: %f, %f\n"), lat, lon);
		}

		/* Latitude */
		if (lat < MIN_LAT || lat > MAX_LAT) {
			/* TRANSLATORS: Append degree symbols if possible. */
			fprintf(stderr,
				_("Latitude must be between %.1f and %.1f.\n"),
				MIN_LAT, MAX_LAT);
			exit(EXIT_FAILURE);
		}

		/* Longitude */
		if (lon < MIN_LON || lon > MAX_LON) {
			/* TRANSLATORS: Append degree symbols if possible. */
			fprintf(stderr,
				_("Longitude must be between"
				  " %.1f and %.1f.\n"), MIN_LON, MAX_LON);
			exit(EXIT_FAILURE);
		}
	}

	/* Color temperature at daytime */
	if (temp_day < MIN_TEMP || temp_day >= MAX_TEMP) {
		fprintf(stderr,
			_("Temperature must be between %uK and %uK.\n"),
			MIN_TEMP, MAX_TEMP);
		exit(EXIT_FAILURE);
	}

	/* Color temperature at night */
	if (temp_night < MIN_TEMP || temp_night >= MAX_TEMP) {
		fprintf(stderr,
			_("Temperature must be between %uK and %uK.\n"),
			MIN_TEMP, MAX_TEMP);
		exit(EXIT_FAILURE);
	}

	/* Gamma */
	if (gamma[0] < MIN_GAMMA || gamma[0] > MAX_GAMMA ||
	    gamma[1] < MIN_GAMMA || gamma[1] > MAX_GAMMA ||
	    gamma[2] < MIN_GAMMA || gamma[2] > MAX_GAMMA) {
		fprintf(stderr,
			_("Gamma value must be between %.1f and %.1f.\n"),
			MIN_GAMMA, MAX_GAMMA);
		exit(EXIT_FAILURE);
	}

	if (verbose) {
		printf(_("Gamma: %.3f, %.3f, %.3f\n"),
		       gamma[0], gamma[1], gamma[2]);
	}

	/* Initialize gamma adjustment method. If method is NULL
	   try all methods until one that works is found. */
	gamma_state_t state;

	if (method != NULL) {
		/* Use method specified on command line. */
		r = method_try_start(method, &state, &config_state,
				     method_args);
		if (r < 0) exit(EXIT_FAILURE);
	} else {
		/* Try all methods, use the first that works. */
		for (int i = 0; gamma_methods[i].name != NULL; i++) {
			const gamma_method_t *m = &gamma_methods[i];
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

	switch (mode) {
	case PROGRAM_MODE_ONE_SHOT:
	{
		/* Current angular elevation of the sun */
		double now;
		r = systemtime_get_time(&now);
		if (r < 0) {
			fputs(_("Unable to read system time.\n"), stderr);
			method->free(&state);
			exit(EXIT_FAILURE);
		}

		double elevation = solar_elevation(now, lat, lon);

		if (verbose) {
			/* TRANSLATORS: Append degree symbol if possible. */
			printf(_("Solar elevation: %f\n"), elevation);
		}

		/* Use elevation of sun to set color temperature */
		int temp = calculate_temp(elevation, temp_day, temp_night,
					  verbose);

		if (verbose) printf(_("Color temperature: %uK\n"), temp);

		/* Adjust temperature */
		r = method->set_temperature(&state, temp, gamma);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			method->free(&state);
			exit(EXIT_FAILURE);
		}
	}
	break;
	case PROGRAM_MODE_RESET:
	{
		/* Reset screen */
		r = method->set_temperature(&state, NEUTRAL_TEMP, gamma);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			method->free(&state);
			exit(EXIT_FAILURE);
		}
	}
	break;
	case PROGRAM_MODE_CONTINUAL:
	{
		/* Transition state */
		double short_trans_end = 0;
		int short_trans = 0;
		int short_trans_done = 0;

		/* Make an initial transition from 6500K */
		int short_trans_create = 1;
		int short_trans_begin = 1;
		int short_trans_len = 10;

		/* Amount of adjustment to apply. At zero the color
		   temperature will be exactly as calculated, and at one it
		   will be exactly 6500K. */
		float adjustment_alpha = 0.0;

#ifdef HAVE_SYS_SIGNAL_H
		struct sigaction sigact;
		sigset_t sigset;
		sigemptyset(&sigset);

		/* Install signal handler for INT and TERM signals */
		sigact.sa_handler = sigexit;
		sigact.sa_mask = sigset;
		sigact.sa_flags = 0;
		sigaction(SIGINT, &sigact, NULL);
		sigaction(SIGTERM, &sigact, NULL);

		/* Install signal handler for USR1 singal */
		sigact.sa_handler = sigdisable;
		sigact.sa_mask = sigset;
		sigact.sa_flags = 0;
		sigaction(SIGUSR1, &sigact, NULL);
#endif /* HAVE_SYS_SIGNAL_H */

		/* Continously adjust color temperature */
		int done = 0;
		int disabled = 0;
		while (1) {
			/* Check to see if disable signal was caught */
			if (disable) {
				short_trans_create = 1;
				short_trans_len = 2;
				if (!disabled) {
					/* Transition to disabled state */
					short_trans_begin = 0;
					adjustment_alpha = 1.0;
					disabled = 1;
				} else {
					/* Transition back to enabled */
					short_trans_begin = 1;
					adjustment_alpha = 0.0;
					disabled = 0;
				}
				disable = 0;
			}

			/* Check to see if exit signal was caught */
			if (exiting) {
				if (done) {
					/* On second signal stop the
					   ongoing transition */
					short_trans = 0;
				} else {
					if (!disabled) {
						/* Make a short transition
						   back to 6500K */
						short_trans_create = 1;
						short_trans_begin = 0;
						short_trans_len = 2;
						adjustment_alpha = 1.0;
					}

					done = 1;
				}
				exiting = 0;
			}

			/* Read timestamp */
			double now;
			r = systemtime_get_time(&now);
			if (r < 0) {
				fputs(_("Unable to read system time.\n"),
				      stderr);
				method->free(&state);
				exit(EXIT_FAILURE);
			}

			/* Set up a new transition */
			if (short_trans_create) {
				if (transition) {
					short_trans_end = now;
					short_trans_end += short_trans_len;
					short_trans = 1;
					short_trans_create = 0;
				} else {
					short_trans_done = 1;
				}
			}

			/* Current angular elevation of the sun */
			double elevation = solar_elevation(now, lat, lon);

			/* Use elevation of sun to set color temperature */
			int temp = calculate_temp(elevation, temp_day,
						  temp_night, verbose);

			/* Ongoing short transition */
			if (short_trans) {
				double start = now;
				double end = short_trans_end;

				if (start > end) {
					/* Transisiton done */
					short_trans = 0;
					short_trans_done = 1;
				}

				/* Calculate alpha */
				adjustment_alpha = (end - start) /
					(float)short_trans_len;
				if (!short_trans_begin) {
					adjustment_alpha =
						1.0 - adjustment_alpha;
				}

				/* Clamp alpha value */
				adjustment_alpha =
					MAX(0.0, MIN(adjustment_alpha, 1.0));
			}

			/* Handle end of transition */
			if (short_trans_done) {
				if (disabled) {
					/* Restore saved gamma ramps */
					method->restore(&state);
				}
				short_trans_done = 0;
			}

			/* Interpolate between 6500K and calculated
			   temperature */
			temp = adjustment_alpha*6500 +
				(1.0-adjustment_alpha)*temp;

			/* Quit loop when done */
			if (done && !short_trans) break;

			if (verbose) {
				printf(_("Color temperature: %uK\n"), temp);
			}

			/* Adjust temperature */
			if (!disabled || short_trans) {
				r = method->set_temperature(&state,
							    temp, gamma);
				if (r < 0) {
					fputs(_("Temperature adjustment"
						" failed.\n"), stderr);
					method->free(&state);
					exit(EXIT_FAILURE);
				}
			}

			/* Sleep for a while */
#ifndef _WIN32
			if (short_trans) usleep(100000);
			else usleep(5000000);
#else /* ! _WIN32 */
			if (short_trans) Sleep(100);
			else Sleep(5000);
#endif /* ! _WIN32 */
		}

		/* Restore saved gamma ramps */
		method->restore(&state);
	}
	break;
	}

	/* Clean up gamma adjustment state */
	method->free(&state);

	return EXIT_SUCCESS;
}
