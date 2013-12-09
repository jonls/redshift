/* colorramp-common.h -- colorramp common files header
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
*/

#ifndef _REDSHIFT_COLORRAMP_COMMON_H
#define _REDSHIFT_COLORRAMP_COMMON_H

void interpolate_color(float a, const float *c1, const float *c2, float *c);
extern const float blackbody_color[];

#endif /* ! _REDSHIFT_COLORRAMP_COMMON_H */
