/* options.c -- Program options
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

   Copyright (c) 2017  Jon Lund Steffensen <jonlst@gmail.com>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "redshift.h"
#include "config-ini.h"
#include "options.h"
#include "solar.h"

/* Angular elevation of the sun at which the color temperature
   transition period starts and ends (in degress).
   Transition during twilight, and while the sun is lower than
   3.0 degrees above the horizon. */
#define TRANSITION_LOW     SOLAR_CIVIL_TWILIGHT_ELEV
#define TRANSITION_HIGH    3.0

/* Default values for parameters. */
#define DEFAULT_DAY_TEMP    6500
#define DEFAULT_NIGHT_TEMP  4500
#define DEFAULT_BRIGHTNESS   1.0
#define DEFAULT_GAMMA        1.0


/* A brightness string contains either one floating point value,
   or two values separated by a colon. */
static void
parse_brightness_string(
	const char *str, float *bright_day, float *bright_night)
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

/* Print help text. */
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
		"  -P\t\tReset existing gamma ramps before applying new"
		" color effect\n"
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

/* Print list of adjustment methods. */
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

/* Print list of location providers. */
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

/* Return the gamma method with the given name. */
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

/* Return location provider with the given name. */
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


/* Initialize options struct. */
void
options_init(options_t *options)
{
	options->config_filepath = NULL;

	/* Default elevation values. */
	options->scheme.high = TRANSITION_HIGH;
	options->scheme.low = TRANSITION_LOW;

	/* Settings for day, night and transition period.
	   Initialized to indicate that the values are not set yet. */
	options->scheme.use_time = 0;
	options->scheme.dawn.start = -1;
	options->scheme.dawn.end = -1;
	options->scheme.dusk.start = -1;
	options->scheme.dusk.end = -1;

	options->scheme.day.temperature = -1;
	options->scheme.day.gamma[0] = NAN;
	options->scheme.day.brightness = NAN;

	options->scheme.night.temperature = -1;
	options->scheme.night.gamma[0] = NAN;
	options->scheme.night.brightness = NAN;

	/* Temperature for manual mode */
	options->temp_set = -1;

	options->method = NULL;
	options->method_args = NULL;

	options->provider = NULL;
	options->provider_args = NULL;

	options->use_fade = -1;
	options->preserve_gamma = 1;
	options->mode = PROGRAM_MODE_CONTINUAL;
	options->verbose = 0;
}

/* Parse a single option from the command-line. */
static int
parse_command_line_option(
	const char option, char *value, options_t *options,
	const char *program_name, const gamma_method_t *gamma_methods,
	const location_provider_t *location_providers)
{
	int r;
	char *s;

	switch (option) {
	case 'b':
		parse_brightness_string(
			value, &options->scheme.day.brightness,
			&options->scheme.night.brightness);
		break;
	case 'c':
		free(options->config_filepath);
		options->config_filepath = strdup(value);
		break;
	case 'g':
		r = parse_gamma_string(value, options->scheme.day.gamma);
		if (r < 0) {
			fputs(_("Malformed gamma argument.\n"), stderr);
			fputs(_("Try `-h' for more information.\n"), stderr);
			return -1;
		}

		/* Set night gamma to the same value as day gamma.
		   To set these to distinct values use the config
		   file. */
		memcpy(options->scheme.night.gamma,
		       options->scheme.day.gamma,
		       sizeof(options->scheme.night.gamma));
		break;
	case 'h':
		print_help(program_name);
		exit(EXIT_SUCCESS);
		break;
	case 'l':
		/* Print list of providers if argument is `list' */
		if (strcasecmp(value, "list") == 0) {
			print_provider_list(location_providers);
			exit(EXIT_SUCCESS);
		}

		char *provider_name = NULL;

		/* Don't save the result of strtof(); we simply want
		   to know if value can be parsed as a float. */
		errno = 0;
		char *end;
		strtof(value, &end);
		if (errno == 0 && *end == ':') {
			/* Use instead as arguments to `manual'. */
			provider_name = "manual";
			options->provider_args = value;
		} else {
			/* Split off provider arguments. */
			s = strchr(value, ':');
			if (s != NULL) {
				*(s++) = '\0';
				options->provider_args = s;
			}

			provider_name = value;
		}

		/* Lookup provider from name. */
		options->provider = find_location_provider(
			location_providers, provider_name);
		if (options->provider == NULL) {
			fprintf(stderr, _("Unknown location provider `%s'.\n"),
				provider_name);
			return -1;
		}

		/* Print provider help if arg is `help'. */
		if (options->provider_args != NULL &&
		    strcasecmp(options->provider_args, "help") == 0) {
			options->provider->print_help(stdout);
			exit(EXIT_SUCCESS);
		}
		break;
	case 'm':
		/* Print list of methods if argument is `list' */
		if (strcasecmp(value, "list") == 0) {
			print_method_list(gamma_methods);
			exit(EXIT_SUCCESS);
		}

		/* Split off method arguments. */
		s = strchr(value, ':');
		if (s != NULL) {
			*(s++) = '\0';
			options->method_args = s;
		}

		/* Find adjustment method by name. */
		options->method = find_gamma_method(gamma_methods, value);
		if (options->method == NULL) {
			/* TRANSLATORS: This refers to the method
			   used to adjust colors e.g VidMode */
			fprintf(stderr, _("Unknown adjustment method `%s'.\n"),
				value);
			return -1;
		}

		/* Print method help if arg is `help'. */
		if (options->method_args != NULL &&
		    strcasecmp(options->method_args, "help") == 0) {
			options->method->print_help(stdout);
			exit(EXIT_SUCCESS);
		}
		break;
	case 'o':
		options->mode = PROGRAM_MODE_ONE_SHOT;
		break;
	case 'O':
		options->mode = PROGRAM_MODE_MANUAL;
		options->temp_set = atoi(value);
		break;
	case 'p':
		options->mode = PROGRAM_MODE_PRINT;
		break;
	case 'P':
		options->preserve_gamma = 0;
		break;
	case 'r':
		options->use_fade = 0;
		break;
	case 't':
		s = strchr(value, ':');
		if (s == NULL) {
			fputs(_("Malformed temperature argument.\n"), stderr);
			fputs(_("Try `-h' for more information.\n"), stderr);
			return -1;
		}
		*(s++) = '\0';
		options->scheme.day.temperature = atoi(value);
		options->scheme.night.temperature = atoi(s);
		break;
	case 'v':
		options->verbose = 1;
		break;
	case 'V':
		printf("%s\n", PACKAGE_STRING);
		exit(EXIT_SUCCESS);
		break;
	case 'x':
		options->mode = PROGRAM_MODE_RESET;
		break;
	case '?':
		fputs(_("Try `-h' for more information.\n"), stderr);
		return -1;
		break;
	}

	return 0;
}

/* Parse command line arguments. */
void
options_parse_args(
	options_t *options, int argc, char *argv[],
	const gamma_method_t *gamma_methods,
	const location_provider_t *location_providers)
{
	const char* program_name = argv[0];
	int opt;
	while ((opt = getopt(argc, argv, "b:c:g:hl:m:oO:pPrt:vVx")) != -1) {
		char option = opt;
		int r = parse_command_line_option(
			option, optarg, options, program_name, gamma_methods,
			location_providers);
		if (r < 0) exit(EXIT_FAILURE);
	}
}

/* Parse a single key-value pair from the configuration file. */
static int
parse_config_file_option(
	const char *key, const char *value, options_t *options,
	const gamma_method_t *gamma_methods,
	const location_provider_t *location_providers)
{
	if (strcasecmp(key, "temp-day") == 0) {
		if (options->scheme.day.temperature < 0) {
			options->scheme.day.temperature = atoi(value);
		}
	} else if (strcasecmp(key, "temp-night") == 0) {
		if (options->scheme.night.temperature < 0) {
			options->scheme.night.temperature = atoi(value);
		}
	} else if (strcasecmp(key, "transition") == 0 ||
		   strcasecmp(key, "fade") == 0) {
		/* "fade" is preferred, "transition" is
		   deprecated as the setting key. */
		if (options->use_fade < 0) {
			options->use_fade = !!atoi(value);
		}
	} else if (strcasecmp(key, "brightness") == 0) {
		if (isnan(options->scheme.day.brightness)) {
			options->scheme.day.brightness = atof(value);
		}
		if (isnan(options->scheme.night.brightness)) {
			options->scheme.night.brightness = atof(value);
		}
	} else if (strcasecmp(key, "brightness-day") == 0) {
		if (isnan(options->scheme.day.brightness)) {
			options->scheme.day.brightness = atof(value);
		}
	} else if (strcasecmp(key, "brightness-night") == 0) {
		if (isnan(options->scheme.night.brightness)) {
			options->scheme.night.brightness = atof(value);
		}
	} else if (strcasecmp(key, "elevation-high") == 0) {
		options->scheme.high = atof(value);
	} else if (strcasecmp(key, "elevation-low") == 0) {
		options->scheme.low = atof(value);
	} else if (strcasecmp(key, "gamma") == 0) {
		if (isnan(options->scheme.day.gamma[0])) {
			int r = parse_gamma_string(
				value, options->scheme.day.gamma);
			if (r < 0) {
				fputs(_("Malformed gamma setting.\n"), stderr);
				return -1;
			}
			memcpy(options->scheme.night.gamma,
			       options->scheme.day.gamma,
			       sizeof(options->scheme.night.gamma));
		}
	} else if (strcasecmp(key, "gamma-day") == 0) {
		if (isnan(options->scheme.day.gamma[0])) {
			int r = parse_gamma_string(
				value, options->scheme.day.gamma);
			if (r < 0) {
				fputs(_("Malformed gamma setting.\n"), stderr);
				return -1;
			}
		}
	} else if (strcasecmp(key, "gamma-night") == 0) {
		if (isnan(options->scheme.night.gamma[0])) {
			int r = parse_gamma_string(
				value, options->scheme.night.gamma);
			if (r < 0) {
				fputs(_("Malformed gamma setting.\n"), stderr);
				return -1;
			}
		}
	} else if (strcasecmp(key, "adjustment-method") == 0) {
		if (options->method == NULL) {
			options->method = find_gamma_method(
				gamma_methods, value);
			if (options->method == NULL) {
				fprintf(stderr, _("Unknown adjustment"
						  " method `%s'.\n"), value);
				return -1;
			}
		}
	} else if (strcasecmp(key, "location-provider") == 0) {
		if (options->provider == NULL) {
			options->provider = find_location_provider(
				location_providers, value);
			if (options->provider == NULL) {
				fprintf(stderr, _("Unknown location"
						  " provider `%s'.\n"), value);
				return -1;
			}
		}
	} else if (strcasecmp(key, "dawn-time") == 0) {
		if (options->scheme.dawn.start < 0) {
			int r = parse_transition_range(
				value, &options->scheme.dawn);
			if (r < 0) {
				fprintf(stderr, _("Malformed dawn-time"
						  " setting `%s'.\n"), value);
				return -1;
			}
		}
	} else if (strcasecmp(key, "dusk-time") == 0) {
		if (options->scheme.dusk.start < 0) {
			int r = parse_transition_range(
				value, &options->scheme.dusk);
			if (r < 0) {
				fprintf(stderr, _("Malformed dusk-time"
						  " setting `%s'.\n"), value);
				return -1;
			}
		}
	} else {
		fprintf(stderr, _("Unknown configuration setting `%s'.\n"),
			key);
	}

	return 0;
}

/* Parse options defined in the config file. */
void
options_parse_config_file(
	options_t *options, config_ini_state_t *config_state,
	const gamma_method_t *gamma_methods,
	const location_provider_t *location_providers)
{
	/* Read global config settings. */
	config_ini_section_t *section = config_ini_get_section(
		config_state, "redshift");
	if (section == NULL) return;

	config_ini_setting_t *setting = section->settings;
	while (setting != NULL) {
		int r = parse_config_file_option(
			setting->name, setting->value, options,
			gamma_methods, location_providers);
		if (r < 0) exit(EXIT_FAILURE);

		setting = setting->next;
	}
}

/* Replace unspecified options with default values. */
void
options_set_defaults(options_t *options)
{
	if (options->scheme.day.temperature < 0) {
		options->scheme.day.temperature = DEFAULT_DAY_TEMP;
	}
	if (options->scheme.night.temperature < 0) {
		options->scheme.night.temperature = DEFAULT_NIGHT_TEMP;
	}

	if (isnan(options->scheme.day.brightness)) {
		options->scheme.day.brightness = DEFAULT_BRIGHTNESS;
	}
	if (isnan(options->scheme.night.brightness)) {
		options->scheme.night.brightness = DEFAULT_BRIGHTNESS;
	}

	if (isnan(options->scheme.day.gamma[0])) {
		options->scheme.day.gamma[0] = DEFAULT_GAMMA;
		options->scheme.day.gamma[1] = DEFAULT_GAMMA;
		options->scheme.day.gamma[2] = DEFAULT_GAMMA;
	}
	if (isnan(options->scheme.night.gamma[0])) {
		options->scheme.night.gamma[0] = DEFAULT_GAMMA;
		options->scheme.night.gamma[1] = DEFAULT_GAMMA;
		options->scheme.night.gamma[2] = DEFAULT_GAMMA;
	}

	if (options->use_fade < 0) options->use_fade = 1;
}
