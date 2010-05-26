/* redshift.h -- Main program header
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

#ifndef _REDSHIFT_REDSHIFT_H
#define _REDSHIFT_REDSHIFT_H

#include <stdio.h>
#include <stdlib.h>


/* Gamma adjustment method */
typedef int gamma_method_init_func(void *state, char *args);
typedef void gamma_method_free_func(void *state);
typedef void gamma_method_print_help_func(FILE *f);
typedef void gamma_method_restore_func(void *state);
typedef int gamma_method_set_temperature_func(void *state, int temp,
					      float gamma[3]);

typedef struct {
	char *name;
	gamma_method_init_func *init;
	gamma_method_free_func *free;
	gamma_method_print_help_func *print_help;
	gamma_method_restore_func *restore;
	gamma_method_set_temperature_func *set_temperature;
} gamma_method_t;


/* Location provider */
typedef int location_provider_init_func(void *state, char *args);
typedef void location_provider_free_func(void *state);
typedef void location_provider_print_help_func(FILE *f);
typedef int location_provider_get_location_func(void *state, float *lat,
						float *lon);

typedef struct {
	char *name;
	location_provider_init_func *init;
	location_provider_free_func *free;
	location_provider_print_help_func *print_help;
	location_provider_get_location_func *get_location;
} location_provider_t;


#endif /* ! _REDSHIFT_REDSHIFT_H */
