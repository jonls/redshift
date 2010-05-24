/* location-gnome-clock.c -- GNOME Panel Clock location provider source
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

   Copyright (c) 2010  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdio.h>
#include <string.h>

#include <gconf/gconf-client.h>

#include "location-gnome-clock.h"

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif


int
location_gnome_clock_init(location_gnome_clock_state_t *state, char *args)
{
	g_type_init();

	GError *error = NULL;
	GConfClient *client = gconf_client_get_default();

	GSList *applets = gconf_client_all_dirs(client, "/apps/panel/applets",
						&error);
	if (error) {
		fputs(_("Cannot list dirs in `/apps/panel/applets'.\n"),
		      stderr);
		g_object_unref(client);
		return -1;
	}

	char *cities_key = NULL;
	for (GSList *applet = applets; applet != NULL;
	     applet = g_slist_next(applet)) {
		char *path = applet->data;
		if (cities_key == NULL) {
			char *key = g_strdup_printf("%s/bonobo_iid", path);
			char *bonobo_iid = gconf_client_get_string(client, key,
								   &error);

			if (!error && bonobo_iid != NULL) {
				if (!strcmp(bonobo_iid,
					    "OAFIID:GNOME_ClockApplet")) {
					cities_key = g_strdup_printf(
						"%s/prefs/cities", path);
				}
				g_free(bonobo_iid);
			}

			g_free(key);
		}
		g_free(path);
	}

	g_slist_free(applets);

	if (cities_key == NULL) {
		fputs(_("No clock applet was found.\n"), stderr);
		g_object_unref(client);
		return -1;
	}

	GSList *cities = gconf_client_get_list(client, cities_key,
					       GCONF_VALUE_STRING, &error);
	if (error) {
		fprintf(stderr, _("Error reading city list: `%s'.\n"),
			cities_key);
		g_free(cities_key);
		g_object_unref(client);
		return -1;
	}

	g_free(cities_key);

	char *current_city = NULL;
	for (GSList *city = cities; city != NULL;
	     city = g_slist_next(city)) {
		char *city_spec = city->data;
		char *c = strstr(city_spec, "current=\"true\"");
		if (c) current_city = g_strdup(city_spec);
		g_free(city->data);
	}
	g_slist_free(cities);

	if (current_city == NULL) {
		fputs(_("No city selected as current city.\n"), stderr);
		return -1;
	}

	char *lat_str = strstr(current_city, "latitude=\"");
	char *lon_str = strstr(current_city, "longitude=\"");
	if (lat_str == NULL || lon_str == NULL) {
		fputs(_("Location not specified for city.\n"), stderr);
		return -1;
	}

	char *lat_num_str = lat_str + strlen("latitude=\"");
	char *lon_num_str = lon_str + strlen("longitude=\"");

	state->lat = g_ascii_strtod(lat_num_str, NULL);
	state->lon = g_ascii_strtod(lon_num_str, NULL);

	return 0;
}

void
location_gnome_clock_free(location_gnome_clock_state_t *state)
{
}

int
location_gnome_clock_get_location(location_gnome_clock_state_t *state,
				  float *lat, float *lon)
{
	*lat = state->lat;
	*lon = state->lon;

	return 0;
}
