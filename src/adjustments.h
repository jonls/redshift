/* adjustments.h -- Adjustment constants and data structures header
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

   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#ifndef REDSHIFT_ADJUSTMENTS_H
#define REDSHIFT_ADJUSTMENTS_H

#include <stdint.h>
#include <unistd.h>



/* Bounds for parameters. */
#define MIN_TEMP   1000
#define MAX_TEMP  25000
#ifndef MIN_BRIGHTNESS
#  define MIN_BRIGHTNESS  0.1f
#endif
#if !defined(MAX_BRIGHTNESS) && !defined(NO_MAX_BRIGHTNESS)
#  define MAX_BRIGHTNESS  1.0f
#endif
#ifndef MIN_GAMMA
#  define MIN_GAMMA  0.1f
#endif
#if !defined(MAX_GAMMA) && !defined(NO_MAX_GAMMA)
#  define MAX_GAMMA  10.0f
#endif

/* Default values for parameters. */
#define DEFAULT_DAY_TEMP    5500
#define DEFAULT_NIGHT_TEMP  3500
#define DEFAULT_BRIGHTNESS   1.0f
#define DEFAULT_GAMMA        1.0f

/* The color temperature when no adjustment is applied. */
#define NEUTRAL_TEMP  6500



/* Gamma ramp trio. */
typedef struct {
	/* The number of stops in each ramp. */
	size_t red_size;
	size_t green_size;
	size_t blue_size;
	/* The actual ramps. */
	uint16_t *red;
	uint16_t *green;
	uint16_t *blue;
} gamma_ramps_t;


/* Color adjustment settings. */
typedef struct {
	/* The monitor's gamma correction. */
	float gamma_correction[3];
	/* Lookup table for monitor calibration. */
	gamma_ramps_t *lut_calibration;
	/* Adjustments.
	   The gamma is only one value, rather than
	   three becuase it is not an correction,
	   but rather an adjustment as suggest in
	   <https://github.com/jonls/redshift/issues/10>.
	   This is included for performance: it takes
	   less work for Redshift to multiply the gamma
	   values than for a front-ent to create or
	   modify a lookup table. */
	float gamma;
	float brightness;
	float temperature;
	/* Lookup table with adjustments, before
	   and after gamma–brightness–temperature,
	   but both before gamma correction and
	   LUT calibration. */
	gamma_ramps_t *lut_pre;
	gamma_ramps_t *lut_post;
} gamma_settings_t;



#endif
