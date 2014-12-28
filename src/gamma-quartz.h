/* gamma-quartz.h -- Quartz (OSX) gamma adjustment header
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

   Copyright (c) 2014  Jon Lund Steffensen <jonlst@gmail.com>
*/

#ifndef REDSHIFT_GAMMA_QUARTZ_H
#define REDSHIFT_GAMMA_QUARTZ_H

#include "redshift.h"


typedef struct {
	int dummy;
} quartz_state_t;


int quartz_init(quartz_state_t *state);
int quartz_start(quartz_state_t *state);
void quartz_free(quartz_state_t *state);

void quartz_print_help(FILE *f);
int quartz_set_option(quartz_state_t *state, const char *key,
		      const char *value);

void quartz_restore(quartz_state_t *state);
int quartz_set_temperature(quartz_state_t *state,
			   const color_setting_t *setting);


#endif /* ! REDSHIFT_GAMMA_QUARTZ_H */
