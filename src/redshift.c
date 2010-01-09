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

   Copyright (c) 2009  Jon Lund Steffensen <jonlst@gmail.com>
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
#include <sys/signal.h>

#include "solar.h"


#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#define MAX(x,y)  ((x) > (y) ? (x) : (y))


#if !(defined(ENABLE_RANDR) || defined(ENABLE_VIDMODE))
# error "At least one of RANDR or VidMode must be enabled."
#endif

#ifdef ENABLE_RANDR
# include "randr.h"
#endif

#ifdef ENABLE_VIDMODE
# include "vidmode.h"
#endif


/* Union of state data for gamma adjustment methods */
typedef union {
#ifdef ENABLE_RANDR
	randr_state_t randr;
#endif
#ifdef ENABLE_VIDMODE
	vidmode_state_t vidmode;
#endif
} gamma_state_t;


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


/* Help and usage messages */
#define USAGE  \
	"Usage: %s -l LAT:LON -t DAY:NIGHT [OPTIONS...]\n"
#define HELP \
	USAGE \
	" " PACKAGE_STRING "\n" \
	" Set color temperature of display according to time of day.\n" \
	"  -g R:G:B\tAdditional gamma correction to apply\n" \
	"  -h\t\tDisplay this help message\n" \
	"  -l LAT:LON\tYour current location\n" \
	"  -m METHOD\tMethod to use to set color temperature" \
	" (randr or vidmode)\n"				      \
	"  -o\t\tOne shot mode (do not continously adjust" \
	" color temperature)\n"					   \
	"  -r\t\tDisable initial temperature transition\n" \
	"  -s SCREEN\tX screen to apply adjustments to\n" \
	"  -t DAY:NIGHT\tColor temperature to set at daytime/night\n" \
	"  -v\t\tVerbose output\n" \
	"Please report bugs to <" PACKAGE_BUGREPORT ">\n"

/* DEGREE SIGN is Unicode U+00b0 */
#define DEG_CHAR  0xb0


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


/* Restore saved gamma ramps with the appropriate adjustment method. */
static void
gamma_state_restore(gamma_state_t *state, int use_randr)
{
	switch (use_randr) {
#ifdef ENABLE_VIDMODE
	case 0:
		vidmode_restore(&state->vidmode);
		break;
#endif
#ifdef ENABLE_RANDR
	case 1:
		randr_restore(&state->randr);
		break;
#endif
	}
}

/* Free the state associated with the appropriate adjustment method. */
static void
gamma_state_free(gamma_state_t *state, int use_randr)
{
	switch (use_randr) {
#ifdef ENABLE_VIDMODE
	case 0:
		vidmode_free(&state->vidmode);
		break;
#endif
#ifdef ENABLE_RANDR
	case 1:
		randr_free(&state->randr);
		break;
#endif
	}
}

/* Set temperature with the appropriate adjustment method. */
static int
gamma_state_set_temperature(gamma_state_t *state, int use_randr,
			    int temp, float gamma[3])
{
	switch (use_randr) {
#ifdef ENABLE_VIDMODE
	case 0:
		return vidmode_set_temperature(&state->vidmode, temp, gamma);
#endif
#ifdef ENABLE_RANDR
	case 1:
		return randr_set_temperature(&state->randr, temp, gamma);
#endif
	}

	return -1;
}


/* Calculate color temperature for the specified solar elevation. */
static int
calculate_temp(double elevation, int temp_day, int temp_night,
	       int verbose)
{
	int temp = 0;
	if (elevation < TRANSITION_LOW) {
		temp = temp_night;
		if (verbose) printf("Period: Night\n");
	} else if (elevation < TRANSITION_HIGH) {
		/* Transition period: interpolate */
		float a = (TRANSITION_LOW - elevation) /
			(TRANSITION_LOW - TRANSITION_HIGH);
		temp = (1.0-a)*temp_night + a*temp_day;
		if (verbose) {
			printf("Period: Transition (%.2f%% day)\n", a*100);
		}
	} else {
		temp = temp_day;
		if (verbose) printf("Period: Daytime\n");
	}

	return temp;
}


int
main(int argc, char *argv[])
{
	int r;

	/* Init locale for degree symbol. */
	char *loc = setlocale(LC_CTYPE, "");
	if (loc == NULL) {
		fprintf(stderr, "Unable to set locale.\n");
		exit(EXIT_FAILURE);
	}

	/* Initialize to defaults */
	float lat = NAN;
	float lon = NAN;
	int temp_day = DEFAULT_DAY_TEMP;
	int temp_night = DEFAULT_NIGHT_TEMP;
	float gamma[3] = { DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA };
	int use_randr = -1;
	int screen_num = -1;
	int transition = 1;
	int one_shot = 0;
	int verbose = 0;
	char *s;

	/* Parse arguments. */
	int opt;
	while ((opt = getopt(argc, argv, "g:hl:m:ors:t:v")) != -1) {
		switch (opt) {
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
  					fprintf(stderr, USAGE, argv[0]);
					exit(EXIT_FAILURE);
				}

				*(s++) = '\0';
				gamma[1] = atof(g_s); /* Blue */
				gamma[2] = atof(s); /* Green */
			}
			break;
		case 'h':
			printf(HELP, argv[0]);
			exit(EXIT_SUCCESS);
			break;
		case 'l':
			s = strchr(optarg, ':');
			if (s == NULL) {
				fprintf(stderr, USAGE, argv[0]);
				exit(EXIT_FAILURE);
			}
			*(s++) = '\0';
			lat = atof(optarg);
			lon = atof(s);
			break;
		case 'm':
			if (strcmp(optarg, "randr") == 0 ||
			    strcmp(optarg, "RANDR") == 0) {
#ifdef ENABLE_RANDR
				use_randr = 1;
#else
				fprintf(stderr, "RANDR method was not"
					" enabled at compile time.\n");
				exit(EXIT_FAILURE);
#endif
			} else if (strcmp(optarg, "vidmode") == 0 ||
				   strcmp(optarg, "VidMode") == 0) {
#ifdef ENABLE_VIDMODE
				use_randr = 0;
#else
				fprintf(stderr, "VidMode method was not"
					" enabled at compile time.\n");
				exit(EXIT_FAILURE);
#endif
			} else {
				fprintf(stderr, "Unknown method `%s'.\n",
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
				fprintf(stderr, USAGE, argv[0]);
				exit(EXIT_FAILURE);
			}
			*(s++) = '\0';
			temp_day = atoi(optarg);
			temp_night = atoi(s);
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			fprintf(stderr, USAGE, argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	/* Latitude and longitude must be set */
	if (isnan(lat) || isnan(lon)) {
		fprintf(stderr, USAGE, argv[0]);
		fprintf(stderr, "Latitude and longitude must be set.\n");
		exit(EXIT_FAILURE);
	}

	if (verbose) {
		printf("Location: %f%lc, %f%lc\n",
		       lat, DEG_CHAR, lon, DEG_CHAR);
	}

	/* Latitude */
	if (lat < MIN_LAT || lat > MAX_LAT) {
		fprintf(stderr,
			"Latitude must be between %.1f%lc and %.1f%lc.\n",
			MIN_LAT, DEG_CHAR, MAX_LAT, DEG_CHAR);
		exit(EXIT_FAILURE);
	}

	/* Longitude */
	if (lon < MIN_LON || lon > MAX_LON) {
		fprintf(stderr,
			"Longitude must be between %.1f%lc and %.1f%lc.\n",
			MIN_LON, DEG_CHAR, MAX_LON, DEG_CHAR);
		exit(EXIT_FAILURE);
	}

	/* Color temperature at daytime */
	if (temp_day < MIN_TEMP || temp_day >= MAX_TEMP) {
		fprintf(stderr, "Temperature must be between %uK and %uK.\n",
			MIN_TEMP, MAX_TEMP);
		exit(EXIT_FAILURE);
	}

	/* Color temperature at night */
	if (temp_night < MIN_TEMP || temp_night >= MAX_TEMP) {
		fprintf(stderr, "Temperature must be between %uK and %uK.\n",
			MIN_TEMP, MAX_TEMP);
		exit(EXIT_FAILURE);
	}

	/* Gamma */
	if (gamma[0] < MIN_GAMMA || gamma[0] > MAX_GAMMA ||
	    gamma[1] < MIN_GAMMA || gamma[1] > MAX_GAMMA ||
	    gamma[2] < MIN_GAMMA || gamma[2] > MAX_GAMMA) {
		fprintf(stderr, "Gamma value must be between %.1f and %.1f.\n",
			MIN_GAMMA, MAX_GAMMA);
		exit(EXIT_FAILURE);
	}

	if (verbose) {
		printf("Gamma: %.3f, %.3f, %.3f\n",
		       gamma[0], gamma[1], gamma[2]);
	}

	/* Initialize gamma adjustment method. If use_randr is negative
	   try all methods until one that works is found. */
	gamma_state_t state;
#ifdef ENABLE_RANDR
	if (use_randr < 0 || use_randr == 1) {
		/* Initialize RANDR state */
		r = randr_init(&state.randr, screen_num);
		if (r < 0) {
			fprintf(stderr, "Initialization of RANDR failed.\n");
			if (use_randr < 0) {
				fprintf(stderr, "Trying other method...\n");
			} else {
				exit(EXIT_FAILURE);
			}
		} else {
			use_randr = 1;
		}
	}
#endif

#ifdef ENABLE_VIDMODE
	if (use_randr < 0 || use_randr == 0) {
		/* Initialize VidMode state */
		r = vidmode_init(&state.vidmode, screen_num);
		if (r < 0) {
			fprintf(stderr, "Initialization of VidMode failed.\n");
			exit(EXIT_FAILURE);
		} else {
			use_randr = 0;
		}
	}
#endif

	if (one_shot) {
		/* Current angular elevation of the sun */
		struct timespec now;
		r = clock_gettime(CLOCK_REALTIME, &now);
		if (r < 0) {
			perror("clock_gettime");
			gamma_state_free(&state, use_randr);
			exit(EXIT_FAILURE);
		}

		double elevation = solar_elevation(now, lat, lon);

		if (verbose) {
			printf("Solar elevation: %f%lc\n", elevation,
			       DEG_CHAR);
		}

		/* Use elevation of sun to set color temperature */
		int temp = calculate_temp(elevation, temp_day, temp_night,
					  verbose);

		if (verbose) printf("Color temperature: %uK\n", temp);

		/* Adjust temperature */
		r = gamma_state_set_temperature(&state, use_randr,
						temp, gamma);
		if (r < 0) {
			fprintf(stderr, "Temperature adjustment failed.\n");
			gamma_state_free(&state, use_randr);
			exit(EXIT_FAILURE);
		}
	} else {
		/* Transition state */
		struct timespec short_trans_end;
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
			struct timespec now;
			r = clock_gettime(CLOCK_REALTIME, &now);
			if (r < 0) {
				perror("clock_gettime");
				gamma_state_free(&state, use_randr);
				exit(EXIT_FAILURE);
			}

			/* Set up a new transition */
			if (short_trans_create) {
				if (transition) {
					memcpy(&short_trans_end, &now,
					       sizeof(struct timespec));
					short_trans_end.tv_sec +=
						short_trans_len;

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
				double start = now.tv_sec +
					now.tv_nsec / 1000000000.0;
				double end = short_trans_end.tv_sec +
					short_trans_end.tv_nsec /
					1000000000.0;

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
					gamma_state_restore(&state, use_randr);
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
				printf("Temperature: %iK\n", temp);
			}

			/* Adjust temperature */
			if (!disabled || short_trans) {
				r = gamma_state_set_temperature(&state,
								use_randr,
								temp, gamma);
				if (r < 0) {
					fprintf(stderr, "Temperature"
						" adjustment failed.\n");
					gamma_state_free(&state, use_randr);
					exit(EXIT_FAILURE);
				}
			}

			/* Sleep for a while */
			if (short_trans) usleep(100000);
			else usleep(5000000);
		}

		/* Restore saved gamma ramps */
		gamma_state_restore(&state, use_randr);
	}

	/* Clean up gamma adjustment state */
	gamma_state_free(&state, use_randr);

	return EXIT_SUCCESS;
}
