/* gamma-print-binary.h -- Binary output gamma adjustment header
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

   Copyright (c) 2016  Mattias Andrée <maandree@member.fsf.org>
*/

#ifndef REDSHIFT_GAMMA_PRINT_BINARY_H
#define REDSHIFT_GAMMA_PRINT_BINARY_H

#include <stddef.h>
#include <stdio.h>

#include "redshift.h"


typedef struct print_binary_state {
	int fd; /* Flexible because some other parts use stdout. */
	int type;
	int size;
	void *gamma;
	void *r_gamma;
	void *g_gamma;
	void *b_gamma;
	void *pure;
	size_t total_size;
	void (*colorramp_fill_function)(void *gamma_r, void *gamma_g, void *gamma_b,
					int size, const color_setting_t *setting);
} print_binary_state_t;


int print_binary_init(print_binary_state_t *state);
int print_binary_start(print_binary_state_t *state);
void print_binary_free(print_binary_state_t *state);

void print_binary_print_help(FILE *f);
int print_binary_set_option(print_binary_state_t *state,
			    const char *key, const char *value);

void print_binary_restore(print_binary_state_t *state);
int print_binary_set_temperature(print_binary_state_t *state,
				 const color_setting_t *setting);


#endif /* ! REDSHIFT_GAMMA_PRINT_BINARY_H */
