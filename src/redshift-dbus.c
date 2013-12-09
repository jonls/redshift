/* redshift-dbus.c -- DBus server for Redshift
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
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include "redshift.h"
#include "solar.h"


#ifdef ENABLE_RANDR
# include "gamma-randr.h"
#endif

#ifdef ENABLE_VIDMODE
# include "gamma-vidmode.h"
#endif

#include "gamma-dummy.h"

#ifdef ENABLE_GEOCLUE
# include "location-geoclue.h"
#endif


/* Union of state data for gamma adjustment methods */
union {
#ifdef ENABLE_RANDR
	randr_state_t randr;
#endif
#ifdef ENABLE_VIDMODE
	vidmode_state_t vidmode;
#endif
} gamma_state;


/* Gamma adjustment method structs */
static const gamma_method_t gamma_methods[] = {
#ifdef ENABLE_RANDR
	{
		"randr", 1,
		(gamma_method_init_func *)randr_init,
		(gamma_method_start_func *)randr_start,
		(gamma_method_free_func *)randr_free,
		(gamma_method_print_help_func *)randr_print_help,
		(gamma_method_set_option_func *)randr_set_option,
		(gamma_method_restore_func *)randr_restore,
		(gamma_method_set_temperature_func *)randr_set_temperature
	},
#endif
#ifdef ENABLE_VIDMODE
	{
		"vidmode", 1,
		(gamma_method_init_func *)vidmode_init,
		(gamma_method_start_func *)vidmode_start,
		(gamma_method_free_func *)vidmode_free,
		(gamma_method_print_help_func *)vidmode_print_help,
		(gamma_method_set_option_func *)vidmode_set_option,
		(gamma_method_restore_func *)vidmode_restore,
		(gamma_method_set_temperature_func *)vidmode_set_temperature
	},
#endif
	{
		"dummy", 0,
		(gamma_method_init_func *)gamma_dummy_init,
		(gamma_method_start_func *)gamma_dummy_start,
		(gamma_method_free_func *)gamma_dummy_free,
		(gamma_method_print_help_func *)gamma_dummy_print_help,
		(gamma_method_set_option_func *)gamma_dummy_set_option,
		(gamma_method_restore_func *)gamma_dummy_restore,
		(gamma_method_set_temperature_func *)gamma_dummy_set_temperature
	},
	{ NULL }
};

static const gamma_method_t *current_method = NULL;


/* DBus names */
#define REDSHIFT_BUS_NAME        "dk.jonls.redshift.Redshift"
#define REDSHIFT_OBJECT_PATH     "/dk/jonls/redshift/Redshift"
#define REDSHIFT_INTERFACE_NAME  "dk.jonls.redshift.Redshift"

/* Parameter bounds */
#define LAT_MIN   -90.0
#define LAT_MAX    90.0
#define LON_MIN  -180.0
#define LON_MAX   180.0
#define TEMP_MIN   1000
#define TEMP_MAX  25000

/* The color temperature when no adjustment is applied. */
#define TEMP_NEUTRAL  6500

/* Default values for parameters. */
#define DEFAULT_DAY_TEMP    TEMP_NEUTRAL
#define DEFAULT_NIGHT_TEMP  3500
#define DEFAULT_BRIGHTNESS   1.0
#define DEFAULT_GAMMA        1.0

/* Angular elevation of the sun at which the color temperature
   transition period starts and ends (in degress).
   Transition during twilight, and while the sun is lower than
   3.0 degrees above the horizon. */
#define TRANSITION_LOW     SOLAR_CIVIL_TWILIGHT_ELEV
#define TRANSITION_HIGH    3.0

/* Periods of day */
typedef enum {
	PERIOD_NONE = 0,
	PERIOD_DAY,
	PERIOD_NIGHT,
	PERIOD_TRANSITION
} period_t;

static const gchar *period_names[] = {
	"None", "Day", "Night", "Transition"
};


static GDBusNodeInfo *introspection_data = NULL;

/* Cookies for programs wanting to interact though
   the DBus service. */
static GHashTable *cookies = NULL;
static gint32 next_cookie = 1;

/* Set of clients inhibiting the program. */
static GHashTable *inhibitors = NULL;
static gboolean inhibited = FALSE;

/* Solar elevation */
static gdouble elevation = 0.0;
static period_t period = PERIOD_NONE;

/* Location */
static gdouble latitude = 0.0;
static gdouble longitude = 0.0;

/* Temperature bounds */
static guint temp_day = DEFAULT_DAY_TEMP;
static guint temp_night = DEFAULT_NIGHT_TEMP;

/* Current temperature */
static guint temperature = 0;
static guint temp_now = TEMP_NEUTRAL;

/* Short transition parameters */
static guint trans_timer = 0;
static guint trans_temp_start = 0;
static guint trans_length = 0;
static guint trans_time = 0;

/* Forced temperature: 2-layered, so that an external program
   can drive the temperature, and at the same time an external
   program can demo temperatures as part of the user interface
   (priority mode). */
#define FORCED_TEMP_LAYERS  2
static gint32 forced_temp_cookie[FORCED_TEMP_LAYERS] = {0};
static guint forced_temp[FORCED_TEMP_LAYERS] = {0};

/* Forced location */
static gint32 forced_location_cookie = 0;
static gdouble forced_lat = 0.0;
static gdouble forced_lon = 0.0;

/* Screen update timer */
static guint screen_update_timer = 0;


/* DBus service definition */
static const gchar introspection_xml[] =
	"<node>"
	" <interface name='dk.jonls.redshift.Redshift'>"
	"  <method name='AcquireCookie'>"
	"   <arg type='s' name='program' direction='in'/>"
	"   <arg type='i' name='cookie' direction='out'/>"
	"  </method>"
	"  <method name='ReleaseCookie'>"
	"   <arg type='i' name='cookie' direction='in'/>"
	"  </method>"
	"  <method name='Inhibit'>"
	"   <arg type='i' name='cookie' direction='in'/>"
	"  </method>"
	"  <method name='Uninhibit'>"
	"   <arg type='i' name='cookie' direction='in'/>"
	"  </method>"
	"  <method name='EnforceTemperature'>"
	"   <arg type='i' name='cookie' direction='in'/>"
	"   <arg type='u' name='temperature' direction='in'/>"
	"   <arg type='b' name='priority' direction='in'/>"
	"  </method>"
	"  <method name='UnenforceTemperature'>"
	"   <arg type='i' name='cookie' direction='in'/>"
	"   <arg type='b' name='priority' direction='in'/>"
	"  </method>"
	"  <method name='EnforceLocation'>"
	"   <arg type='i' name='cookie' direction='in'/>"
	"   <arg type='d' name='latitude' direction='in'/>"
	"   <arg type='d' name='longitude' direction='in'/>"
	"  </method>"
	"  <method name='UnenforceLocation'>"
	"   <arg type='i' name='cookie' direction='in'/>"
	"  </method>"
	"  <method name='GetElevation'>"
	"   <arg type='d' name='elevation' direction='out'/>"
	"  </method>"
	"  <property type='b' name='Inhibited' access='read'/>"
	"  <property type='s' name='Period' access='read'/>"
	"  <property type='u' name='Temperature' access='read'/>"
	"  <property type='d' name='CurrentLatitude' access='read'/>"
	"  <property type='d' name='CurrentLongitude' access='read'/>"
	"  <property type='u' name='TemperatureDay' access='readwrite'/>"
	"  <property type='u' name='TemperatureNight' access='readwrite'/>"
	" </interface>"
	"</node>";


/* Update elevation from location and time. */
static void
update_elevation()
{
	gdouble time = g_get_real_time() / 1000000.0;

	/* Check for forced location */
	gdouble lat = latitude;
	gdouble lon = longitude;
	if (forced_location_cookie != 0) {
		lat = forced_lat;
		lon = forced_lon;
	}

	elevation = solar_elevation(time, lat, lon);

	g_printf("Location: %f, %f\n", lat, lon);
	g_printf("Elevation: %f\n", elevation);
}

/* Update temperature from elevation */
static void
update_temperature()
{
	/* Calculate temperature according to elevation */
	if (elevation < TRANSITION_LOW) {
		temperature = temp_night;
	} else if (elevation < TRANSITION_HIGH) {
		float a = (TRANSITION_LOW - elevation) /
			(TRANSITION_LOW - TRANSITION_HIGH);
		temperature = (1.0-a)*temp_night + a*temp_day;
	} else {
		temperature = temp_day;
	}
}


/* Timer callback to update short transitions */
static gboolean
short_transition_update_cb(gpointer data)
{
	trans_time += 1;
	gfloat a = trans_time/(gfloat)trans_length;
	guint temp = (1.0-a)*trans_temp_start + a*temperature;

	const gfloat gamma[] = {1.0, 1.0, 1.0};
	if (current_method != NULL) {
		current_method->set_temperature(&gamma_state, temp,
						1.0, gamma);
	}
	temp_now = temp;

	if (trans_time >= trans_length) {
		trans_time = 0;
		trans_length = 0;
		return FALSE;
	}

	return TRUE;
}

/* Timer callback to update the screen */
static gboolean
screen_update_cb(gpointer data)
{
	GDBusConnection *conn = G_DBUS_CONNECTION(data);

	/* Update elevation from location */
	update_elevation();

	gboolean prev_inhibit = inhibited;
	guint prev_temp = temperature;
	period_t prev_period = period;

	/* Calculate period */
	if (elevation < TRANSITION_LOW) {
		period = PERIOD_NIGHT;
	} else if (elevation < TRANSITION_HIGH) {
		period = PERIOD_TRANSITION;
	} else {
		period = PERIOD_DAY;
	}

	/* Check for inhibition */
	inhibited = g_hash_table_size(inhibitors) > 0;

	if (inhibited) {
		temperature = TEMP_NEUTRAL;
	} else {
		/* Check for forced temperature */
		int forced_index = -1;
		for (int i = FORCED_TEMP_LAYERS-1; i >= 0; i--) {
			if (forced_temp_cookie[i] != 0) {
				forced_index = i;
				break;
			}
		}

		if (forced_index < 0) {
			update_temperature();
		} else {
			temperature = forced_temp[forced_index];
		}
	}

	g_printf("Temperature: %u\n", temperature);

	/* Signal if temperature has changed */
	if (prev_temp != temperature ||
	    prev_inhibit != inhibited ||
	    prev_period != period) {
		GError *local_error = NULL;
		GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
		if (prev_temp != temperature) {
			g_variant_builder_add(builder, "{sv}",
					      "Temperature", g_variant_new_uint32(temperature));
		}
		if (prev_inhibit != inhibited) {
			g_variant_builder_add(builder, "{sv}",
					      "Inhibited", g_variant_new_boolean(inhibited));
		}
		if (prev_period != period) {
			const char *name = period_names[period];
			g_variant_builder_add(builder, "{sv}",
					      "Period", g_variant_new_string(name));
		}

		g_dbus_connection_emit_signal(conn, NULL,
					      REDSHIFT_OBJECT_PATH,
					      "org.freedesktop.DBus.Properties",
					      "PropertiesChanged",
					      g_variant_new("(sa{sv}as)",
							    REDSHIFT_INTERFACE_NAME,
							    builder,
							    NULL),
					      &local_error);
		g_assert_no_error(local_error);
	}

	/* If the temperature difference is large enough,
	   make a nice transition. */
	if (abs(temperature - temp_now) > 25) {
		if (trans_timer != 0) {
			g_source_remove(trans_timer);
		}

		g_printf("Create short transition: %u -> %u\n", temp_now, temperature);
		trans_temp_start = temp_now;
		trans_length = 40 - trans_time;
		trans_time = 0;

		trans_timer = g_timeout_add(100, short_transition_update_cb, NULL);
	} else if (temperature != temp_now) {
		const gfloat gamma[] = {1.0, 1.0, 1.0};
		if (current_method != NULL) {
			current_method->set_temperature(&gamma_state, temperature,
							1.0, gamma);
		}
		temp_now = temperature;
	}

	return TRUE;
}

static void
screen_update_restart(GDBusConnection *conn)
{
	if (screen_update_timer != 0) {
		g_source_remove(screen_update_timer);
	}
	screen_update_cb(conn);
	screen_update_timer = g_timeout_add_seconds(5, screen_update_cb, conn);
}


/* DBus service functions */
static void
handle_method_call(GDBusConnection *conn,
		   const gchar *sender,
		   const gchar *obj_path,
		   const gchar *interface_name,
		   const gchar *method_name,
		   GVariant *parameters,
		   GDBusMethodInvocation *invocation,
		   gpointer data)
{
	if (g_strcmp0(method_name, "AcquireCookie") == 0) {
		gint32 cookie = next_cookie++;
		const gchar *program;
		g_variant_get(parameters, "(&s)", &program);

		g_printf("AcquireCookie for `%s'.\n", program);

		g_hash_table_insert(cookies, GINT_TO_POINTER(cookie), g_strdup(program));
		g_dbus_method_invocation_return_value(invocation,
						      g_variant_new("(i)", cookie));
	} else if (g_strcmp0(method_name, "ReleaseCookie") == 0) {
		gint32 cookie;
		g_variant_get(parameters, "(i)", &cookie);

		gchar *program = g_hash_table_lookup(cookies, GINT_TO_POINTER(cookie));
		if (program == NULL) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.UnknownCookie",
								   "Unknown cookie value");
			return;
		}

		g_printf("ReleaseCookie for `%s'.\n", program);

		/* Remove all rules enforced by program */
		gboolean found = FALSE;
		found = g_hash_table_remove(inhibitors, GINT_TO_POINTER(cookie));

		for (int i = 0; i < FORCED_TEMP_LAYERS; i++) {
			if (forced_temp_cookie[i] == cookie) {
				forced_temp_cookie[i] = 0;
				found = TRUE;
			}
		}

		if (forced_location_cookie == cookie) {
			forced_location_cookie = 0;
			found = TRUE;
		}

		if (found) screen_update_restart(conn);

		/* Remove from list of cookies */
		g_hash_table_remove(cookies, GINT_TO_POINTER(cookie));
		g_free(program);

		g_dbus_method_invocation_return_value(invocation, NULL);
	} else if (g_strcmp0(method_name, "Inhibit") == 0) {
		gint32 cookie;
		g_variant_get(parameters, "(i)", &cookie);

		gchar *program = g_hash_table_lookup(cookies, GINT_TO_POINTER(cookie));
		if (program == NULL) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.UnknownCookie",
								   "Unknown cookie value");
			return;
		}

		g_hash_table_add(inhibitors, GINT_TO_POINTER(cookie));

		if (!inhibited) {
			screen_update_restart(conn);
		}

		g_printf("Inhibit for `%s'.\n", program);

		g_dbus_method_invocation_return_value(invocation, NULL);
	} else if (g_strcmp0(method_name, "Uninhibit") == 0) {
		gint32 cookie;
		g_variant_get(parameters, "(i)", &cookie);

		gchar *program = g_hash_table_lookup(cookies, GINT_TO_POINTER(cookie));
		if (program == NULL) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.UnknownCookie",
								   "Unknown cookie value");
			return;
		}

		g_hash_table_remove(inhibitors, GINT_TO_POINTER(cookie));

		if (inhibited && g_hash_table_size(inhibitors) == 0) {
			screen_update_restart(conn);
		}

		g_printf("Uninhibit for `%s'.\n", program);

		g_dbus_method_invocation_return_value(invocation, NULL);
	} else if (g_strcmp0(method_name, "EnforceTemperature") == 0) {
		gint32 cookie;
		guint32 temp;
		gboolean priority;
		g_variant_get(parameters, "(iub)", &cookie, &temp, &priority);

		gchar *program = g_hash_table_lookup(cookies, GINT_TO_POINTER(cookie));
		if (program == NULL) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.UnknownCookie",
								   "Unknown cookie value");
			return;
		}

		int index = priority ? 1 : 0;

		if (forced_temp_cookie[index] != 0 &&
		    forced_temp_cookie[index] != cookie) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.AlreadyEnforced",
								   "Another client is already enforcing temperature");
			return;
		}

		/* Check parameter bounds */
		if (temp < TEMP_MIN || temp > TEMP_MAX) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.InvalidArgument",
								   "Temperature is invalid");
			return;
		}

		/* Set forced location */
		forced_temp_cookie[index] = cookie;
		forced_temp[index] = temp;

		screen_update_restart(conn);

		g_printf("EnforceTemperature for `%s'.\n", program);

		g_dbus_method_invocation_return_value(invocation, NULL);
	} else if (g_strcmp0(method_name, "UnenforceTemperature") == 0) {
		gint32 cookie;
		gboolean priority;
		g_variant_get(parameters, "(ib)", &cookie, &priority);

		gchar *program = g_hash_table_lookup(cookies, GINT_TO_POINTER(cookie));
		if (program == NULL) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.UnknownCookie",
								   "Unknown cookie value");
			return;
		}

		g_printf("UnenforceTemperature for `%s'.\n", program);

		int index = priority ? 1 : 0;

		if (forced_temp_cookie[index] == cookie) {
			forced_temp_cookie[index] = 0;
			screen_update_restart(conn);
		}

		g_dbus_method_invocation_return_value(invocation, NULL);
	} else if (g_strcmp0(method_name, "EnforceLocation") == 0) {
		gint32 cookie;
		gdouble lat;
		gdouble lon;
		g_variant_get(parameters, "(idd)", &cookie, &lat, &lon);

		gchar *program = g_hash_table_lookup(cookies, GINT_TO_POINTER(cookie));
		if (program == NULL) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.UnknownCookie",
								   "Unknown cookie value");
			return;
		}

		if (forced_location_cookie != 0 &&
		    forced_location_cookie != cookie) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.AlreadyEnforced",
								   "Another client is already enforcing location");
			return;
		}

		/* Check parameter bounds */
		if (lat < LAT_MIN || lat > LAT_MAX ||
		    lon < LON_MIN || lon > LON_MAX) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.InvalidArgument",
								   "Location is invalid");
			return;
		}

		/* Set forced location */
		forced_location_cookie = cookie;
		forced_lat = lat;
		forced_lon = lon;

		/* Signal change in location */
		GError *local_error = NULL;
		GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
		g_variant_builder_add(builder, "{sv}", "CurrentLatitude", g_variant_new_double(forced_lat));
		g_variant_builder_add(builder, "{sv}", "CurrentLongitude", g_variant_new_double(forced_lon));

		g_dbus_connection_emit_signal(conn, NULL,
					      obj_path,
					      "org.freedesktop.DBus.Properties",
					      "PropertiesChanged",
					      g_variant_new("(sa{sv}as)",
							    interface_name,
							    builder,
							    NULL),
					      &local_error);
		g_assert_no_error(local_error);

		screen_update_restart(conn);

		g_printf("EnforceLocation for `%s'.\n", program);

		g_dbus_method_invocation_return_value(invocation, NULL);
	} else if (g_strcmp0(method_name, "UnenforceLocation") == 0) {
		gint32 cookie;
		g_variant_get(parameters, "(i)", &cookie);

		gchar *program = g_hash_table_lookup(cookies, GINT_TO_POINTER(cookie));
		if (program == NULL) {
			g_dbus_method_invocation_return_dbus_error(invocation,
								   "dk.jonls.redshift.Redshift.UnknownCookie",
								   "Unknown cookie value");
			return;
		}

		g_printf("UnenforceLocation for `%s'.\n", program);

		if (forced_location_cookie == cookie) {
			forced_location_cookie = 0;
			screen_update_restart(conn);

			/* Signal change in location */
			GError *local_error = NULL;
			GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
			g_variant_builder_add(builder, "{sv}", "CurrentLatitude", g_variant_new_double(latitude));
			g_variant_builder_add(builder, "{sv}", "CurrentLongitude", g_variant_new_double(longitude));

			g_dbus_connection_emit_signal(conn, NULL,
						      obj_path,
						      "org.freedesktop.DBus.Properties",
						      "PropertiesChanged",
						      g_variant_new("(sa{sv}as)",
								    interface_name,
								    builder,
								    NULL),
						      &local_error);
			g_assert_no_error(local_error);
		}

		g_dbus_method_invocation_return_value(invocation, NULL);
	} else if (g_strcmp0(method_name, "GetElevation") == 0) {
		g_dbus_method_invocation_return_value(invocation,
						      g_variant_new("(d)", elevation));
	}
}

static GVariant *
handle_get_property(GDBusConnection *conn,
		    const gchar *sender,
		    const gchar *obj_path,
		    const gchar *interface_name,
		    const gchar *prop_name,
		    GError **error,
		    gpointer data)
{
	GVariant *ret = NULL;

	if (g_strcmp0(prop_name, "Inhibited") == 0) {
		ret = g_variant_new_boolean(inhibited);
	} else if (g_strcmp0(prop_name, "Period") == 0) {
		ret = g_variant_new_string(period_names[period]);
	} else if (g_strcmp0(prop_name, "Temperature") == 0) {
		ret = g_variant_new_uint32(temperature);
	} else if (g_strcmp0(prop_name, "CurrentLatitude") == 0) {
		double lat = latitude;
		if (forced_location_cookie != 0) lat = forced_lat;
		ret = g_variant_new_double(lat);
	} else if (g_strcmp0(prop_name, "CurrentLongitude") == 0) {
		double lon = longitude;
		if (forced_location_cookie != 0) lon = forced_lon;
		ret = g_variant_new_double(lon);
	} else if (g_strcmp0(prop_name, "TemperatureDay") == 0) {
		ret = g_variant_new_uint32(temp_day);
	} else if (g_strcmp0(prop_name, "TemperatureNight") == 0) {
		ret = g_variant_new_uint32(temp_night);
	}

	return ret;
}

static gboolean
handle_set_property(GDBusConnection *conn,
		    const gchar *sender,
		    const gchar *obj_path,
		    const gchar *interface_name,
		    const gchar *prop_name,
		    GVariant *value,
		    GError **error,
		    gpointer data)
{
	if (g_strcmp0(prop_name, "TemperatureDay") == 0) {
		guint32 temp = g_variant_get_uint32(value);

		if (temp < TEMP_MIN || temp > TEMP_MAX) {
			g_set_error(error,
				    G_IO_ERROR,
				    G_IO_ERROR_FAILED,
				    "Temperature out of bounds");
		} else {
			temp_day = temp;

			screen_update_restart(conn);

			GError *local_error = NULL;
			GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
			g_variant_builder_add(builder, "{sv}", "TemperatureDay",
					      g_variant_new_uint32(temp_day));

			g_dbus_connection_emit_signal(conn, NULL,
						      obj_path,
						      "org.freedesktop.DBus.Properties",
						      "PropertiesChanged",
						      g_variant_new("(sa{sv}as)",
								    interface_name,
								    builder,
								    NULL),
						      &local_error);
			g_assert_no_error(local_error);
		}
	} else if (g_strcmp0(prop_name, "TemperatureNight") == 0) {
		guint32 temp = g_variant_get_uint32(value);

		if (temp < TEMP_MIN || temp > TEMP_MAX) {
			g_set_error(error,
				    G_IO_ERROR,
				    G_IO_ERROR_FAILED,
				    "Temperature out of bounds");
		} else {
			temp_night = temp;

			screen_update_restart(conn);

			GError *local_error = NULL;
			GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
			g_variant_builder_add(builder, "{sv}", "TemperatureNight",
					      g_variant_new_uint32(temp_night));

			g_dbus_connection_emit_signal(conn, NULL,
						      obj_path,
						      "org.freedesktop.DBus.Properties",
						      "PropertiesChanged",
						      g_variant_new("(sa{sv}as)",
								    interface_name,
								    builder,
								    NULL),
						      &local_error);
			g_assert_no_error(local_error);
		}
	}

	return *error == NULL;
}

static const GDBusInterfaceVTable interface_vtable = {
	handle_method_call,
	handle_get_property,
	handle_set_property
};


static void
on_bus_acquired(GDBusConnection *conn,
		const gchar *name,
		gpointer data)
{
	g_fprintf(stderr, "Bus acquired: `%s'.\n", name);

	guint registration_id = g_dbus_connection_register_object(conn,
								  REDSHIFT_OBJECT_PATH,
								  introspection_data->interfaces[0],
								  &interface_vtable,
								  NULL, NULL,
								  NULL);
	g_assert(registration_id > 0);

	/* Start screen update timer */
	screen_update_restart(conn);
}

static void
on_name_acquired(GDBusConnection *conn,
		 const gchar *name,
		 gpointer data)
{
	g_fprintf(stderr, "Name acquired: `%s'.\n", name);
}

static void
on_name_lost(GDBusConnection *conn,
	     const gchar *name,
	     gpointer data)
{
	g_fprintf(stderr, "Name lost: `%s'.\n", name);
	exit(EXIT_FAILURE);
}


int
main(int argc, char *argv[])
{
#if !GLIB_CHECK_VERSION(2, 35, 0)
	g_type_init();
#endif

	/* Create hash table for cookies */
	cookies = g_hash_table_new(NULL, NULL);

	/* Create hash table for inhibitors (set) */
	inhibitors = g_hash_table_new(NULL, NULL);

	/* Setup gamma method */
	for (int i = 0; gamma_methods[i].name != NULL; i++) {
		const gamma_method_t *m = &gamma_methods[i];

		int r = m->init(&gamma_state);
		if (r < 0) continue;

		r = m->start(&gamma_state);
		if (r < 0) continue;

		current_method = m;
		g_printf("Using method `%s'.\n", current_method->name);
		break;
	}

	/* Build node info from XML */
	introspection_data = g_dbus_node_info_new_for_xml(introspection_xml,
							  NULL);
	g_assert(introspection_data != NULL);

	/* Obtain DBus bus name */
	GBusNameOwnerFlags flags =
		G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
		G_BUS_NAME_OWNER_FLAGS_REPLACE;
	guint owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
					REDSHIFT_BUS_NAME,
					flags,
					on_bus_acquired,
					on_name_acquired,
					on_name_lost,
					NULL,
					NULL);

	/* Start main loop */
	GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	/* Clean up */
	g_bus_unown_name(owner_id);

	if (current_method != NULL) {
		current_method->restore(&gamma_state);
		current_method->free(&gamma_state);
	}

	return 0;
}
