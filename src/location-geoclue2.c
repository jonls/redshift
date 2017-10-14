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

   Copyright (c) 2014-2017  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include "location-geoclue2.h"
#include "redshift.h"
#include "pipeutils.h"

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#define DBUS_ACCESS_ERROR  "org.freedesktop.DBus.Error.AccessDenied"


typedef struct {
	GMainLoop *loop;
	GThread *thread;
	GMutex lock;
	int pipe_fd_read;
	int pipe_fd_write;
	int available;
	int error;
	float latitude;
	float longitude;
} location_geoclue2_state_t;


/* Print the message explaining denial from GeoClue. */
static void
print_denial_message()
{
	g_printerr(_(
		"Access to the current location was denied by GeoClue!\n"
		"Make sure that location services are enabled and that"
		" Redshift is permitted\nto use location services."
		" See https://github.com/jonls/redshift#faq for more\n"
		"information.\n"));
}

/* Indicate an unrecoverable error during GeoClue2 communication. */
static void
mark_error(location_geoclue2_state_t *state)
{
	g_mutex_lock(&state->lock);

	state->error = 1;

	g_mutex_unlock(&state->lock);

	pipeutils_signal(state->pipe_fd_write);
}

/* Handle position change callbacks */
static void
geoclue_client_signal_cb(GDBusProxy *client, gchar *sender_name,
			 gchar *signal_name, GVariant *parameters,
			 gpointer user_data)
{
	location_geoclue2_state_t *state = user_data;

	/* Only handle LocationUpdated signals */
	if (g_strcmp0(signal_name, "LocationUpdated") != 0) {
		return;
	}

	/* Obtain location path */
	const gchar *location_path;
	g_variant_get_child(parameters, 1, "&o", &location_path);

	/* Obtain location */
	GError *error = NULL;
	GDBusProxy *location = g_dbus_proxy_new_sync(
		g_dbus_proxy_get_connection(client),
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
		mark_error(state);
		return;
	}

	g_mutex_lock(&state->lock);

	/* Read location properties */
	GVariant *lat_v = g_dbus_proxy_get_cached_property(
		location, "Latitude");
	state->latitude = g_variant_get_double(lat_v);

	GVariant *lon_v = g_dbus_proxy_get_cached_property(
		location, "Longitude");
	state->longitude = g_variant_get_double(lon_v);

	state->available = 1;

	g_mutex_unlock(&state->lock);

	pipeutils_signal(state->pipe_fd_write);
}

/* Callback when GeoClue name appears on the bus */
static void
on_name_appeared(GDBusConnection *conn, const gchar *name,
		 const gchar *name_owner, gpointer user_data)
{
	location_geoclue2_state_t *state = user_data;

	/* Obtain GeoClue Manager */
	GError *error = NULL;
	GDBusProxy *geoclue_manager = g_dbus_proxy_new_sync(
		conn,
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
		mark_error(state);
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
		mark_error(state);
		return;
	}

	const gchar *client_path;
	g_variant_get(client_path_v, "(&o)", &client_path);

	/* Obtain GeoClue client */
	error = NULL;
	GDBusProxy *geoclue_client = g_dbus_proxy_new_sync(
		conn,
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
		mark_error(state);
		return;
	}

	g_variant_unref(client_path_v);

	/* Set desktop id (basename of the .desktop file) */
	error = NULL;
	GVariant *ret_v = g_dbus_proxy_call_sync(
		geoclue_client,
		"org.freedesktop.DBus.Properties.Set",
		g_variant_new("(ssv)",
		"org.freedesktop.GeoClue2.Client",
		"DesktopId",
		g_variant_new("s", "redshift")),
		G_DBUS_CALL_FLAGS_NONE,
		-1, NULL, &error);
	if (ret_v == NULL) {
		/* Ignore this error for now. The property is not available
		   in early versions of GeoClue2. */
	} else {
		g_variant_unref(ret_v);
	}

	/* Set distance threshold */
	error = NULL;
	ret_v = g_dbus_proxy_call_sync(
		geoclue_client,
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
		mark_error(state);
		return;
	}

	g_variant_unref(ret_v);

	/* Attach signal callback to client */
	g_signal_connect(geoclue_client, "g-signal",
			 G_CALLBACK(geoclue_client_signal_cb),
			 user_data);

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
		if (g_dbus_error_is_remote_error(error)) {
			gchar *dbus_error = g_dbus_error_get_remote_error(
				error);
			if (g_strcmp0(dbus_error, DBUS_ACCESS_ERROR) == 0) {
				print_denial_message();
			}
			g_free(dbus_error);
		}
		g_error_free(error);
		g_object_unref(geoclue_client);
		g_object_unref(geoclue_manager);
		mark_error(state);
		return;
	}

	g_variant_unref(ret_v);
}

/* Callback when GeoClue disappears from the bus */
static void
on_name_vanished(GDBusConnection *connection, const gchar *name,
		 gpointer user_data)
{
	location_geoclue2_state_t *state = user_data;

	g_mutex_lock(&state->lock);

	state->available = 0;

	g_mutex_unlock(&state->lock);

	pipeutils_signal(state->pipe_fd_write);
}

/* Callback when the pipe to the main thread is closed. */
static gboolean
on_pipe_closed(GIOChannel *channel, GIOCondition condition, gpointer user_data)
{
	location_geoclue2_state_t *state = user_data;
	g_main_loop_quit(state->loop);

	return FALSE;
}


/* Run loop for location provider thread. */
static void *
run_geoclue2_loop(void *state_)
{
	location_geoclue2_state_t *state = state_;

	GMainContext *context = g_main_context_new();
	g_main_context_push_thread_default(context);
	state->loop = g_main_loop_new(context, FALSE);

	guint watcher_id = g_bus_watch_name(
		G_BUS_TYPE_SYSTEM,
		"org.freedesktop.GeoClue2",
		G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
		on_name_appeared,
		on_name_vanished,
		state, NULL);

	/* Listen for closure of pipe */
	GIOChannel *pipe_channel = g_io_channel_unix_new(state->pipe_fd_write);
	GSource *pipe_source = g_io_create_watch(
		pipe_channel, G_IO_IN | G_IO_HUP | G_IO_ERR);
        g_source_set_callback(
		pipe_source, (GSourceFunc)on_pipe_closed, state, NULL);
        g_source_attach(pipe_source, context);

	g_main_loop_run(state->loop);

	g_source_unref(pipe_source);
	g_io_channel_unref(pipe_channel);
	close(state->pipe_fd_write);

	g_bus_unwatch_name(watcher_id);

	g_main_loop_unref(state->loop);
	g_main_context_unref(context);

	return NULL;
}

static int
location_geoclue2_init(location_geoclue2_state_t **state)
{
#if !GLIB_CHECK_VERSION(2, 35, 0)
	g_type_init();
#endif
	*state = malloc(sizeof(location_geoclue2_state_t));
	if (*state == NULL) return -1;
	return 0;
}

static int
location_geoclue2_start(location_geoclue2_state_t *state)
{
	state->pipe_fd_read = -1;
	state->pipe_fd_write = -1;

	state->available = 0;
	state->error = 0;
	state->latitude = 0;
	state->longitude = 0;

	int pipefds[2];
	int r = pipeutils_create_nonblocking(pipefds);
	if (r < 0) {
		fputs(_("Failed to start GeoClue2 provider!\n"), stderr);
		return -1;
	}

	state->pipe_fd_read = pipefds[0];
	state->pipe_fd_write = pipefds[1];

	pipeutils_signal(state->pipe_fd_write);

	g_mutex_init(&state->lock);
	state->thread = g_thread_new("geoclue2", run_geoclue2_loop, state);

	return 0;
}

static void
location_geoclue2_free(location_geoclue2_state_t *state)
{
	if (state->pipe_fd_read != -1) {
		close(state->pipe_fd_read);
	}

	/* Closing the pipe should cause the thread to exit. */
	g_thread_join(state->thread);
	state->thread = NULL;

	g_mutex_clear(&state->lock);

	free(state);
}

static void
location_geoclue2_print_help(FILE *f)
{
	fputs(_("Use the location as discovered by a GeoClue2 provider.\n"),
	      f);
	fputs("\n", f);
}

static int
location_geoclue2_set_option(location_geoclue2_state_t *state,
			     const char *key, const char *value)
{
	fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
	return -1;
}

static int
location_geoclue2_get_fd(location_geoclue2_state_t *state)
{
	return state->pipe_fd_read;
}

static int
location_geoclue2_handle(
	location_geoclue2_state_t *state,
	location_t *location, int *available)
{
	pipeutils_handle_signal(state->pipe_fd_read);

	g_mutex_lock(&state->lock);

	int error = state->error;
	location->lat = state->latitude;
	location->lon = state->longitude;
	*available = state->available;

	g_mutex_unlock(&state->lock);

	if (error) return -1;

	return 0;
}


const location_provider_t geoclue2_location_provider = {
	"geoclue2",
	(location_provider_init_func *)location_geoclue2_init,
	(location_provider_start_func *)location_geoclue2_start,
	(location_provider_free_func *)location_geoclue2_free,
	(location_provider_print_help_func *)location_geoclue2_print_help,
	(location_provider_set_option_func *)location_geoclue2_set_option,
	(location_provider_get_fd_func *)location_geoclue2_get_fd,
	(location_provider_handle_func *)location_geoclue2_handle
};
