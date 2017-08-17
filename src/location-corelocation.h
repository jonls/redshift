/* location-corelocation.h -- CoreLocation (OSX) location provider header
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

   Copyright (c) 2014-2017  Jon Lund Steffense <jonlst@gmail.com>
*/

#ifndef REDSHIFT_LOCATION_CORELOCATION_H
#define REDSHIFT_LOCATION_CORELOCATION_H

#include <stdio.h>

#include "redshift.h"

typedef struct location_corelocation_private location_corelocation_private_t;

typedef struct {
	location_corelocation_private_t *private;
	int pipe_fd_read;
	int pipe_fd_write;
	int available;
	int error;
	float latitude;
	float longitude;
} location_corelocation_state_t;


int location_corelocation_init(location_corelocation_state_t *state);
int location_corelocation_start(location_corelocation_state_t *state);
void location_corelocation_free(location_corelocation_state_t *state);

void location_corelocation_print_help(FILE *f);
int location_corelocation_set_option(
	location_corelocation_state_t *state,
	const char *key, const char *value);

int location_corelocation_get_fd(
	location_corelocation_state_t *state);
int location_corelocation_handle(
	location_corelocation_state_t *state,
	location_t *location, int *available);


#endif /* ! REDSHIFT_LOCATION_CORELOCATION_H */
