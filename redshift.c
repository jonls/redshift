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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <locale.h>

#include "solar.h"
#include "colortemp.h"


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


#define USAGE  \
	"Usage: %s -l LAT:LON -t DAY:NIGHT [OPTIONS...]\n"
#define HELP \
	USAGE \
	" Set color temperature of display according to time of day.\n" \
	"  -g R:G:B\tAdditional gamma correction to apply\n" \
	"  -h\t\tDisplay this help message\n" \
	"  -l LAT:LON\tYour current location\n" \
	"  -t DAY:NIGHT\tColor temperature to set at daytime/night\n" \
	"  -v\t\tVerbose output\n"

/* DEGREE SIGN is Unicode U+00b0 */
#define DEG_CHAR  0xb0


int
main(int argc, char *argv[])
{
	/* Check extensions needed for color temperature adjustment. */
	int r = colortemp_check_extension();
	if (r < 0) {
		fprintf(stderr, "Unable to set color temperature:"
			" Needed extension is missing.\n");
		exit(EXIT_FAILURE);
	}

	/* Init locale for degree symbol. */
	char *loc = setlocale(LC_CTYPE, "");
	if (loc == NULL) {
		fprintf(stderr, "Unable to set locale.\n");
		exit(EXIT_FAILURE);
	}

	/* Parse arguments. */
	float lat = NAN;
	float lon = NAN;
	int temp_day = DEFAULT_DAY_TEMP;
	int temp_night = DEFAULT_NIGHT_TEMP;
	float gamma[3] = { DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA };
	int verbose = 0;
	char *s;

	int opt;
	while ((opt = getopt(argc, argv, "g:hl:t:v")) != -1) {
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

	/* Current angular elevation of the sun */
	time_t now = time(NULL);
	double elevation = solar_elevation(now, lat, lon);

	if (verbose) {
		printf("Solar elevation: %f%lc\n", elevation, DEG_CHAR);
	}

	/* Use elevation of sun to set color temperature */
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

	if (verbose) {
		printf("Color temperature: %uK\n", temp);
		printf("Gamma: %.3f, %.3f, %.3f\n",
		       gamma[0], gamma[1], gamma[2]);
	}

	/* Set color temperature */
	r = colortemp_set_temperature(temp, gamma);
	if (r < 0) {
		fprintf(stderr, "Unable to set color temperature.\n");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
