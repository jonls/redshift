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

   Copyright (c) 2010  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>


/* Source: http://www.vendian.org/mncharity/dir3/blackbody/
   Rescaled to make exactly 6500K equal to full intensity in all channels. */
static const float blackbody_color[] = {
	1.0000, 0.0425, 0.0000, /* 1000K */
	1.0000, 0.0668, 0.0000, /* 1100K */
	1.0000, 0.0911, 0.0000, /* 1200K */
	1.0000, 0.1149, 0.0000, /* ... */
	1.0000, 0.1380, 0.0000,
	1.0000, 0.1604, 0.0000,
	1.0000, 0.1819, 0.0000,
	1.0000, 0.2024, 0.0000,
	1.0000, 0.2220, 0.0000,
	1.0000, 0.2406, 0.0000,
	1.0000, 0.2630, 0.0062,
	1.0000, 0.2868, 0.0155,
	1.0000, 0.3102, 0.0261,
	1.0000, 0.3334, 0.0379,
	1.0000, 0.3562, 0.0508,
	1.0000, 0.3787, 0.0650,
	1.0000, 0.4008, 0.0802,
	1.0000, 0.4227, 0.0964,
	1.0000, 0.4442, 0.1136,
	1.0000, 0.4652, 0.1316,
	1.0000, 0.4859, 0.1505,
	1.0000, 0.5062, 0.1702,
	1.0000, 0.5262, 0.1907,
	1.0000, 0.5458, 0.2118,
	1.0000, 0.5650, 0.2335,
	1.0000, 0.5839, 0.2558,
	1.0000, 0.6023, 0.2786,
	1.0000, 0.6204, 0.3018,
	1.0000, 0.6382, 0.3255,
	1.0000, 0.6557, 0.3495,
	1.0000, 0.6727, 0.3739,
	1.0000, 0.6894, 0.3986,
	1.0000, 0.7058, 0.4234,
	1.0000, 0.7218, 0.4485,
	1.0000, 0.7375, 0.4738,
	1.0000, 0.7529, 0.4992,
	1.0000, 0.7679, 0.5247,
	1.0000, 0.7826, 0.5503,
	1.0000, 0.7970, 0.5760,
	1.0000, 0.8111, 0.6016,
	1.0000, 0.8250, 0.6272,
	1.0000, 0.8384, 0.6529,
	1.0000, 0.8517, 0.6785,
	1.0000, 0.8647, 0.7040,
	1.0000, 0.8773, 0.7294,
	1.0000, 0.8897, 0.7548,
	1.0000, 0.9019, 0.7801,
	1.0000, 0.9137, 0.8051,
	1.0000, 0.9254, 0.8301,
	1.0000, 0.9367, 0.8550,
	1.0000, 0.9478, 0.8795,
	1.0000, 0.9587, 0.9040,
	1.0000, 0.9694, 0.9283,
	1.0000, 0.9798, 0.9524,
	1.0000, 0.9900, 0.9763,
	1.0000, 1.0000, 1.0000, /* 6500K */
	0.9771, 0.9867, 1.0000,
	0.9554, 0.9740, 1.0000,
	0.9349, 0.9618, 1.0000,
	0.9154, 0.9500, 1.0000,
	0.8968, 0.9389, 1.0000,
	0.8792, 0.9282, 1.0000,
	0.8624, 0.9179, 1.0000,
	0.8465, 0.9080, 1.0000,
	0.8313, 0.8986, 1.0000,
	0.8167, 0.8895, 1.0000,
	0.8029, 0.8808, 1.0000,
	0.7896, 0.8724, 1.0000,
	0.7769, 0.8643, 1.0000,
	0.7648, 0.8565, 1.0000,
	0.7532, 0.8490, 1.0000,
	0.7420, 0.8418, 1.0000,
	0.7314, 0.8348, 1.0000,
	0.7212, 0.8281, 1.0000,
	0.7113, 0.8216, 1.0000,
	0.7018, 0.8153, 1.0000,
	0.6927, 0.8092, 1.0000,
	0.6839, 0.8032, 1.0000,
	0.6755, 0.7975, 1.0000,
	0.6674, 0.7921, 1.0000,
	0.6595, 0.7867, 1.0000,
	0.6520, 0.7816, 1.0000,
	0.6447, 0.7765, 1.0000,
	0.6376, 0.7717, 1.0000,
	0.6308, 0.7670, 1.0000,
	0.6242, 0.7623, 1.0000,
	0.6179, 0.7579, 1.0000,
	0.6117, 0.7536, 1.0000,
	0.6058, 0.7493, 1.0000,
	0.6000, 0.7453, 1.0000,
	0.5944, 0.7414, 1.0000 /* 10000K */
};


static void
interpolate_color(float a, const float *c1, const float *c2, float *c)
{
	c[0] = (1.0-a)*c1[0] + a*c2[0];
	c[1] = (1.0-a)*c1[1] + a*c2[1];
	c[2] = (1.0-a)*c1[2] + a*c2[2];
}

void
colorramp_fill(uint16_t *gamma_r, uint16_t *gamma_g, uint16_t *gamma_b,
	       int size, int temp, float gamma[3])
{
	/* Calculate white point */
	float white_point[3];
	float alpha = (temp % 100) / 100.0;
	int temp_index = ((temp - 1000) / 100)*3;
	interpolate_color(alpha, &blackbody_color[temp_index],
			  &blackbody_color[temp_index+3], white_point);

	for (int i = 0; i < size; i++) {
		gamma_r[i] = pow((float)i/size, 1.0/gamma[0]) *
			UINT16_MAX * white_point[0];
		gamma_g[i] = pow((float)i/size, 1.0/gamma[1]) *
			UINT16_MAX * white_point[1];
		gamma_b[i] = pow((float)i/size, 1.0/gamma[2]) *
			UINT16_MAX * white_point[2];
	}
}
