/* colorramp.c -- color temperature calculation source
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
   Copyright (c) 2013  Ingo Thies <ithies@astro.uni-bonn.de>
*/

#include <stdint.h>
#include <math.h>
#include "colorramp-common.h"

void
colorramp_fill(uint16_t *gamma_r, uint16_t *gamma_g, uint16_t *gamma_b,
	       int size, int temp, float brightness, float gamma[3])
{
	/* Approximate white point */
	float white_point[3];
	float alpha = (temp % 100) / 100.0;
	int temp_index = ((temp - 1000) / 100)*3;
	interpolate_color(alpha, &blackbody_color[temp_index],
			  &blackbody_color[temp_index+3], white_point);

	for (int i = 0; i < size; i++) {
		gamma_r[i] = pow((float)i/size, 1.0/gamma[0]) *
			(UINT16_MAX+1) * brightness * white_point[0];
		gamma_g[i] = pow((float)i/size, 1.0/gamma[1]) *
			(UINT16_MAX+1) * brightness * white_point[1];
		gamma_b[i] = pow((float)i/size, 1.0/gamma[2]) *
			(UINT16_MAX+1) * brightness * white_point[2];
	}
}
