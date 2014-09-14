/* gamma-libgamma.h -- libgamma gamma adjustment header
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

#ifndef REDSHIFT_GAMMA_LIBGAMMA_H
#define REDSHIFT_GAMMA_LIBGAMMA_H

#include "redshift.h"
#include "gamma-common.h"

#include <libgamma.h>


typedef struct gamma_libgamma_state_data {
	int method;
	libgamma_method_capabilities_t caps;
} gamma_libgamma_state_data_t;


int gamma_libgamma_auto(const char *subsystem);
int gamma_libgamma_is_available(const char *subsystem);

int gamma_libgamma_init(gamma_server_state_t *state, const char *subsystem);
int gamma_libgamma_start(gamma_server_state_t *state);

void gamma_libgamma_print_help(FILE *f, const char *subsystem);


#endif /* ! REDSHIFT_GAMMA_LIBGAMMA_H */
