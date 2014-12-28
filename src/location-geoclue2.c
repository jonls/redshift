/* location-geoclue2.c -- GeoClue2 location provider source
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

   Copyright (c) 2014  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include "location-geoclue2.h"

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif


typedef struct {
	GMainLoop *loop;

	int available;
	float latitude;
	float longitude;
} get_location_data_t;


int
location_geoclue2_init(void *state)
{
#if !GLIB_CHECK_VERSION(2, 35, 0)
	g_type_init();
#endif
	return 0;
}

int
location_geoclue2_start(void *state)
{
	return 0;
}

void
location_geoclue2_free(void *state)
{
}

void
location_geoclue2_print_help(FILE *f)
{
	fputs(_("Use the location as discovered by a GeoClue2 provider.\n"), f);
	fputs("\n", f);

	fprintf(f, _("NOTE: currently Redshift doesn't recheck %s once started,\n"
		     "which means it has to be restarted to take notice after travel.\n"),
		"GeoClue2");
	fputs("\n", f);
}

int
location_geoclue2_set_option(void *state,
			    const char *key, const char *value)
{
	fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
	return -1;
}

/* Handle position change callbacks */
static void
geoclue_client_signal_cb(GDBusProxy *client, gchar *sender_name,
			 gchar *signal_name, GVariant *parameters,
			 gpointer *user_data)
{
	get_location_data_t *data = (get_location_data_t *)user_data;

	/* Only handle LocationUpdated signals */
	if (g_strcmp0(signal_name, "LocationUpdated") != 0) {
		return;
	}

	/* Obtain location path */
	const gchar *location_path;
	g_variant_get_child(parameters, 1, "&o", &location_path);

	/* Obtain location */
	GError *error = NULL;
	GDBusProxy *location =
		g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
					      G_DBUS_PROXY_FLAGS_NONE,
					      NULL,
					      "org.freedesktop.GeoClue2",
					      location_path,
					      "org.freedesktop.GeoClue2.Location",
					      NULL, &error);
	if (location == NULL) {
		g_printerr(_("Unable to obtain location: %s.\n"),
			   error->message);
		g_error_free(error);
		return;
	}

	/* Read location properties */
	GVariant *lat_v = g_dbus_proxy_get_cached_property(location,
							   "Latitude");
	data->latitude = g_variant_get_double(lat_v);

	GVariant *lon_v = g_dbus_proxy_get_cached_property(location,
							   "Longitude");
	data->longitude = g_variant_get_double(lon_v);

	data->available = 1;

	/* Return from main loop */
	g_main_loop_quit(data->loop);
}

/* Callback when GeoClue name appears on the bus */
static void
on_name_appeared(GDBusConnection *conn, const gchar *name,
		 const gchar *name_owner, gpointer user_data)
{
	get_location_data_t *data = (get_location_data_t *)user_data;

	/* Obtain GeoClue Manager */
	GError *error = NULL;
	GDBusProxy *geoclue_manager =
		g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
					      G_DBUS_PROXY_FLAGS_NONE,
					      NULL,
					      "org.freedesktop.GeoClue2",
					      "/org/freedesktop/GeoClue2/Manager",
					      "org.freedesktop.GeoClue2.Manager",
					      NULL, &error);
	if (geoclue_manager == NULL) {
		g_printerr(_("Unable to obtain GeoClue Manager: %s.\n"),
			   error->message);
		g_error_free(error);
		return;
	}

	/* Obtain GeoClue Client path */
	error = NULL;
	GVariant *client_path_v =
		g_dbus_proxy_call_sync(geoclue_manager,
				       "GetClient",
				       NULL,
				       G_DBUS_CALL_FLAGS_NONE,
				       -1, NULL, &error);
	if (client_path_v == NULL) {
		g_printerr(_("Unable to obtain GeoClue client path: %s.\n"),
			   error->message);
		g_error_free(error);
		g_object_unref(geoclue_manager);
		return;
	}

	const gchar *client_path;
	g_variant_get(client_path_v, "(&o)", &client_path);

	/* Obtain GeoClue client */
	error = NULL;
	GDBusProxy *geoclue_client =
		g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
					      G_DBUS_PROXY_FLAGS_NONE,
					      NULL,
					      "org.freedesktop.GeoClue2",
					      client_path,
					      "org.freedesktop.GeoClue2.Client",
					      NULL, &error);
	if (geoclue_client == NULL) {
		g_printerr(_("Unable to obtain GeoClue Client: %s.\n"),
			   error->message);
		g_error_free(error);
		g_variant_unref(client_path_v);
		g_object_unref(geoclue_manager);
		return;
	}

	g_variant_unref(client_path_v);

	/* Set distance threshold */
	error = NULL;
	GVariant *ret_v =
		g_dbus_proxy_call_sync(geoclue_client,
				       "org.freedesktop.DBus.Properties.Set",
				       g_variant_new("(ssv)",
						     "org.freedesktop.GeoClue2.Client",
						     "DistanceThreshold",
						     g_variant_new("u", 50000)),
				       G_DBUS_CALL_FLAGS_NONE,
				       -1, NULL, &error);
	if (ret_v == NULL) {
		g_printerr(_("Unable to set distance threshold: %s.\n"),
			   error->message);
		g_error_free(error);
		g_object_unref(geoclue_client);
		g_object_unref(geoclue_manager);
		return;
	}

	g_variant_unref(ret_v);

	/* Attach signal callback to client */
	g_signal_connect(geoclue_client, "g-signal",
			 G_CALLBACK(geoclue_client_signal_cb),
			 data);

	/* Start GeoClue client */
	error = NULL;
	ret_v = g_dbus_proxy_call_sync(geoclue_client,
				       "Start",
				       NULL,
				       G_DBUS_CALL_FLAGS_NONE,
				       -1, NULL, &error);
	if (ret_v == NULL) {
		g_printerr(_("Unable to start GeoClue client: %s.\n"),
			   error->message);
		g_error_free(error);
		g_object_unref(geoclue_client);
		g_object_unref(geoclue_manager);
		return;
	}

	g_variant_unref(ret_v);
}

/* Callback when GeoClue disappears from the bus */
static void
on_name_vanished(GDBusConnection *connection, const gchar *name,
		 gpointer user_data)
{
	get_location_data_t *data = (get_location_data_t *)user_data;

	g_fprintf(stderr, _("Unable to connect to GeoClue.\n"));

	g_main_loop_quit(data->loop);
}

int
location_geoclue2_get_location(void *state,
			       float *lat, float *lon)
{
	get_location_data_t data;
	data.available = 0;

	guint watcher_id = g_bus_watch_name(G_BUS_TYPE_SYSTEM,
					    "org.freedesktop.GeoClue2",
					    G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
					    on_name_appeared,
					    on_name_vanished,
					    &data, NULL);
	data.loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(data.loop);

	g_bus_unwatch_name(watcher_id);

	if (!data.available) return -1;

	*lat = data.latitude;
	*lon = data.longitude;

	return 0;
}
