/* gamma-dbus.c -- DBus gamma adjustment
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "redshift.h"
#include "gamma-dbus.h"


int
gamma_dbus_init(dbus_state_t *state)
{
	state->conn = NULL;
	state->use_session_bus = 0;
	state->serial = 0;
	return 0;
}

int
gamma_dbus_start(dbus_state_t *state)
{
	DBusError err;
	int ret;

	dbus_error_init(&err);

	// Connect to the dbus.
	state->conn = dbus_bus_get(state->use_session_bus ? DBUS_BUS_SESSION : DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, _("DBus connection error (%s)\n"), err.message);
		dbus_error_free(&err);
	}
	
	if (state->conn == NULL) {
		return -1;
	}
	
	// Register our name on the bus
	ret = dbus_bus_request_name(state->conn, GAMMA_DBUS_SOURCE_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, _("DBus name error (%s)\n"), err.message);
		dbus_error_free(&err);
	}

	if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		return -1;
	}	

	return 0;
}

void
gamma_dbus_restore(dbus_state_t *state)
{
}

void
gamma_dbus_free(dbus_state_t *state)
{
}

void
gamma_dbus_print_help(FILE *f)
{
	fputs(_("Select where to send signals to dbus.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: DBUS help output
	   left column must not be translated */
	fputs(_("  session=1\tSend messages to the session instead of system bus\n"), f);
	fputs("\n", f);
}

int
gamma_dbus_set_option(dbus_state_t *state, const char *key, const char *value)
{
	if (strcasecmp(key, "session") == 0) {
		state->use_session_bus = atoi(value);
		if (state->use_session_bus < 0 || state->use_session_bus > 1) {
			fprintf(stderr, _("session must be either 0 (use system bus) or 1 (use session bus)\n"));
			return -1;
		}
	} else {
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}

	return 0;
}

int
gamma_dbus_set_temperature(dbus_state_t *state, const color_setting_t *setting)
{
	DBusMessage* msg;
	DBusMessageIter args;

	// Create a signal
	msg = dbus_message_new_signal(GAMMA_DBUS_OBJECT_PATH, GAMMA_DBUS_INTERFACE_NAME, GAMMA_DBUS_SIGNAL_NAME);
	if (msg == NULL) {
		fprintf(stderr, _("DBus message is null\n"));
		return -1;
	}

	// Add argument
	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT16, &(setting->temperature))) {
		fprintf(stderr, _("Out of memory!\n"));
		return -1;
	}

	// Send the message
	if (!dbus_connection_send(state->conn, msg, &(state->serial))) {
		fprintf(stderr, _("Failed to send DBus message\n"));
		return -1;
	}

	dbus_connection_flush(state->conn);
	dbus_message_unref(msg);
	return 0;
}
