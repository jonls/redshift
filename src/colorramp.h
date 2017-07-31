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

   Copyright (c) 2010-2014  Jon Lund Steffensen <jonlst@gmail.com>
*/

#ifndef REDSHIFT_COLORRAMP_H
#define REDSHIFT_COLORRAMP_H

#include <stdint.h>

#include "redshift.h"

#define LIST_RAMPS_STOP_VALUE_TYPES\
	X(u8, uint8_t, UINT8_MAX + 1ULL, UINT8_MAX, 8)\
	X(u16, uint16_t, UINT16_MAX + 1ULL, UINT16_MAX, 16)\
	X(u32, uint32_t, UINT32_MAX + 1ULL, UINT32_MAX, 32)\
	X(u64, uint64_t, UINT64_MAX, UINT64_MAX, 64)\
	X(float, float, 1, 1, -1)\
	X(double, double, 1, 1, -2)

#define X(SUFFIX, TYPE, MAX, TRUE_MAX, DEPTH)\
	void colorramp_fill_##SUFFIX(TYPE *gamma_r, TYPE *gamma_g, TYPE *gamma_b,\
				     size_t size_r, size_t size_g, size_t size_b,\
				     const color_setting_t *setting);
LIST_RAMPS_STOP_VALUE_TYPES
#undef X

#endif /* ! REDSHIFT_COLORRAMP_H */
