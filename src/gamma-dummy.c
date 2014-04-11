/* gamma-dummy.c -- No-op gamma adjustment
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

#include "gamma-dummy.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif


int
gamma_dummy_auto()
{
	return 1;
}


static int
gamma_dummy_set_ramps(gamma_server_state_t *state, gamma_crtc_state_t *crtc, gamma_ramps_t ramps)
{
	(void) state;
	(void) crtc;
	(void) ramps;
	return 0;
}

static int
gamma_dummy_set_option(gamma_server_state_t *state, const char *key, char *value, ssize_t section)
{
	(void) state;
	(void) key;
	(void) value;
	(void) section;
	return 1;
}


int
gamma_dummy_init(gamma_server_state_t *state)
{
	int r;
	r = gamma_init(state);
	if (r != 0) return r;
	state->data = NULL;
	state->set_ramps = gamma_dummy_set_ramps;
	state->set_option = gamma_dummy_set_option;
	return 0;
}

int
gamma_dummy_start(gamma_server_state_t *state)
{
	(void) state;
	fputs(_("WARNING: Using dummy gamma method! "
		"Display will not be affected by this gamma method.\n"),
	      stderr);
	return 0;
}

void
gamma_dummy_print_help(FILE *f)
{
	fputs(_("Does not affect the display but prints "
		"the color temperature to the terminal.\n"),
	      f);
	fputs("\n", f);
}
