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
   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <alloca.h>
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
#include "config-ini.h"
#include "solar.h"
#include "systemtime.h"
#include "adjustments.h"
#include "opt-parser.h"
#include "gamma-common.h"


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



/* Gamma adjustment method structs */
static const gamma_method_t gamma_methods[] = {
#ifdef ENABLE_DRM
	{
		"drm", 0,
		(gamma_method_init_func *)drm_init,
		(gamma_method_start_func *)drm_start,
		(gamma_method_print_help_func *)drm_print_help
	},
#endif
#ifdef ENABLE_RANDR
	{
		"randr", 1,
		(gamma_method_init_func *)randr_init,
		(gamma_method_start_func *)randr_start,
		(gamma_method_print_help_func *)randr_print_help
	},
#endif
#ifdef ENABLE_VIDMODE
	{
		"vidmode", 1,
		(gamma_method_init_func *)vidmode_init,
		(gamma_method_start_func *)vidmode_start,
		(gamma_method_print_help_func *)vidmode_print_help
	},
#endif
#ifdef ENABLE_WINGDI
	{
		"wingdi", 1,
		(gamma_method_init_func *)w32gdi_init,
		(gamma_method_start_func *)w32gdi_start,
		(gamma_method_print_help_func *)w32gdi_print_help
	},
#endif
	{
		"dummy", 0,
		(gamma_method_init_func *)gamma_dummy_init,
		(gamma_method_start_func *)gamma_dummy_start,
		(gamma_method_print_help_func *)gamma_dummy_print_help
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

/* Transition settings. */
static float transition_low = TRANSITION_LOW;
static float transition_high = TRANSITION_HIGH;


/* Print which period (night, day or transition) we're currently in. */
static void
print_period(double elevation)
{
	if (elevation < transition_low) {
		printf(_("Period: Night\n"));
	} else if (elevation < transition_high) {
		float a = (transition_low - elevation) /
			(transition_low - transition_high);
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
	if (elevation < transition_low) {
		result = night;
	} else if (elevation < transition_high) {
		/* Transition period: interpolate */
		float a = (transition_low - elevation) /
			(transition_low - transition_high);
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
	if (args != NULL) {
		while (*args != '\0') {
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

			args = value + strlen(value) + 1;
			i += 1;
		}
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
		 gamma_server_state_t *state,
		 config_ini_state_t *config, char *args,
		 char *gamma)
{
	int r;

	r = method->init(state);
	if (r < 0) {
		fprintf(stderr, _("Initialization of %s failed.\n"),
			method->name);
		return -1;
	}


	/* Set default gamma. */
	if (gamma != NULL) {
		r = gamma_set_option(state, "gamma", gamma, 0);
		free(gamma);
		if (r < 0) {
			gamma_free(state);
			return -1;
		}
	}

	/* Set method options from config file. */
	config_ini_section_t **sections =
		config_ini_get_sections(config, method->name);
	if (sections != NULL) {
		int section_i = 0;
		while (sections[section_i] != NULL) {
			config_ini_setting_t *setting = sections[section_i]->settings;
			while (setting != NULL) {
				r = gamma_set_option(state, setting->name,
						     setting->value, section_i + 1);
				if (r < 0) {
					gamma_free(state);
					fprintf(stderr, _("Failed to set %s option.\n"),
						method->name);
					/* TRANSLATORS: `help' must not be translated. */
					fprintf(stderr, _("Try `-m %s:help' for more information.\n"),
						method->name);
					return -1;
				}
				setting = setting->next;
			}
			section_i++;
		}
		free(sections);
	} else {
		gamma_free(state);
		return -1;
	}

	/* Set method options from command line. */
	if (args != NULL) {
		while (*args != '\0') {
			const char *key = args;
			char *value = strchr(args, '=');
			if (value == NULL) {
				fprintf(stderr, _("Failed to parse option `%s'.\n"),
					args);
				return -1;
			} else {
				*(value++) = '\0';
			}

			r = gamma_set_option(state, key, value, -1);
			if (r < 0) {
				gamma_free(state);
				fprintf(stderr, _("Failed to set %s option.\n"),
					method->name);
				/* TRANSLATORS: `help' must not be translated. */
				fprintf(stderr, _("Try -m %s:help' for more"
						  " information.\n"), method->name);
				return -1;
			}

			args = value + strlen(value) + 1;
		}
	}

	/* Start method. */
	r = method->start(state);
	if (r < 0) {
		gamma_free(state);
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

static int
set_temperature(gamma_server_state_t *state, int temp, float brightness)
{
	gamma_update_all_brightness(state, brightness);
	gamma_update_all_temperature(state, (float)temp);
	return gamma_update(state);
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

	int temp_set = -1;
	int temp_day = -1;
	int temp_night = -1;
	char *gamma = NULL;
	float brightness_day = NAN;
	float brightness_night = NAN;

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
	const char **args = alloca(argc * sizeof(char*));
	int args_count;
	while ((opt = parseopt(argc, argv, "b:c:g:hl:m:oO:prt:vVx", args, &args_count)) != -1) {
		float gamma_[3];
		switch (opt) {
		case 'b':
			parse_brightness_string(optarg, &brightness_day, &brightness_night);
			break;
		case 'c':
			if (config_filepath != NULL) free(config_filepath);
			config_filepath = strdup(optarg);
			if (config_filepath == NULL) {
				perror("strdup");
				abort();
			}
			break;
		case 'g':
			gamma = strdup(optarg);
			if (gamma == NULL) {
				perror("strdup");
				abort();
			}
			r = parse_gamma_string(optarg, gamma_);
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
			if (strcasecmp(*args, "list") == 0) {
				print_provider_list();
				exit(EXIT_SUCCESS);
			}

			provider_args = coalesce_args(args, args_count, '\0', '\0');
			if (provider_args == NULL) {
				perror("coalesce_args");
				exit(EXIT_FAILURE);
			}

			/* Don't save the result of strtof(); we simply want
			   to know if optarg can be parsed as a float. */
			errno = 0;
			char *end;
			strtof(provider_args, &end);
			if (errno == 0 && (end[0] == ':' || end[0] == '\0') && end[1] != '\0') {
				/* Set delimiter to `\0'. */
				end[0] = '\0';
				/* Set provider method to `manual'. */
				char *old_buf = provider_args;
				size_t n = 0;
				while (provider_args[n] != '\0' || provider_args[n + 1] != '\0')
					n++;
				n += 2;
				provider_args = realloc(old_buf, (strlen("manual") + 1 + n) * sizeof(char));
				if (provider_args == NULL) {
					perror("realloc");
					free(old_buf);
					exit(EXIT_FAILURE);
				}
				memmove(provider_args + strlen("manual") + 1, provider_args, n * sizeof(char));
				strcpy(provider_args, "manual");
			}

			/* Lookup provider from name. */
			provider = find_location_provider(provider_args);
			if (provider == NULL) {
				fprintf(stderr, _("Unknown location provider"
						  " `%s'.\n"), provider_args);
				free(provider_args);
				exit(EXIT_FAILURE);
			}

			/* Print provider help if arg is `help'. */
			if (provider_args != NULL &&
			    strcasecmp(provider_args, "help") == 0) {
				provider->print_help(stdout);
				free(provider_args);
				exit(EXIT_SUCCESS);
			}
			break;
		case 'm':
			/* Print list of methods if argument is `list' */
			if (strcasecmp(optarg, "list") == 0) {
				print_method_list();
				exit(EXIT_SUCCESS);
			}

			method_args = coalesce_args(args, args_count, '\0', '\0');
			if (method_args == NULL) {
				perror("coalesce_args");
				exit(EXIT_FAILURE);
			}

			/* Find adjustment method by name. */
			method = find_gamma_method(method_args);
			if (method == NULL) {
				/* TRANSLATORS: This refers to the method
				   used to adjust colors e.g VidMode */
				fprintf(stderr, _("Unknown adjustment method"
						  " `%s'.\n"), optarg);
				free(method_args);
				exit(EXIT_FAILURE);
			}

			/* Print method help if arg is `help'. */
			if (method_args != NULL &&
			    strcasecmp(method_args, "help") == 0) {
				method->print_help(stdout);
				free(method_args);
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
			temp_day = atoi(optarg);
			temp_night = atoi(s);
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
			} else if (strcasecmp(setting->name,
					      "brightness") == 0) {
				if (isnan(brightness_day)) {
					brightness_day = atof(setting->value);
				}
				if (isnan(brightness_night)) {
					brightness_night = atof(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "brightness-day") == 0) {
				if (isnan(brightness_day)) {
					brightness_day = atof(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "brightness-night") == 0) {
				if (isnan(brightness_night)) {
					brightness_night = atof(setting->value);
				}
			} else if (strcasecmp(setting->name,
					      "elevation-high") == 0) {
				transition_high = atof(setting->value);
			} else if (strcasecmp(setting->name,
					      "elevation-low") == 0) {
				transition_low = atof(setting->value);
			} else if (strcasecmp(setting->name, "gamma") == 0) {
				if (gamma == NULL) {
					gamma = strdup(setting->value);
					if (gamma == NULL) {
						perror("strdup");
						abort();
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
	if (isnan(brightness_day)) brightness_day = DEFAULT_BRIGHTNESS;
	if (isnan(brightness_night)) brightness_night = DEFAULT_BRIGHTNESS;
	if (transition < 0) transition = 1;

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
			r = provider_try_start(provider, &location_state, &config_state,
					       provider_args == NULL ? NULL :
					       provider_args + strlen(provider_args) + 1);
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
		        /* TRANSLATORS: Append degree symbols after %f if possible. */
		        printf(_("Location: %.2f %s, %.2f %s\n"),
			       fabs(lat), lat >= 0.f ? _("N") : _("S"),
			       fabs(lon), lon >= 0.f ? _("E") : _("W"));
			printf(_("Temperatures: %dK at day, %dK at night\n"),
			       temp_day, temp_night);
		        /* TRANSLATORS: Append degree symbols if possible. */
			printf(_("Solar elevations: day above %.1f, night below %.1f\n"),
			       transition_high, transition_low);
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

		/* Color temperature at daytime */
		if (temp_day < MIN_TEMP || temp_day > MAX_TEMP) {
			fprintf(stderr,
				_("Temperature must be between %uK and %uK.\n"),
				MIN_TEMP, MAX_TEMP);
			exit(EXIT_FAILURE);
		}
	
		/* Color temperature at night */
		if (temp_night < MIN_TEMP || temp_night > MAX_TEMP) {
			fprintf(stderr,
				_("Temperature must be between %uK and %uK.\n"),
				MIN_TEMP, MAX_TEMP);
			exit(EXIT_FAILURE);
		}

		/* Solar elevations */
		if (transition_high < transition_low) {
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
	if (brightness_day < MIN_BRIGHTNESS ||
	    brightness_day > MAX_BRIGHTNESS ||
	    brightness_night < MIN_BRIGHTNESS ||
	    brightness_night > MAX_BRIGHTNESS) {
		fprintf(stderr,
			_("Brightness values must be between %.1f and %.1f.\n"),
			MIN_BRIGHTNESS, MAX_BRIGHTNESS);
		exit(EXIT_FAILURE);
	}

	if (verbose) {
		printf(_("Brightness: %.2f:%.2f\n"), brightness_day, brightness_night);
	}

	/* Initialize gamma adjustment method. If method is NULL
	   try all methods until one that works is found. */
	gamma_server_state_t state;

	/* Gamma adjustment not needed for print mode */
	if (mode != PROGRAM_MODE_PRINT) {
		if (method != NULL) {
			/* Use method specified on command line. */
			r = method_try_start(method, &state, &config_state,
					     method_args == NULL ? NULL :
					     method_args + strlen(method_args) + 1,
					     gamma);
			if (r < 0) {
				config_ini_free(&config_state);
				exit(EXIT_FAILURE);
			}
		} else {
			/* Try all methods, use the first that works. */
			for (int i = 0; gamma_methods[i].name != NULL; i++) {
				const gamma_method_t *m = &gamma_methods[i];
				if (!m->autostart) continue;

				r = method_try_start(m, &state, &config_state, NULL,
						     gamma);
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
			gamma_free(&state);
			exit(EXIT_FAILURE);
		}

		double elevation = solar_elevation(now, lat, lon);

		if (verbose) {
			/* TRANSLATORS: Append degree symbol if possible. */
			printf(_("Solar elevation: %f\n"), elevation);
		}

		/* Use elevation of sun to set color temperature */
		int temp = (int)calculate_interpolated_value(elevation,
							     temp_day, temp_night);
		float brightness = calculate_interpolated_value(elevation,
								brightness_day, brightness_night);

		if (verbose || mode == PROGRAM_MODE_PRINT) {
			print_period(elevation);
			printf(_("Color temperature: %uK\n"), temp);
			printf(_("Brightness: %.2f\n"), brightness);
		}

		if (mode == PROGRAM_MODE_PRINT) {
			exit(EXIT_SUCCESS);
		}

		/* Adjust temperature */
		r = set_temperature(&state, temp, brightness);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			gamma_free(&state);
			exit(EXIT_FAILURE);
		}
	}
	break;
	case PROGRAM_MODE_MANUAL:
	{
		if (verbose) printf(_("Color temperature: %uK\n"), temp_set);

		/* Adjust temperature */
		r = set_temperature(&state, temp_set, brightness_day);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			gamma_free(&state);
			exit(EXIT_FAILURE);
		}

	}
	break;
	case PROGRAM_MODE_RESET:
	{
		/* Reset screen */
		r = set_temperature(&state, NEUTRAL_TEMP, 1.0);
		if (r < 0) {
			fputs(_("Temperature adjustment failed.\n"), stderr);
			gamma_free(&state);
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
#endif /* HAVE_SIGNAL_H && ! __WIN32__ */

		if (verbose) {
			printf("Status: %s\n", "Enabled");
		}

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
				gamma_free(&state);
				exit(EXIT_FAILURE);
			}

			/* Skip over transition if transitions are disabled */
			int set_adjustments = 0;
			if (!transition) {
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
								temp_day, temp_night);
			float brightness = calculate_interpolated_value(elevation,
								brightness_day, brightness_night);

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
				r = set_temperature(&state, temp, brightness);
				if (r < 0) {
					fputs(_("Temperature adjustment"
						" failed.\n"), stderr);
					gamma_free(&state);
					exit(EXIT_FAILURE);
				}
			}

			/* Sleep for 5 seconds or 0.1 second. */
#ifndef _WIN32
			if (short_trans_delta) usleep(100000);
			else usleep(5000000);
#else /* ! _WIN32 */
			if (short_trans_delta) Sleep(100);
			else Sleep(5000);
#endif /* ! _WIN32 */
		}

		/* Restore saved gamma ramps */
		gamma_restore(&state);
	}
	break;
	}

	/* Clean up gamma adjustment state */
	gamma_free(&state);

	/* Free memory */
	if (method_args != NULL)
		free(method_args);
	if (provider_args != NULL)
		free(provider_args);

	return EXIT_SUCCESS;
}
