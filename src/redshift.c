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

   Copyright (c) 2013  Jon Lund Steffensen <jonlst@gmail.com>
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

#if defined(HAVE_SIGNAL_H) && !defined(__WIN32__)
# include <signal.h>
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "redshift.h"
#include "settings.h"
#include "config-ini.h"
#include "solar.h"
#include "systemtime.h"


#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#define MAX(x,y)  ((x) > (y) ? (x) : (y))


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

#ifdef ENABLE_WINGDI
# include "gamma-w32gdi.h"
#endif


#include "location-manual.h"

#ifdef ENABLE_GEOCLUE
# include "location-geoclue.h"
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

/* The color temperature when no adjustment is applied. */
#define NEUTRAL_TEMP  6500

/* Program modes. */
typedef enum {
	PROGRAM_MODE_CONTINUAL,
	PROGRAM_MODE_ONE_SHOT,
	PROGRAM_MODE_PRINT,
	PROGRAM_MODE_RESET,
	PROGRAM_MODE_MANUAL
} program_mode_t;


#if defined(HAVE_SIGNAL_H) && !defined(__WIN32__)

static volatile sig_atomic_t exiting = 0;
static volatile sig_atomic_t disable = 0;
static volatile sig_atomic_t reload = 0;

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

/* Signal handler for reload signal */
static void
sigreload(int signo)
{
	reload = 1;
}

#else /* ! HAVE_SIGNAL_H || __WIN32__ */

static int exiting = 0;
static int disable = 0;
static int reload = 0;

#endif /* ! HAVE_SIGNAL_H || __WIN32__ */

static settings_t settings;


/* Print which period (night, day or transition) we're currently in. */
static void
print_period(double elevation)
{
	if (elevation < settings.transition_low) {
		printf(_("Period: Night\n"));
	} else if (elevation < settings.transition_high) {
		float a = (settings.transition_low - elevation) /
			(settings.transition_low - settings.transition_high);
		printf(_("Period: Transition (%.2f%% day)\n"), a*100);
	} else {
		printf(_("Period: Daytime\n"));
	}
}

/* Interpolate value based on the specified solar elevation. */
static float
calculate_interpolated_value(double elevation, float day, float night)
{
	float result;
	if (elevation < settings.transition_low) {
		result = night;
	} else if (elevation < settings.transition_high) {
		/* Transition period: interpolate */
		float a = (settings.transition_low - elevation) /
			(settings.transition_low - settings.transition_high);
		result = (1.0-a)*night + a*day;
	} else {
		result = day;
	}

	return result;
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

	/* Initialize settings to NULL values. */
	char *config_filepath = NULL;

	settings_t settings_cmdline;
	settings_init(&settings);

	const gamma_method_t *method = NULL;
	char *method_args = NULL;

	const location_provider_t *provider = NULL;
	char *provider_args = NULL;

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
			parse_brightness_string(optarg, &(settings.brightness_day),
						&(settings.brightness_night));
			break;
		case 'c':
			if (config_filepath != NULL) free(config_filepath);
			config_filepath = strdup(optarg);
			break;
		case 'g':
			r = parse_gamma_string(optarg, settings.gamma);
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
		case 'O':
			mode = PROGRAM_MODE_MANUAL;
			settings.temp_set = atoi(optarg);
			break;
		case 'p':
			mode = PROGRAM_MODE_PRINT;
			break;
		case 'r':
			settings.transition = 0;
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
			settings.temp_day = atoi(optarg);
			settings.temp_night = atoi(s);
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

	settings_copy(&settings_cmdline, &settings);

	/* Load settings from config file. */
	config_ini_state_t config_state;
	r = config_ini_init(&config_state, config_filepath);
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
			r = settings_parse(&settings, setting->name, setting->value);
			if (r == 0)
				;
			else if (r < 0)
				exit(EXIT_FAILURE);
			else if (strcasecmp(setting->name,
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
	settings_finalize(&settings);

	float lat = NAN;
	float lon = NAN;

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
			printf(_("Temperatures: %dK at day, %dK at night\n"),
			       settings.temp_day, settings.temp_night);
		        /* TRANSLATORS: Append degree symbols if possible. */
			printf(_("Solar elevations: day above %.1f, night below %.1f\n"),
			       settings.transition_high, settings.transition_low);
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
	    
	r = settings_validate(&settings, mode == PROGRAM_MODE_MANUAL, mode == PROGRAM_MODE_RESET);
	if (r < 0)
		exit(EXIT_FAILURE);

	if (verbose) {
		printf(_("Brightness: %.2f:%.2f\n"),
		       settings.brightness_day, settings.brightness_night);
		printf(_("Gamma: %.3f, %.3f, %.3f\n"),
		       settings.gamma[0], settings.gamma[1], settings.gamma[2]);
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

		double elevation = solar_elevation(now, lat, lon);

		if (verbose) {
			/* TRANSLATORS: Append degree symbol if possible. */
			printf(_("Solar elevation: %f\n"), elevation);
		}

		/* Use elevation of sun to set color temperature */
		int temp = (int)calculate_interpolated_value(elevation,
							     settings.temp_day, settings.temp_night);
		float brightness = calculate_interpolated_value(elevation,
								settings.brightness_day,
								settings.brightness_night);

		if (verbose || mode == PROGRAM_MODE_PRINT) {
			print_period(elevation);
			printf(_("Color temperature: %uK\n"), temp);
			printf(_("Brightness: %.2f\n"), brightness);
		}

		if (mode == PROGRAM_MODE_PRINT) {
			exit(EXIT_SUCCESS);
		}

		/* Adjust temperature */
		r = method->set_temperature(&state, temp, brightness, settings.gamma);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			method->free(&state);
			exit(EXIT_FAILURE);
		}
	}
	break;
	case PROGRAM_MODE_MANUAL:
	{
		if (verbose) printf(_("Color temperature: %uK\n"), settings.temp_set);

		/* Adjust temperature */
		r = method->set_temperature(&state, settings.temp_set, settings.brightness_day, settings.gamma);
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
		r = method->set_temperature(&state, NEUTRAL_TEMP, 1.0, settings.gamma);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			method->free(&state);
			exit(EXIT_FAILURE);
		}
	}
	break;
	case PROGRAM_MODE_CONTINUAL:
	{
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
		sigaction(SIGINT, &sigact, NULL);
		sigaction(SIGTERM, &sigact, NULL);

		/* Install signal handler for USR1 singal */
		sigact.sa_handler = sigdisable;
		sigact.sa_mask = sigset;
		sigact.sa_flags = 0;
		sigaction(SIGUSR1, &sigact, NULL);

		/* Install signal handler for USR2 singal */
		sigact.sa_handler = sigreload;
		sigact.sa_mask = sigset;
		sigact.sa_flags = 0;
		sigaction(SIGUSR2, &sigact, NULL);
#endif /* HAVE_SIGNAL_H && ! __WIN32__ */

		if (verbose) {
			printf("Status: %s\n", "Enabled");
		}

		/* Continuously adjust color temperature */
		int done = 0;
		int disabled = 0;
		settings_t old_settings;
		settings_t new_settings;
		double reload_trans_delta = 0.2;
		double reload_trans = 0;
		int reloading = 0;
		
		while (1) {
			/* Reload settings if reload signal was caught */
			if (reload) {
				reload = 0;
				settings_copy(&new_settings, &settings_cmdline);

				/* Load settings from config file. */
				config_ini_state_t config_state;
				r = config_ini_init(&config_state, config_filepath);
				if (r < 0) {
					fputs("Unable to load config file.\n", stderr);
					goto reload_failed;
				}

				/* Read global config settings. */
				config_ini_section_t *section =
					config_ini_get_section(&config_state, "redshift");
				if (section != NULL) {
					config_ini_setting_t *setting = section->settings;
					while (setting != NULL) {
						r = settings_parse(&new_settings, setting->name, setting->value);
						if (r < 0) {
							config_ini_free(&config_state);
							goto reload_failed;
						}
						setting = setting->next;
					}
				}

				config_ini_free(&config_state);
				settings_finalize(&new_settings);
				r = settings_validate(&new_settings, 0, 0);
				if (r < 0) goto reload_failed;
				
				if (new_settings.reload_transition) {
					settings_copy(&old_settings, &settings);
					reloading = 1;
					reload_trans = 0;
				} else {
					settings_copy(&settings, &new_settings);
				}
				
				if (verbose) {
				        /* TRANSLATORS: Append degree symbols if possible. */
				        printf(_("Location: %f, %f\n"), lat, lon);
					printf(_("Temperatures: %dK at day, %dK at night\n"),
					       settings.temp_day, settings.temp_night);
				        /* TRANSLATORS: Append degree symbols if possible. */
					printf(_("Solar elevations: day above %.1f, night below %.1f\n"),
					       settings.transition_high, settings.transition_low);
					printf(_("Brightness: %.2f:%.2f\n"),
					       settings.brightness_day, settings.brightness_night);
					printf(_("Gamma: %.3f, %.3f, %.3f\n"),
					       settings.gamma[0], settings.gamma[1], settings.gamma[2]);
				}
			}
		reload_failed:

			/* Perform reload transition */
			if (reloading) {
				reload_trans += reload_trans_delta;
				if (reload_trans >= 1.0)
					reloading = 0;
				settings_interpolate(&settings, old_settings, new_settings, reload_trans);
			}

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
					printf("Status: %s\n", disabled ?
					       "Disabled" : "Enabled");
				}
			}

			/* Check to see if exit signal was caught */
			if (exiting) {
				if (done) {
					/* On second signal stop the
					   ongoing transition */
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
				fputs(_("Unable to read system time.\n"),
				      stderr);
				method->free(&state);
				exit(EXIT_FAILURE);
			}

			/* Skip over transition if transitions are disabled */
			int set_adjustments = 0;
			if (!settings.transition) {
				if (short_trans_delta) {
					adjustment_alpha = short_trans_delta < 0 ? 0.0 : 1.0;
					short_trans_delta = 0;
					set_adjustments = 1;
				}
			}

			/* Current angular elevation of the sun */
			double elevation = solar_elevation(now, lat, lon);

			/* Use elevation of sun to set color temperature */
			int temp = (int)calculate_interpolated_value(elevation,
								settings.temp_day, settings.temp_night);
			float brightness = calculate_interpolated_value(elevation,
									settings.brightness_day,
									settings.brightness_night);

			if (verbose) print_period(elevation);

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
			temp = adjustment_alpha*6500 +
				(1.0-adjustment_alpha)*temp;

			brightness = adjustment_alpha*1.0 +
				(1.0-adjustment_alpha)*brightness;

			/* Quit loop when done */
			if (done && !short_trans_delta) break;

			if (verbose) {
				printf(_("Color temperature: %uK\n"), temp);
				printf(_("Brightness: %.2f\n"), brightness);
			}

			/* Adjust temperature */
			if (!disabled || short_trans_delta || set_adjustments) {
				r = method->set_temperature(&state,
							    temp, brightness,
							    settings.gamma);
				if (r < 0) {
					fputs(_("Temperature adjustment"
						" failed.\n"), stderr);
					method->free(&state);
					exit(EXIT_FAILURE);
				}
			}

			/* Sleep for 5 seconds or 0.1 second. */
#ifndef _WIN32
			if (short_trans_delta || reloading) usleep(100000);
			else usleep(5000000);
#else /* ! _WIN32 */
			if (short_trans_delta || reloading) Sleep(100);
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
	
	if (config_filepath != NULL) free(config_filepath);
	return EXIT_SUCCESS;
}
