/* colorramp.h -- color temperature calculation header
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

#ifndef REDSHIFT_COLORRAMP_H
#define REDSHIFT_COLORRAMP_H

#include <stdint.h>

void colorramp_fill(uint16_t *gamma_r, uint16_t *gamma_g, uint16_t *gamma_b,
		    int size, int temp, float brightness, const float gamma[3]);

#endif /* ! REDSHIFT_COLORRAMP_H */
