/* method.h -- Adjustment method selection and enumeration header
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
   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#ifndef REDSHIFT_METHOD_H
#define REDSHIFT_METHOD_H

#include <stdio.h>


/* Gamma adjustment method */
typedef int gamma_method_auto_func(const char *subsystem);
typedef int gamma_method_is_available_func(const char *subsystem);
typedef int gamma_method_get_priority_func(const char *subsystem);
typedef int gamma_method_init_func(void *state, const char *subsystem);
typedef int gamma_method_start_func(void *state);
typedef void gamma_method_print_help_func(FILE *f, const char *subsystem);

typedef struct {
	char *name;

	/* If evaluated to true, this method will be tried if none is explicitly chosen. */
	gamma_method_auto_func *autostart_test;
	/* If evaluated to true, this method is available and can be tried. */
	gamma_method_is_available_func *availability_test;
	/* Evaluates the system's perference for a subsystem. */
	gamma_method_get_priority_func *get_priority;

	/* Initialize state. Options can be set between init and start. */
	gamma_method_init_func *init;
	/* Allocate storage and make connections that depend on options. */
	gamma_method_start_func *start;

	/* Print help on options for this adjustment method. */
	gamma_method_print_help_func *print_help;

	int priority;
} gamma_method_t;



void print_method_list(void);
const gamma_method_t *find_gamma_method(const char *name);
int get_autostartable_methods(const gamma_method_t **methods, size_t* method_count);

#endif /* ! REDSHIFT_METHOD_H */

