/* gamma-print-binary.c -- Binary output gamma adjustment source
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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "gamma-print-binary.h"
#include "colorramp.h"


typedef void colorramp_fill_func(void *, void *, void *, int, const color_setting_t *);


#define FILL_PURE(TYPE, MAX)\
	do {\
		TYPE *r_gamma = state->pure;\
		TYPE *g_gamma = r_gamma + state->size;\
		TYPE *b_gamma = g_gamma + state->size;\
		for (int i = 0; i < state->size; i++) {\
			TYPE value = (double)i / (state->size - 1) * MAX;\
			r_gamma[i] = value;\
			g_gamma[i] = value;\
			b_gamma[i] = value;\
		}\
	} while (0)


static void fill_pure_u16(print_binary_state_t *state) { FILL_PURE(uint16_t, UINT16_MAX); }
static void fill_pure_f(print_binary_state_t *state)   { FILL_PURE(float, 1); }


static int
print_gamma(print_binary_state_t *state)
{
	size_t p = 0, n = state->total_size;
	ssize_t r;

	while (p < n) {
		r = write(state->fd, (char *)state->gamma + p, n - p);
		if (r < 0) {
			perror("write");
			return -1;
		}
		p += (size_t)r;
	}

	return 0;
}


int
print_binary_init(print_binary_state_t *state)
{
	state->fd = STDOUT_FILENO;
	state->type = 16;
	state->size = 256;
	state->gamma = NULL;

	return 0;
}

int
print_binary_start(print_binary_state_t *state)
{
	void (*fill_pure_function)(print_binary_state_t *) = NULL;
	size_t elemsize = 0;

	switch (state->type) {
	case 16:
		state->colorramp_fill_function = (colorramp_fill_func *)colorramp_fill;
		fill_pure_function = fill_pure_u16;
		elemsize = sizeof(uint16_t);
		break;
	case -1:
		state->colorramp_fill_function = (colorramp_fill_func *)colorramp_fill_float;
		fill_pure_function = fill_pure_f;
		elemsize = sizeof(float);
		break;
	default:
		abort();
	}

	state->total_size = 3 * elemsize * (size_t)state->size;

	state->gamma = calloc((size_t)state->size, 3 * elemsize);
	if (state->gamma == NULL) {
		perror("calloc");
		return -1;
	}
	state->r_gamma = (char *)state->gamma   + state->size * elemsize;
	state->g_gamma = (char *)state->r_gamma + state->size * elemsize;
	state->b_gamma = (char *)state->g_gamma + state->size * elemsize;

	state->pure = malloc(state->total_size);
	if (state->pure == NULL) {
		perror("malloc");
		return -1;
	}
	fill_pure_function(state);

	return 0;
}

void
print_binary_restore(print_binary_state_t *state)
{
	if (state->gamma != NULL) {
		memcpy(state->gamma, state->pure, state->total_size);
		print_gamma(state);
	}
}

void
print_binary_free(print_binary_state_t *state)
{
	free(state->gamma);
	state->gamma = NULL;
	free(state->pure);
	state->pure = NULL;
}

void
print_binary_print_help(FILE *f)
{
	fputs(_("Print gamma ramps to stdout in binary format.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: print-binary help output
	   left column must not be translated */
	fputs(_("  fd=FD    \tFile descriptor for the output\n"
		"  size=N   \tThe number of stops in the ramps\n"
		"  type=TYPE\t`16' for unsigned 16-bit integer ramps\n"
		"           \t`f' for single-precision floating-point ramps\n"), f);
	fputs("\n", f);
}

int
print_binary_set_option(print_binary_state_t *state, const char *key, const char *value)
{
	if (!strcasecmp(key, "fd")) {
		state->fd = atoi(value);
	} else if (!strcasecmp(key, "size")) {
		char *end;
		errno = 0;
		state->size = strtol(value, &end, 10);
		if (errno || *end || state->size < 1 || state->size > INT_MAX) {
			fprintf(stderr,
				_("Ramp size must be an integer between 1 and %i, inclusively\n"),
				INT_MAX);
			return -1;
		}
	} else if (!strcasecmp(key, "type")) {
		if (!strcmp(value, "16")) {
			state->type = 16;
		} else if (!strcmp(value, "f")) {
			state->type = -1;
		} else {
			fprintf(stderr, _("Ramp stop value type must `16' or `f'\n"));
			return -1;
		}
	} else {
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}

	return 0;
}

int
print_binary_set_temperature(print_binary_state_t *state, const color_setting_t *setting)
{
	memcpy(state->gamma, state->pure, state->total_size);
	state->colorramp_fill_function(state->r_gamma, state->g_gamma, state->b_gamma,
				       state->size, setting);
	return print_gamma(state);
}
