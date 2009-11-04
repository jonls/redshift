/* redshift.c */
/* Copyright (c) 2009, Jon Lund Steffensen <jonlst@gmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>

#include "solar.h"
#include "colortemp.h"

#define MIN_LAT   -90.0
#define MAX_LAT    90.0
#define MIN_LON  -180.0
#define MAX_LON   180.0
#define MIN_TEMP   1000
#define MAX_TEMP  10000


#define USAGE  \
	"Usage: %s LAT LON DAY-TEMP NIGHT-TEMP [GAMMA]\n"

#define DEG_CHAR  0xb0


static void
printtime(time_t time)
{
	char s[64];
	strftime(s, 64, "%a, %d %b %Y %H:%M:%S %z", localtime(&time));
	printf("%s\n", s);
}

int
main(int argc, char *argv[])
{
	/* Check extensions needed for color temperature adjustment. */
	int r = colortemp_check_extension();
	if (r < 0) {
		fprintf(stderr, "Unable to set color temperature.\n");
		exit(EXIT_FAILURE);
	}

	/* Init locale for special symbols */
	char *loc = setlocale(LC_CTYPE, "");
	if (loc == NULL) {
		fprintf(stderr, "Unable to set locale.\n");
		exit(EXIT_FAILURE);
	}

	/* Load arguments */
	if (argc < 5) {
		printf(USAGE, argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Latitude */
	float lat = atof(argv[1]);
	if (lat < MIN_LAT || lat > MAX_LAT) {
		fprintf(stderr,
			"Latitude must be between %.1f%lc and %.1f%lc\n",
			MIN_LAT, DEG_CHAR, MAX_LAT, DEG_CHAR);
		exit(EXIT_FAILURE);
	}

	/* Longitude */
	float lon = atof(argv[2]);
	if (lon < MIN_LON || lon > MAX_LON) {
		fprintf(stderr,
			"Longitude must be between %.1f%lc and %.1f%lc\n",
			MIN_LON, DEG_CHAR, MAX_LON, DEG_CHAR);
		exit(EXIT_FAILURE);
	}

	/* Color temperature at daytime */
	int temp_day = atoi(argv[3]);
	if (temp_day < MIN_TEMP || temp_day >= MAX_TEMP) {
		fprintf(stderr, "Temperature must be between %uK and %uK\n",
			MIN_TEMP, MAX_TEMP);
		exit(EXIT_FAILURE);
	}

	/* Color temperature at night */
	int temp_night = atoi(argv[4]);
	if (temp_night < MIN_TEMP || temp_night >= MAX_TEMP) {
		fprintf(stderr, "Temperature must be between %uK and %uK\n",
			MIN_TEMP, MAX_TEMP);
		exit(EXIT_FAILURE);
	}

	/* Gamma value */
	float gamma = 1.0;
	if (argc > 5) gamma = atof(argv[5]);

	/* Current angular elevation of the sun */
	time_t now = time(NULL);
	double elevation = solar_elevation(now, lat, lon);
	printf("Solar elevation is %f\n", elevation);

	/* Use elevation of sun to set color temperature */
	int temp = 0;
	if (elevation < SOLAR_CIVIL_TWILIGHT_ELEV) {
		temp = temp_night;
		printf("Set color temperature to %uK\n", temp_night);
	} else if (elevation < SOLAR_DAYTIME_ELEV) {
		float a = (SOLAR_DAYTIME_ELEV - elevation) /
			(SOLAR_DAYTIME_ELEV - SOLAR_CIVIL_TWILIGHT_ELEV);
		temp = (1.0-a)*temp_day + a*temp_night;
		printf("Interpolated (%f) temperature is %uK\n", a, temp);
	} else {
		temp = temp_day;
		printf("Set color temperature to %uK\n", temp_day);
	}

	/* Set color temperature */
	r = colortemp_set_temperature(temp, gamma);
	if (r < 0) {
		fprintf(stderr, "Unable to set color temperature.\n");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
