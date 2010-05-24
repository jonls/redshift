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

#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

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
# include "randr.h"
#endif

#ifdef ENABLE_VIDMODE
# include "vidmode.h"
#endif

#ifdef ENABLE_WINGDI
# include "w32gdi.h"
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

/* Enum of gamma adjustment methods */
typedef enum {
	GAMMA_METHOD_RANDR,
	GAMMA_METHOD_VIDMODE,
	GAMMA_METHOD_WINGDI,
	GAMMA_METHOD_MAX
} gamma_method_t;

typedef int gamma_method_init_func(void *state, int screen_num, int crtc_num);
typedef void gamma_method_free_func(void *state);
typedef void gamma_method_restore_func(void *state);
typedef int gamma_method_set_temperature_func(void *state, int temp,
					      float gamma[3]);

typedef struct {
	char *name;
	gamma_method_init_func *init;
	gamma_method_free_func *free;
	gamma_method_restore_func *restore;
	gamma_method_set_temperature_func *set_temperature;
} gamma_method_spec_t;


/* Gamma adjustment method structs */
static const gamma_method_spec_t gamma_methods[] = {
#ifdef ENABLE_RANDR
	{
		"RANDR",
		(gamma_method_init_func *)randr_init,
		(gamma_method_free_func *)randr_free,
		(gamma_method_restore_func *)randr_restore,
		(gamma_method_set_temperature_func *)randr_set_temperature
	},
#endif
#ifdef ENABLE_VIDMODE
	{
		"VidMode",
		(gamma_method_init_func *)vidmode_init,
		(gamma_method_free_func *)vidmode_free,
		(gamma_method_restore_func *)vidmode_restore,
		(gamma_method_set_temperature_func *)vidmode_set_temperature
	},
#endif
#ifdef ENABLE_WINGDI
	{
		"WinGDI",
		(gamma_method_init_func *)w32gdi_init,
		(gamma_method_free_func *)w32gdi_free,
		(gamma_method_restore_func *)w32gdi_restore,
		(gamma_method_set_temperature_func *)w32gdi_set_temperature
	},
#endif
	{ NULL, NULL, NULL, NULL, NULL }
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

/* Angular elevation of the sun at which the color temperature
   transition period starts and ends (in degress).
   Transition during twilight, and while the sun is lower than
   3.0 degrees above the horizon. */
#define TRANSITION_LOW     SOLAR_CIVIL_TWILIGHT_ELEV
#define TRANSITION_HIGH    3.0


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
	   no-wrap */
	fputs(_("  -g R:G:B\tAdditional gamma correction to apply\n"
		"  -l LAT:LON\tYour current location\n"
		"  -m METHOD\tMethod to use to set color temperature\n"
		"  \t\t(Type `list' to see available methods)\n"
		"  -o\t\tOne shot mode (do not continously adjust"
		" color temperature)\n"
		"  -r\t\tDisable temperature transitions\n"
		"  -s SCREEN\tX screen to apply adjustments to\n"
		"  -c CRTC\tCRTC to apply adjustments to (RANDR only)\n"
		"  -t DAY:NIGHT\tColor temperature to set at daytime/night\n"),
	      stdout);
	fputs("\n", stdout);

	/* TRANSLATORS: help output 5 */
	printf(_("Default values:\n\n"
		 "  Daytime temperature: %uK\n"
		 "  Night temperature: %uK\n"),
	       DEFAULT_DAY_TEMP, DEFAULT_NIGHT_TEMP);

	fputs("\n", stdout);

	/* TRANSLATORS: help output 6 */
	printf(_("Please report bugs to <%s>\n"), PACKAGE_BUGREPORT);
}

static void
print_method_list()
{
	fputs(_("Available adjustment methods:\n"), stdout);
	for (int i = 0; gamma_methods[i].name != NULL; i++) {
		printf("  %s\n", gamma_methods[i].name);
	}
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

	/* Initialize to defaults */
	float lat = NAN;
	float lon = NAN;
	int temp_day = DEFAULT_DAY_TEMP;
	int temp_night = DEFAULT_NIGHT_TEMP;
	float gamma[3] = { DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA };

	const gamma_method_spec_t *method = NULL;

	int screen_num = -1;
	int crtc_num = -1;
	int transition = 1;
	int one_shot = 0;
	int verbose = 0;
	char *s;

	/* Parse arguments. */
	int opt;
	while ((opt = getopt(argc, argv, "c:g:hl:m:ors:t:v")) != -1) {
		switch (opt) {
		case 'c':
			crtc_num = atoi(optarg);
			break;
		case 'g':
			s = strchr(optarg, ':');
			if (s == NULL) {
				/* Use value for all channels */
				float g = atof(optarg);
				gamma[0] = gamma[1] = gamma[2] = g;
			} else {
				/* Parse separate value for each channel */
				*(s++) = '\0';
				gamma[0] = atof(optarg); /* Red */
				char *g_s = s;
				s = strchr(s, ':');
				if (s == NULL) {
  					fputs(_("Malformed gamma argument.\n"),
					      stderr);
					fputs(_("Try `-h' for more"
						" information.\n"), stderr);
					exit(EXIT_FAILURE);
				}

				*(s++) = '\0';
				gamma[1] = atof(g_s); /* Blue */
				gamma[2] = atof(s); /* Green */
			}
			break;
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		case 'l':
			s = strchr(optarg, ':');
			if (s == NULL) {
				fputs(_("Malformed location argument.\n"),
				      stderr);
				fputs(_("Try `-h' for more information.\n"),
				      stderr);
				exit(EXIT_FAILURE);
			}
			*(s++) = '\0';
			lat = atof(optarg);
			lon = atof(s);
			break;
		case 'm':
			/* Print list of methods if argument is `list' */
			if (strcasecmp(optarg, "list") == 0) {
				print_method_list();
				exit(EXIT_SUCCESS);
			}

			/* Lookup argument in gamma methods table */
			for (int i = 0; gamma_methods[i].name != NULL; i++) {
				const gamma_method_spec_t *m =
					&gamma_methods[i];
				if (strcasecmp(optarg, m->name) == 0) {
					method = m;
				}
			}

			if (method == NULL) {
				/* TRANSLATORS: This refers to the method
				   used to adjust colors e.g VidMode */
				fprintf(stderr, _("Unknown method `%s'.\n"),
					optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 'o':
			one_shot = 1;
			break;
		case 'r':
			transition = 0;
			break;
		case 's':
			screen_num = atoi(optarg);
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
		case '?':
			fputs(_("Try `-h' for more information.\n"), stderr);
			exit(EXIT_FAILURE);
			break;
		}
	}

	/* Latitude and longitude must be set */
	if (isnan(lat) || isnan(lon)) {
		fputs(_("Latitude and longitude must be set.\n"), stderr);
		fputs(_("Try `-h' for more information.\n"), stderr);
		exit(EXIT_FAILURE);
	}

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
			_("Longitude must be between %.1f and %.1f.\n"),
			MIN_LON, MAX_LON);
		exit(EXIT_FAILURE);
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

	/* CRTC can only be selected for RANDR */
	if (crtc_num > -1 && method != GAMMA_METHOD_RANDR) {
		fprintf(stderr, _("CRTC can only be selected"
				  " with the RANDR method.\n"));
		exit(EXIT_FAILURE);
	}

	/* Initialize gamma adjustment method. If method is negative
	   try all methods until one that works is found. */
	gamma_state_t state;

	if (method != NULL) {
		/* Use method specified on command line */
		r = method->init(&state, screen_num, crtc_num);
		if (r < 0) {
			fprintf(stderr, _("Initialization of %s failed.\n"),
				method->name);
			exit(EXIT_FAILURE);
		}
	} else {
		/* Try all methods, use the first that works */
		for (int i = 0; gamma_methods[i].name != NULL; i++) {
			const gamma_method_spec_t *m = &gamma_methods[i];
			r = m->init(&state, screen_num, crtc_num);
			if (r < 0) {
				fprintf(stderr, _("Initialization of %s"
						  " failed.\n"), m->name);
				fputs(_("Trying other method...\n"), stderr);
			} else {
				method = m;
				break;
			}
		}

		/* Failure if no methods were successful at this point. */
		if (method == NULL) {
			fputs(_("No more methods to try.\n"), stderr);
			exit(EXIT_FAILURE);
		}
	}

	if (one_shot) {
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
	} else {
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

	/* Clean up gamma adjustment state */
	method->free(&state);

	return EXIT_SUCCESS;
}
