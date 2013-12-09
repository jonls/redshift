/* gamma-dummy.h -- No-op gamma adjustment header
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

#ifndef _REDSHIFT_GAMMA_DUMMY_H
#define _REDSHIFT_GAMMA_DUMMY_H

#include "redshift.h"


int gamma_dummy_init(void *state);
int gamma_dummy_start(void *state);
void gamma_dummy_free(void *state);

void gamma_dummy_print_help(FILE *f);
int gamma_dummy_set_option(void *state, const char *key, const char *value);

void gamma_dummy_restore(void *state);
int gamma_dummy_set_temperature(void *state, int temp, float brightness,
				const float gamma[3]);


#endif /* ! _REDSHIFT_GAMMA_DUMMY_H */
