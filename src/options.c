/* options.c -- Program options
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

   Copyright (c) 2017  Jon Lund Steffensen <jonlst@gmail.com>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "redshift.h"
#include "options.h"
#include "solar.h"
#include "elektra/redshift-conf.h"

/* A gamma string contains either one floating point value,
   or three values separated by colon. */
static int
parse_gamma_string(const char *str, float gamma[])
{
	char *s = strchr(str, ':');
	if (s == NULL) {
		/* Use value for all channels */
		float g = atof(str);
		gamma[0] = gamma[1] = gamma[2] = g;
	} else {
		/* Parse separate value for each channel */
		*(s++) = '\0';
		char *g_s = s;
		s = strchr(s, ':');
		if (s == NULL) return -1;

		*(s++) = '\0';
		gamma[0] = atof(str); /* Red */
		gamma[1] = atof(g_s); /* Blue */
		gamma[2] = atof(s); /* Green */
	}

	return 0;
}

/* Parse transition time string e.g. "04:50". Returns negative on failure,
   otherwise the parsed time is returned as seconds since midnight. */
static int
parse_transition_time(const char *str, const char **end)
{
	const char *min = NULL;
	errno = 0;
	long hours = strtol(str, (char **)&min, 10);
	if (errno != 0 || min == str || min[0] != ':' ||
	    hours < 0 || hours >= 24) {
		return -1;
	}

	min += 1;
	errno = 0;
	long minutes = strtol(min, (char **)end, 10);
	if (errno != 0 || *end == min || minutes < 0 || minutes >= 60) {
		return -1;
	}

	return minutes * 60 + hours * 3600;
}

/* Print list of adjustment methods. */
static void
print_method_list(const gamma_method_t *gamma_methods)
{
	fputs(_("Available adjustment methods:\n"), stdout);
	for (int i = 0; gamma_methods[i].name != NULL; i++) {
		printf("  %s\n", gamma_methods[i].name);
	}

	fputs("\n", stdout);
	/* TRANSLATORS: `help-adjustment-methods' must not be translated. */
    fputs(_("Try `--help` and `--help-methods' for help about the methods.\n"), stdout);
}

/* Print list of location providers. */
static void
print_provider_list(const location_provider_t location_providers[])
{
	fputs(_("Available location providers:\n"), stdout);
	for (int i = 0; location_providers[i].name != NULL; i++) {
		printf("  %s\n", location_providers[i].name);
	}

	fputs("\n", stdout);
	/* TRANSLATORS: `help-location-providers' must not be translated. */
	fputs(_("Try `--help` and `--help-providers' for help about the providers.\n"), stdout);
}

/* Return the gamma method with the given name. */
static const gamma_method_t *
find_gamma_method(const gamma_method_t gamma_methods[], const char *name)
{
	const gamma_method_t *method = NULL;
	for (int i = 0; gamma_methods[i].name != NULL; i++) {
		const gamma_method_t *m = &gamma_methods[i];
		if (strcasecmp(name, m->name) == 0) {
			method = m;
			break;
		}
	}

	return method;
}

/* Return location provider with the given name. */
static const location_provider_t *
find_location_provider(
	const location_provider_t location_providers[], const char *name)
{
	const location_provider_t *provider = NULL;
	for (int i = 0; location_providers[i].name != NULL; i++) {
		const location_provider_t *p = &location_providers[i];
		if (strcasecmp(name, p->name) == 0) {
			provider = p;
			break;
		}
	}

	return provider;
}

/**
 *
 * Load options from Elektra, store them in the passed options struct and configure the gamma_methods and location_providers supported in this build..
 *
 * @param options A pointer to the options struct where values should be stored.
 * @param elektra A pointer to the elektra instance.
 * @param gamma_methods A pointer to the gamma_methods supported in this build of redshift..
 * @param location_providers A pointer to the location_providers. supported in this build of redshift.
 * @return 0 if loading was successful, -1 on error
 */
int
options_load_from_elektra(
        options_t *options,
        Elektra *elektra,
        const gamma_method_t *gamma_methods,
        const location_provider_t *location_providers) {
    /**
     * Before using Elektra, there were two sources for configuration: CLI options and config file.
     * From redshift's point of view there is only one source now, namely Elektra.
     */
    // BEGIN Block: Options from parse_command_line_option and parse_config_file_option

    // Programm mode
    ElektraEnumMode mode = elektraGetMode(elektra);
    switch (mode) {
        case ELEKTRA_ENUM_MODE_CONTINUAL: {
            options->mode = PROGRAM_MODE_CONTINUAL;
            break;
        }
        case ELEKTRA_ENUM_MODE_ONESHOT: {
            options->mode = PROGRAM_MODE_ONE_SHOT;
            break;
        }
        case ELEKTRA_ENUM_MODE_PRINT: {
            options->mode = PROGRAM_MODE_PRINT;
            break;
        }
        case ELEKTRA_ENUM_MODE_RESET: {
            options->mode = PROGRAM_MODE_RESET;
            break;
        }
        case ELEKTRA_ENUM_MODE_ONESHOTMANUAL: {
            options->temp_set = elektraGetTempOneshotmanual(elektra);
            options->mode = PROGRAM_MODE_MANUAL;
            break;
        }
    }

    // Brightness
    *(&options->scheme.day.brightness) = elektraGetBrightnessDay(elektra);
    *(&options->scheme.night.brightness) = elektraGetBrightnessNight(elektra);

    // Gamma
    const char *gammaDayString = elektraGetGammaDay(elektra);
    parse_gamma_string(gammaDayString, *(&options->scheme.day.gamma));
    const char *gammaNightString = elektraGetGammaNight(elektra);
    parse_gamma_string(gammaNightString, *(&options->scheme.night.gamma));

    // Location and adjustment help
    if (elektraGetAdjustmentMethodHelp(elektra)) {
        for (int i = 0; gamma_methods[i].name != NULL; i++) {
            const gamma_method_t *m = &gamma_methods[i];
            printf(_("Help section for method `%s':\n"), m->name);
            printf(_("=============================\n"));
            m->print_help(stdout);
            printf(_("=============================\n\n"));
        }
        return -1;
    }

    if (elektraGetProviderLocationHelp(elektra)) {
        for (int i = 0; location_providers[i].name != NULL; i++) {
            const location_provider_t *p = &location_providers[i];
            printf(_("Help section for provider `%s':\n"), p->name);
            printf(_("=============================\n"));
            p->print_help(stdout);
            printf(_("=============================\n\n"));
        }
        return -1;
    }

    // Location provider
    ElektraEnumProviderLocation locationProvider = elektraGetProviderLocation(elektra);
    if (locationProvider == ELEKTRA_ENUM_PROVIDER_LOCATION_LIST) {
        // In list mode, print list of supported location providers.
        print_provider_list(location_providers);
        return -1;
    }
    else if (locationProvider == ELEKTRA_ENUM_PROVIDER_LOCATION_AUTO) {
        // In auto mode, a supported provider will be chosen in redshift.c:main(...)
        options->provider = NULL;
    }
    else {
        // Otherwise, try to find the provider by name.
        const char *locationProviderName = elektraEnumProviderLocationToConstString(
                elektraGetProviderLocation(elektra)
        );
        const location_provider_t *provider = find_location_provider(location_providers, locationProviderName);
        if(provider == NULL) {
            // User picked a locationProvider provider which is not supported in this build.
            fprintf(stderr, _("The chosen location provider \"%s\" is not supported in this build of redshift.\n"), locationProviderName);
            return -1;
        }
        else {
            options->provider = provider;
        }
    }
    // Set lat and lon. Will only be used if the manual provider is chosen
    const float lat = elektraGetProviderLocationManualLat(elektra);
    options->provider_manual_arg_lat = lat;
    const float lon = elektraGetProviderLocationManualLon(elektra);
    options->provider_manual_arg_lon = lon;

    // Adjustment method
    ElektraEnumAdjustmentMethod adjustmentMethod = elektraGetAdjustmentMethod(elektra);
    if (adjustmentMethod == ELEKTRA_ENUM_ADJUSTMENT_METHOD_LIST) {
        // In list mode, print list of supported adjustment methods.
        print_method_list(gamma_methods);
        return -1;
    } else if (adjustmentMethod == ELEKTRA_ENUM_ADJUSTMENT_METHOD_AUTO) {
        // In auto mode, a supported method will be chosen in redshift.c:main(...)
        options->method = NULL;
    } else {
        // Otherwise, try to find the method by name.
        const char * adjustmentMethodName = elektraEnumAdjustmentMethodToConstString(
                elektraGetAdjustmentMethod(elektra)
        );
        const gamma_method_t *method = find_gamma_method(gamma_methods, adjustmentMethodName);
        if (method == NULL) {
            // User picked a method which is not supported in this build.
            fprintf(stderr, _("The chosen adjustment method \"%s\" is not supported in this build of redshift.\n"), adjustmentMethodName);
            return -1;
        }
        else {
            options->method = method;
        }
    }

    options->method_crtc = elektraGetAdjustmentCrtc(elektra);
    options->method_screen = elektraGetAdjustmentScreen(elektra);
    options->method_drm_card = elektraGetAdjustmentDrmCard(elektra);


    // Preserve gamma
    options->preserve_gamma = elektraGetGammaPreserve(elektra);

    // Fade
    options->use_fade = !elektraGetFastfade(elektra);

    // Temperature
    options->scheme.day.temperature = elektraGetTempDay(elektra);
    options->scheme.night.temperature = elektraGetTempNight(elektra);

    // Verbose
    options->verbose = elektraGetVerbose(elektra);

    // Version
    if (elektraGetVersion(elektra)) {
        printf("%s\n", PACKAGE_STRING);
        return -1;
    }

    // END Block

    // BEGIN Block: From parse_config_file_option (if not already handled above)

    // Elevation
    options->scheme.high = elektraGetProviderLocationElevationHigh(elektra);
    options->scheme.low = elektraGetProviderLocationElevationLow(elektra);

    // Time
    options->scheme.use_time = elektraGetProvider(elektra) == ELEKTRA_ENUM_PROVIDER_TIME;

    if(options->scheme.use_time) {
        const char* dawnStartString = elektraGetProviderTimeDawnStart(elektra);
        const char* dawnEndString = elektraGetProviderTimeDawnEnd(elektra);
        const char* duskStartString = elektraGetProviderTimeDuskStart(elektra);
        const char* duskEndString = elektraGetProviderTimeDuskEnd(elektra);
        options->scheme.dawn.start = parse_transition_time(dawnStartString, NULL);
        options->scheme.dawn.end = parse_transition_time(dawnEndString, NULL);
        options->scheme.dusk.start = parse_transition_time(duskStartString, NULL);
        options->scheme.dusk.end = parse_transition_time(duskEndString, NULL);

        // Validation from redshift.c:main(...)
        if (options->scheme.dawn.start > options->scheme.dawn.end ||
            options->scheme.dawn.end > options->scheme.dusk.start ||
            options->scheme.dusk.start > options->scheme.dusk.end) {
            fputs(_("Invalid dawn/dusk time configuration!\n"),
                  stderr);
            return -1;
        }
    }
    // END Block



    return 0;
}


/* Initialize options struct. */
void
options_init(options_t *options)
{
	options->config_filepath = NULL;

	/* Settings for day, night and transition period.
	   Initialized to indicate that the values are not set yet. */
	options->scheme.use_time = 0;
	options->scheme.dawn.start = -1;
	options->scheme.dawn.end = -1;
	options->scheme.dusk.start = -1;
	options->scheme.dusk.end = -1;

	options->scheme.day.temperature = -1;
	options->scheme.day.gamma[0] = NAN;
	options->scheme.day.brightness = NAN;

	options->scheme.night.temperature = -1;
	options->scheme.night.gamma[0] = NAN;
	options->scheme.night.brightness = NAN;

	/* Temperature for manual mode */
	options->temp_set = -1;

	options->method = NULL;
    options->method_crtc = -1;
    options->method_screen = -1;
    options->method_drm_card = -1;

	options->provider = NULL;

	options->use_fade = -1;
	options->preserve_gamma = 1;
	options->mode = PROGRAM_MODE_CONTINUAL;
	options->verbose = 0;
}
