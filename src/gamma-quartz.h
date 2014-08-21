/* gamma-quartz.h -- Mac OS X Quartz gamma adjustment header
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

#ifndef REDSHIFT_GAMMA_QUARTZ_H
#define REDSHIFT_GAMMA_QUARTZ_H

#include "gamma-common.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef FAKE_QUARTZ
#  include "fake-quartz.h"
#else
#  include <CoreGraphics/CGDirectDisplay.h>
#  define close_fake_quartz() /* for compatibility with "fake-quartz.h" */
#endif


int quartz_init(gamma_server_state_t *state);
int quartz_start(gamma_server_state_t *state);

void quartz_print_help(FILE *f);


/* TODO: It should be possible to use CGDisplayRestoreColorSyncSettings. */


#endif /* ! REDSHIFT_GAMMA_QUARTZ_H */

