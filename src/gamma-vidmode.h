/* gamma-vidmode.h -- X VidMode gamma adjustment header
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
   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#ifndef REDSHIFT_GAMMA_VIDMODE_H
#define REDSHIFT_GAMMA_VIDMODE_H

#include "gamma-common.h"

#include <stdio.h>
#include <stdint.h>


int vidmode_init(gamma_server_state_t *state);
int vidmode_start(gamma_server_state_t *state);

void vidmode_print_help(FILE *f);


#endif /* ! REDSHIFT_GAMMA_VIDMODE_H */
