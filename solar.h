/* solar.h */

#ifndef _SOLAR_H
#define _SOLAR_H

#include <time.h>

/* Model of atmospheric refraction near horizon (in degrees). */
#define SOLAR_ATM_REFRAC  0.833

#define SOLAR_ASTRO_TWILIGHT_ELEV   -18.0
#define SOLAR_NAUT_TWILIGHT_ELEV    -12.0
#define SOLAR_CIVIL_TWILIGHT_ELEV    -6.0
#define SOLAR_DAYTIME_ELEV           (0.0 - SOLAR_ATM_REFRAC)

typedef enum {
	SOLAR_TIME_NOON = 0,
	SOLAR_TIME_MIDNIGHT,
	SOLAR_TIME_ASTRO_DAWN,
	SOLAR_TIME_NAUT_DAWN,
	SOLAR_TIME_CIVIL_DAWN,
	SOLAR_TIME_SUNRISE,
	SOLAR_TIME_SUNSET,
	SOLAR_TIME_CIVIL_DUSK,
	SOLAR_TIME_NAUT_DUSK,
	SOLAR_TIME_ASTRO_DUSK,
	SOLAR_TIME_MAX
} solar_time_t;

double solar_elevation(time_t t, double lat, double lon);
void solar_table_fill(time_t date, double lat, double lon, time_t *table);

#endif /* ! _SOLAR_H */
