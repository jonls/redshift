/* redshift.c */
/* Copyright (c) 2009, Jon Lund Steffensen <jonlst@gmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <locale.h>

#include "solar.h"
#include "colortemp.h"

#define MIN_LAT   -90.0
#define MAX_LAT    90.0
#define MIN_LON  -180.0
#define MAX_LON   180.0
#define MIN_TEMP   1000
#define MAX_TEMP  10000

#define DEFAULT_DAY_TEMP    5500
#define DEFAULT_NIGHT_TEMP  3700

#define USAGE  \
	"Usage: %s -l LAT:LON -t DAY:NIGHT [OPTIONS...]\n"
#define HELP \
	USAGE \
	" Set color temperature of display according to time of day.\n" \
	"  -g GAMMA\tAdditional gamma correction to apply\n" \
	"  -h\t\tDisplay this help message\n" \
	"  -l LAT:LON\tYour current location\n" \
	"  -t DAY:NIGHT\tColor temperature to set at night/day\n" \
	"  -v\t\tVerbose output\n"

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

	/* Init locale for special (degree) symbol */
	char *loc = setlocale(LC_ALL, "");
	if (loc == NULL) {
		fprintf(stderr, "Unable to set locale.\n");
		exit(EXIT_FAILURE);
	}

	float lat = NAN;
	float lon = NAN;
	int temp_day = DEFAULT_DAY_TEMP;
	int temp_night = DEFAULT_NIGHT_TEMP;
	float gamma = 1.0;
	int verbose = 0;
	char *s;
	
	int opt;
	while ((opt = getopt(argc, argv, "g:hl:t:v")) != -1) {
		switch (opt) {
		case 'g':
			gamma = atof(optarg);
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

	/* Current angular elevation of the sun */
	time_t now = time(NULL);
	double elevation = solar_elevation(now, lat, lon);

	if (verbose) {
		printf("Solar elevation is %f%lc.\n", elevation, DEG_CHAR);
	}

	/* Use elevation of sun to set color temperature */
	int temp = 0;
	if (elevation < SOLAR_CIVIL_TWILIGHT_ELEV) {
		temp = temp_night;
	} else if (elevation < SOLAR_DAYTIME_ELEV) {
		/* Interpolate */
		float a = (SOLAR_DAYTIME_ELEV - elevation) /
			(SOLAR_DAYTIME_ELEV - SOLAR_CIVIL_TWILIGHT_ELEV);
		temp = (1.0-a)*temp_day + a*temp_night;
	} else {
		temp = temp_day;
	}

	if (verbose) {
		printf("Set color temperature to %uK.\n", temp_day);
	}

	/* Set color temperature */
	r = colortemp_set_temperature(temp, gamma);
	if (r < 0) {
		fprintf(stderr, "Unable to set color temperature.\n");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
