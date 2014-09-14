/* method.c -- Adjustment method selection and enumeration
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "method.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif


#include "gamma-libgamma.h"


/* Gamma adjustment method structs */
#define __method(NAME, METHOD)							\
	{									\
		NAME, 								\
		(gamma_method_auto_func *)            METHOD##_auto,		\
		(gamma_method_is_available_func *)    METHOD##_is_available,	\
		(gamma_method_get_priority_func *)    METHOD##_get_priority,	\
		(gamma_method_init_func *)            METHOD##_init,		\
		(gamma_method_start_func *)           METHOD##_start,		\
		(gamma_method_print_help_func *)      METHOD##_print_help,	\
		0								\
	}
static gamma_method_t gamma_methods[] = {
#define X(M)  __method(M,   gamma_libgamma),
	LIST_GAMMA_LIBGAMMA_SUBSYSTEMS
#undef X
	{ NULL }
};
#undef __method


static gamma_method_t sorted_gamma_methods[
#define X(M)  1 +
	LIST_GAMMA_LIBGAMMA_SUBSYSTEMS
#undef X
	0];



void
print_method_list()
{
	fputs(_("Available adjustment methods:\n"), stdout);
	for (size_t i = 0; gamma_methods[i].name != NULL; i++) {
		const gamma_method_t *m = &gamma_methods[i];
		if (!m->availability_test(m->name))
			continue;
		printf("  %s\n", m->name);
	}

	fputs("\n", stdout);
	fputs(_("Specify colon-separated options with"
		" `-m METHOD OPTIONS'.\n"), stdout);
	/* TRANSLATORS: `help' must not be translated. */
	fputs(_("Try `-m METHOD help' for help.\n"), stdout);
}


const gamma_method_t *
find_gamma_method(const char *name)
{
	const gamma_method_t *method = NULL;
	int r;

	for (size_t i = 0; gamma_methods[i].name != NULL; i++) {
		const gamma_method_t *m = &gamma_methods[i];
		r = m->availability_test(m->name);
		if (r < 0)    exit(EXIT_FAILURE);
		else if (!r)  continue;
		if (strcasecmp(name, m->name) == 0) {
		        method = m;
			break;
		}
	}

	return method;
}


static int method_cmp(const void *a_, const void *b_)
{
	const gamma_method_t *a = a_;
	const gamma_method_t *b = b_;
	return a->priority - b->priority; /* low priority should go first. */
}


int
get_autostartable_methods(const gamma_method_t **methods, size_t* method_count)
{
	size_t n = 0;
	int r;

	for (size_t i = 0; gamma_methods[i].name != NULL; i++) {
		gamma_method_t *m = &gamma_methods[i];

		r = m->autostart_test(m->name);
		if (r < 0)    exit(EXIT_FAILURE);
		else if (!r)  continue;

		r = m->availability_test(m->name);
		if (r < 0)    exit(EXIT_FAILURE);
		else if (!r)  continue;

		r = m->get_priority(m->name);
		if (r < 0)    exit(EXIT_FAILURE);
		m->priority = r;

		sorted_gamma_methods[n++] = *m;
	}

	qsort(sorted_gamma_methods, n, sizeof(gamma_method_t), method_cmp);

	*methods = sorted_gamma_methods;
	*method_count = n;
	return 0;
}

