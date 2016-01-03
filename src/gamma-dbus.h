/* gamma-dbus.h -- DBus gamma adjustment header
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

   Copyright (c) 2016 Michael Farrell <micolous+git@gmail.com>
*/

#ifndef REDSHIFT_GAMMA_DBUS_H
#define REDSHIFT_GAMMA_DBUS_H

#include <dbus/dbus.h>
#include "redshift.h"

#define GAMMA_DBUS_SOURCE_NAME "dk.jonls.redshift.dbus"
#define GAMMA_DBUS_OBJECT_PATH "/dk/jonls/Redshift"
#define GAMMA_DBUS_INTERFACE_NAME "dk.jonls.Redshift"
#define GAMMA_DBUS_SIGNAL_NAME "Temperature"

typedef struct {
	DBusConnection* conn;
	int use_session_bus;
	dbus_uint32_t serial;
} dbus_state_t;

int gamma_dbus_init(dbus_state_t *state);
int gamma_dbus_start(dbus_state_t *state);
void gamma_dbus_free(dbus_state_t *state);

void gamma_dbus_print_help(FILE *f);
int gamma_dbus_set_option(dbus_state_t *state, const char *key, const char *value);

void gamma_dbus_restore(dbus_state_t *state);
int gamma_dbus_set_temperature(dbus_state_t *state,
				const color_setting_t *setting);


#endif /* ! REDSHIFT_GAMMA_DBUS_H */
