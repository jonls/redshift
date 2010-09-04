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


/* Find current selected city for the clock applet with the specified id.
   Returns NULL if not found. */
static char *
find_current_city(GConfClient *client, const char *id)
{
	GError *error = NULL;

	char *current_city = NULL;
	char *cities_key = g_strdup_printf("/apps/panel/applets/%s"
					   "/prefs/cities", id);
	GSList *cities = gconf_client_get_list(client,
					       cities_key,
					       GCONF_VALUE_STRING, &error);

	if (error) {
		fprintf(stderr, _("Error reading city list: `%s'.\n"),
			cities_key);
		g_free(cities_key);
		return NULL;
	}

	g_free(cities_key);

	for (GSList *city = cities; city != NULL;
	     city = g_slist_next(city)) {
		char *city_spec = city->data;
		char *c = strstr(city_spec, "current=\"true\"");
		if (c) current_city = g_strdup(city_spec);
		g_free(city->data);
	}
	g_slist_free(cities);

	return current_city;
}

int
location_gnome_clock_init(location_gnome_clock_state_t *state)
{
	g_type_init();

	GError *error = NULL;
	GConfClient *client = gconf_client_get_default();

	/* Get a list of active applets in the panel. */
	GSList *applets = gconf_client_get_list(client,
			"/apps/panel/general/applet_id_list",
			GCONF_VALUE_STRING, &error);
	if (error) {
		fputs(_("Cannot list GNOME panel applets.\n"), stderr);
		g_object_unref(client);
		g_slist_free(applets);
		return -1;
	}

	/* Go through each applet and check if it is a clock applet.
	   When a clock applet is found, check whether there is a
	   city selected as the current city. */
	char *current_city = NULL;

	/* Keep track of the number of clock applets found to be able to give
	   better error output if something fails. */
	 int clock_applet_count = 0;

	for (GSList *applet = applets; applet != NULL;
	     applet = g_slist_next(applet)) {
		char *id = applet->data;
		if (current_city == NULL) {
			char *key = g_strdup_printf("/apps/panel/applets/%s"
						    "/bonobo_iid", id);
			char *bonobo_iid = gconf_client_get_string(client, key,
								   &error);

			if (!error && bonobo_iid != NULL &&
			    !strcmp(bonobo_iid, "OAFIID:GNOME_ClockApplet")) {
				clock_applet_count += 1;
				current_city = find_current_city(client, id);
			}

			g_free(bonobo_iid);
			g_free(key);
		}
		g_free(id);
	}

	g_slist_free(applets);
	g_object_unref(client);

	/* Check whether an applet and a current city was found. */

	if (clock_applet_count == 0) {
		fputs(_("No clock applet was found.\n"), stderr);
		return -1;
	}

	if (current_city == NULL) {
		fputs(_("No city selected as current city.\n"), stderr);
		return -1;
	}

	/* Find coords for selected city and parse as number. */

	char *lat_str = strstr(current_city, "latitude=\"");
	char *lon_str = strstr(current_city, "longitude=\"");
	if (lat_str == NULL || lon_str == NULL) {
		fputs(_("Location not specified for city.\n"), stderr);
		g_free(current_city);
		return -1;
	}

	g_free(current_city);

	char *lat_num_str = lat_str + strlen("latitude=\"");
	char *lon_num_str = lon_str + strlen("longitude=\"");

	state->lat = g_ascii_strtod(lat_num_str, NULL);
	state->lon = g_ascii_strtod(lon_num_str, NULL);

	return 0;
}

int
location_gnome_clock_start(location_gnome_clock_state_t *state)
{
	return 0;
}

void
location_gnome_clock_free(location_gnome_clock_state_t *state)
{
}

void
location_gnome_clock_print_help(FILE *f)
{
	fputs(_("Use the location as set in the GNOME Clock applet.\n"), f);
	fputs("\n", f);
}

int
location_gnome_clock_set_option(location_gnome_clock_state_t *state,
				const char *key, const char *value)
{
	return -1;
}

int
location_gnome_clock_get_location(location_gnome_clock_state_t *state,
				  float *lat, float *lon)
{
	*lat = state->lat;
	*lon = state->lon;

	return 0;
}
