/* location-geoclue.c -- Geoclue location provider source
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

   Copyright (c) 2010  Mathieu Trudel-Lapierre <mathieu-tl@ubuntu.com>
*/

#include <stdio.h>
#include <string.h>

#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-position.h>

#include "location-geoclue.h"

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#define DEFAULT_PROVIDER "org.freedesktop.Geoclue.Providers.UbuntuGeoIP"
#define DEFAULT_PROVIDER_PATH "/org/freedesktop/Geoclue/Providers/UbuntuGeoIP"

int
location_geoclue_init(location_geoclue_state_t *state)
{
	state->position = NULL;
	state->provider = NULL;
	state->provider_path = NULL;
	
	return 0;
}

int
location_geoclue_start(location_geoclue_state_t *state)
{
        if (state->provider && state->provider_path) {
		state->position = geoclue_position_new(state->provider,
						       state->provider_path);
        } else {
                GError *error = NULL;
                GeoclueMaster *master = geoclue_master_get_default();
                GeoclueMasterClient *client = geoclue_master_create_client(master,
                                                                           NULL, &error);
		g_object_unref(master);

                if (client == NULL) {
                        g_printerr(_("Unable to obtain master client: %s\n"),
				   error->message);
			g_error_free(error);
                        return -1;
                }

		if (!geoclue_master_client_set_requirements(client,
							    GEOCLUE_ACCURACY_LEVEL_REGION,
							    0, FALSE,
							    GEOCLUE_RESOURCE_NETWORK,
							    &error)) {
			g_printerr(_("Can't set requirements for master: %s\n"),
				   error->message);
			g_error_free(error);
			g_object_unref(client);

			return -1;
		}

		state->position = geoclue_master_client_create_position(client, NULL);

                g_object_unref(client);
	}

	gchar *name = NULL;

	if (geoclue_provider_get_provider_info(GEOCLUE_PROVIDER(state->position),
					       &name, NULL, NULL)) {
		fprintf(stdout, _("Started Geoclue provider `%s'.\n"), name);
		g_free(name);
	} else {
		fputs(_("Could not find a usable Geoclue provider.\n"), stderr);
		fputs(_("Try setting name and path to specify which to use.\n"), stderr);
		return -1;
	}

	return 0;
}

void
location_geoclue_free(location_geoclue_state_t *state)
{
	if (state->position != NULL) g_object_unref(state->position);
}

void
location_geoclue_print_help(FILE *f)
{
	fputs(_("Use the location as discovered by a Geoclue provider.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: Geoclue help output
	   left column must not be translated */
	fputs(_("  name=N\tName of Geoclue provider (or `default')\n"
		"  path=N\tPath of Geoclue provider (or `default')\n"), f);
	fputs("\n", f);
	fputs(_("NOTE: currently redshift doesn't recheck geoclue once started,\n"
		"which means it has to be restarted to take notice after travel.\n"),
	      f);
	fputs("\n", f);
}

int
location_geoclue_set_option(location_geoclue_state_t *state,
			    const char *key, const char *value)
{
	const char *provider = NULL;
	const char *path = NULL;

	/* Parse string value */
	if (key != NULL && strcasecmp(key, "name") == 0) {
		if (value != NULL && strcasecmp(value, "default") == 0) {
			provider = DEFAULT_PROVIDER;
		} else if (value != NULL) {
			provider = value;
		} else {
			fputs(_("Must specify a provider `name' (or use `default').\n"), stderr);
			return -1;
		}

		/* TODO I don't think we own the string here, should be copied. */
		state->provider = provider;
	} else if (key != NULL && strcasecmp(key, "path") == 0) {
		if (value != NULL && strcasecmp(value, "default") == 0) {
			path = DEFAULT_PROVIDER_PATH;
		} else if (value != NULL) {
			path = value;
		} else {
			fputs(_("Must specify a provider `path' (or use `default').\n"), stderr);
			return -1;
		}

		/* TODO I don't think we own the string here, should be copied. */
		state->provider_path = path;
	} else if (key == NULL) {
		return -1;
	} else {
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}

	return 0;
}

int
location_geoclue_get_location(location_geoclue_state_t *state,
			      float *lat, float *lon)
{
	GeocluePositionFields fields;
	GError *error = NULL;
	double latitude = 0, longitude = 0;

	fields = geoclue_position_get_position(state->position, NULL,
					       &latitude, &longitude, NULL,
					       NULL, &error);
	if (error) {
		g_printerr(_("Could not get location: %s.\n"), error->message);
		g_error_free(error);
		return -1;
	}
	
	if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE &&
	    fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
		fprintf(stdout, _("According to the geoclue provider"
				  " we're at: %.2f, %.2f\n"),
			latitude, longitude);
	} else {
		g_warning(_("Provider does not have a valid location available."));
		return -1;
	}

	*lat = latitude;
	*lon = longitude;

	return 0;
}
