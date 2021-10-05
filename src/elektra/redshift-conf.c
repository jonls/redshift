// clang-format off


// clang-format on
/**
 * @file
 *
 * This file was automatically generated using `kdb gen highlevel`.
 * Any changes will be overwritten, when the file is regenerated.
 *
 * @copyright BSD Zero Clause License
 *
 *     Copyright (c) Elektra Initiative (https://www.libelektra.org)
 *
 *     Permission to use, copy, modify, and/or distribute this software for any
 *     purpose with or without fee is hereby granted.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 *     REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 *     FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 *     INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 *     LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 *     OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 */

#include "redshift-conf.h"



#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <kdbhelper.h>
#include <kdbinvoke.h>
#include <kdbopts.h>
#include <kdbgopts.h>

#include <elektra/conversion.h>

static KeySet * embeddedSpec (void)
{
	return ksNew (30,
	keyNew ("/", KEY_META, "mountpoint", "redshift.ecf", KEY_END),
	keyNew ("/adjustment/crtc", KEY_META, "default", "0", KEY_META, "description", "CRTC to apply adjustments to.", KEY_META, "example", "1", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "crtc", KEY_META, "type", "unsigned_short", KEY_END),
	keyNew ("/adjustment/drm/card", KEY_META, "default", "0", KEY_META, "description", "Graphics card to apply adjustments to.", KEY_META, "example", "1", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "drm-card", KEY_META, "type", "unsigned_short", KEY_END),
	keyNew ("/adjustment/method", KEY_META, "check/enum", "#7", KEY_META, "check/enum/#0", "drm", KEY_META, "check/enum/#1", "dummy", KEY_META, "check/enum/#2", "quartz", KEY_META, "check/enum/#3", "randr", KEY_META, "check/enum/#4", "vidmode", KEY_META, "check/enum/#5", "w32gdi", KEY_META, "check/enum/#6", "auto", KEY_META, "check/enum/#7", "list", KEY_META, "default", "auto", KEY_META, "description", "The method used to adjust screen color temperature. By default, one of the supported methods on the current OS will be chosen automatically. For details see section \"Alternative Features\" in file DESIGN in root directory.", KEY_META, "example", "randr", KEY_META, "opt", "m", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "method", KEY_META, "type", "enum", KEY_END),
	keyNew ("/adjustment/method/help", KEY_META, "default", "0", KEY_META, "description", "Prints the help of the adjustment methods.", KEY_META, "example", "1", KEY_META, "opt/arg", "none", KEY_META, "opt/long", "help-methods", KEY_META, "type", "boolean", KEY_END),
	keyNew ("/adjustment/screen", KEY_META, "default", "0", KEY_META, "description", "X screen to apply adjustments to.", KEY_META, "example", "1", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "screen", KEY_META, "type", "unsigned_short", KEY_END),
	keyNew ("/brightness/day", KEY_META, "check/range", "0-1", KEY_META, "check/type", "float", KEY_META, "default", "1.0", KEY_META, "description", "The screen brightness during daytime. If both day and night brightness are set, these will overrule the value of brightness.", KEY_META, "example", "0.8", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "brightness-day", KEY_META, "type", "float", KEY_END),
	keyNew ("/brightness/night", KEY_META, "check/range", "0-1", KEY_META, "check/type", "float", KEY_META, "default", "1.0", KEY_META, "description", "The screen brightness during nighttime. If both day and night brightness are set, these will overrule the value of brightness.", KEY_META, "example", "0.8", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "brightness-night", KEY_META, "type", "float", KEY_END),
	keyNew ("/fade/fast", KEY_META, "default", "0", KEY_META, "description", "Enable fast fades between color temperatures (e.g. from daytime to nighttime). When disabled, fades will be slow and more pleasant.", KEY_META, "example", "1", KEY_META, "opt", "f", KEY_META, "opt/arg", "none", KEY_META, "opt/long", "fade-fast", KEY_META, "type", "boolean", KEY_END),
	keyNew ("/gamma/day", KEY_META, "check/validation", "^([0-9]*[\\.,]\?[0-9]+)(:([0-9]*[\\.,]\?[0-9]+):([0-9]*[\\.,]\?[0-9]+))\?$", KEY_META, "check/validation/message", "The gamma value you provided is in an unsupported format. Supported formats are: 1. One value, that will be used for red, green and blue (e.g. 0.9). 2. Three colon-separated values for red, green and blue respectively (e.g. 0.9:0.9:0.9).", KEY_META, "default", "1.0:1.0:1.0", KEY_META, "description", "The screen gamma during daytime. Supported formats: 1. One value, that will be used for red, green and blue (e.g. 0.9). 2. Three colon-separated values for red, green and blue respectively (e.g. 0.9:0.9:0.9).", KEY_META, "example", "0.9", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "gamma-day", KEY_META, "type", "string", KEY_END),
	keyNew ("/gamma/night", KEY_META, "check/validation", "^([0-9]*[\\.,]\?[0-9]+)(:([0-9]*[\\.,]\?[0-9]+):([0-9]*[\\.,]\?[0-9]+))\?$", KEY_META, "check/validation/message", "The gamma value you provided is in an unsupported format. Supported formats are: 1. One value, that will be used for red, green and blue (e.g. 0.9). 2. Three colon-separated values for red, green and blue respectively (e.g. 0.9:0.9:0.9).", KEY_META, "default", "1.0:1.0:1.0", KEY_META, "description", "The screen gamma during nighttime. Supported formats: 1. One value, that will be used for red, green and blue (e.g. 0.9). 2. Three colon-separated values for red, green and blue respectively (e.g. 0.9:0.9:0.9).", KEY_META, "example", "0.9", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "gamma-night", KEY_META, "type", "string", KEY_END),
	keyNew ("/gamma/preserve", KEY_META, "default", "1", KEY_META, "description", "Use to preserve existing gamma ramps before applying adjustments.", KEY_META, "example", "0", KEY_META, "opt", "P", KEY_META, "opt/arg", "none", KEY_META, "type", "boolean", KEY_END),
	keyNew ("/help", KEY_META, "default", "0", KEY_META, "description", "Show program help.", KEY_META, "example", "1", KEY_META, "opt", "h", KEY_META, "opt/arg", "none", KEY_META, "type", "boolean", KEY_END),
	keyNew ("/mode", KEY_META, "check/enum", "#4", KEY_META, "check/enum/#0", "continual", KEY_META, "check/enum/#1", "print", KEY_META, "check/enum/#2", "oneshot", KEY_META, "check/enum/#3", "reset", KEY_META, "check/enum/#4", "oneshotmanual", KEY_META, "default", "continual", KEY_META, "description", "The program mode. \"continual\" will constantly adjust the screen color temperature using the configured \"provider\". \"print\" will just print parameters and exit. \"oneshot\" will set temperature once using the configured \"provider\". \"reset\" will remove any color temperature adjustments then exit. \"oneshotmanual\" will not use any provider to determine if it\'s night or day and set the temperature specified by option \"temp/oneshotmanual\" immediately.", KEY_META, "example", "oneshot", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "mode", KEY_META, "type", "enum", KEY_END),
	keyNew ("/provider", KEY_META, "check/enum", "#1", KEY_META, "check/enum/#0", "time", KEY_META, "check/enum/#1", "location", KEY_META, "default", "location", KEY_META, "description", "The provider used to decide at what times of day redshift should be enabled/disabled. Currently two options are supported: 1. location - determines the user\'s location and enable/disable redshift depending on the solar elevation. 2. time: Ignore user location and enable redshift if time is between dusk and dawn.", KEY_META, "example", "time", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "provider", KEY_META, "type", "enum", KEY_END),
	keyNew ("/provider/location", KEY_META, "check/enum", "#4", KEY_META, "check/enum/#0", "corelocation", KEY_META, "check/enum/#1", "geoclue2", KEY_META, "check/enum/#2", "manual", KEY_META, "check/enum/#3", "auto", KEY_META, "check/enum/#4", "list", KEY_META, "default", "auto", KEY_META, "description", "The location provider to be used. By default, one of the supported location providers on the current OS will be chosen automatically. The provider is used to establish whether it is currently daytime or nighttime.", KEY_META, "example", "geoclue2", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "location-provider", KEY_META, "type", "enum", KEY_END),
	keyNew ("/provider/location/elevation/high", KEY_META, "default", "3.0", KEY_META, "description", "By default, Redshift will use the current elevation of the sun to determine whether it is daytime, night or in transition (dawn/dusk). When the sun is above the degrees specified with the elevation-high key it is considered daytime and below the elevation-low key it is considered night (source: https://github.com/jonls/redshift/wiki/Configuration-file#solar-elevation-thresholds). Affects all location providers.", KEY_META, "example", "3.5", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "elevation-high", KEY_META, "type", "float", KEY_END),
	keyNew ("/provider/location/elevation/low", KEY_META, "default", "-6.0", KEY_META, "description", "By default, Redshift will use the current elevation of the sun to determine whether it is daytime, night or in transition (dawn/dusk). When the sun is above the degrees specified with the elevation-high key it is considered daytime and below the elevation-low key it is considered night (source: https://github.com/jonls/redshift/wiki/Configuration-file#solar-elevation-thresholds). Affects all location providers.", KEY_META, "example", "-5.0", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "elevation-low", KEY_META, "type", "float", KEY_END),
	keyNew ("/provider/location/help", KEY_META, "default", "0", KEY_META, "description", "Prints the help of the location providers.", KEY_META, "example", "1", KEY_META, "opt/arg", "none", KEY_META, "opt/long", "help-providers", KEY_META, "type", "boolean", KEY_END),
	keyNew ("/provider/location/manual/lat", KEY_META, "check/range", "-90.0-90.0", KEY_META, "check/type", "float", KEY_META, "default", "52.520008", KEY_META, "description", "The location latitude. Only applies to location provider \"manual\". Some locations (e.g. mainland USA) require negative values.", KEY_META, "example", "52.520008", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "lat", KEY_META, "type", "float", KEY_END),
	keyNew ("/provider/location/manual/lon", KEY_META, "check/range", "-180.0-180.0", KEY_META, "check/type", "float", KEY_META, "default", "13.404954", KEY_META, "description", "The location longitude. Only applies to location provider \"manual\". Some locations (e.g. parts of Africa) require negative values.", KEY_META, "example", "13.404954", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "lon", KEY_META, "type", "float", KEY_END),
	keyNew ("/provider/time/dawn/end", KEY_META, "check/date", "ISO8601", KEY_META, "check/date/format", "timeofday", KEY_META, "default", "06:30", KEY_META, "description", "Instead of using the solar elevation at the user\'s location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).", KEY_META, "example", "07:30", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "time-dawn-end", KEY_META, "type", "string", KEY_END),
	keyNew ("/provider/time/dawn/start", KEY_META, "check/date", "ISO8601", KEY_META, "check/date/format", "timeofday", KEY_META, "default", "05:00", KEY_META, "description", "Instead of using the solar elevation at the user\'s location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).", KEY_META, "example", "06:00", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "time-dawn-start", KEY_META, "type", "string", KEY_END),
	keyNew ("/provider/time/dusk/end", KEY_META, "check/date", "ISO8601", KEY_META, "check/date/format", "timeofday", KEY_META, "default", "20:30", KEY_META, "description", "Instead of using the solar elevation at the user\'s location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).", KEY_META, "example", "21:30", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "time-dusk-end", KEY_META, "type", "string", KEY_END),
	keyNew ("/provider/time/dusk/start", KEY_META, "check/date", "ISO8601", KEY_META, "check/date/format", "timeofday", KEY_META, "default", "19:00", KEY_META, "description", "Instead of using the solar elevation at the user\'s location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).", KEY_META, "example", "20:00", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "time-dusk-start", KEY_META, "type", "string", KEY_END),
	keyNew ("/temp/day", KEY_META, "check/range", "1000-25000", KEY_META, "check/type", "unsigned_short", KEY_META, "default", "6500", KEY_META, "description", "The color temperature the screen should have during daytime.", KEY_META, "example", "6500", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "temp-day", KEY_META, "type", "unsigned_short", KEY_END),
	keyNew ("/temp/night", KEY_META, "check/range", "1000-25000", KEY_META, "check/type", "unsigned_short", KEY_META, "default", "4500", KEY_META, "description", "The color temperature the screen should have during nighttime.", KEY_META, "example", "4500", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "temp-night", KEY_META, "type", "unsigned_short", KEY_END),
	keyNew ("/temp/oneshotmanual", KEY_META, "check/range", "1000-25000", KEY_META, "check/type", "unsigned_short", KEY_META, "default", "6500", KEY_META, "description", "The color temperature the screen should have when oneshotmanual mode is used.", KEY_META, "example", "6500", KEY_META, "opt/arg", "required", KEY_META, "opt/long", "temp-oneshotmanual", KEY_META, "type", "unsigned_short", KEY_END),
	keyNew ("/verbose", KEY_META, "default", "0", KEY_META, "description", "Verbose output.", KEY_META, "example", "1", KEY_META, "opt", "v", KEY_META, "opt/arg", "none", KEY_META, "type", "boolean", KEY_END),
	keyNew ("/version", KEY_META, "default", "0", KEY_META, "description", "Show program version.", KEY_META, "example", "1", KEY_META, "opt", "V", KEY_META, "opt/arg", "none", KEY_META, "type", "boolean", KEY_END),
	KS_END);
;
}

static const char * helpFallback = "Usage: redshift [OPTION...]\n\nOPTIONS\n  --help                      Print this help message\n  , --crtc=ARG                CRTC to apply adjustments to.\n  , --drm-card=ARG            Graphics card to apply adjustments to.\n  -m ARG, --method=ARG        The method used to adjust screen color temperature. By default, one of the supported methods on the current OS will be chosen automatically. For details see section \"Alternative Features\" in file DESIGN in root directory.\n  , --help-methods            Prints the help of the adjustment methods.\n  , --screen=ARG              X screen to apply adjustments to.\n  , --brightness-day=ARG      The screen brightness during daytime. If both day and night brightness are set, these will overrule the value of brightness.\n  , --brightness-night=ARG    The screen brightness during nighttime. If both day and night brightness are set, these will overrule the value of brightness.\n  -f, --fade-fast             Enable fast fades between color temperatures (e.g. from daytime to nighttime). When disabled, fades will be slow and more pleasant.\n  , --gamma-day=ARG           The screen gamma during daytime. Supported formats: 1. One value, that will be used for red, green and blue (e.g. 0.9). 2. Three colon-separated values for red, green and blue respectively (e.g. 0.9:0.9:0.9).\n  , --gamma-night=ARG         The screen gamma during nighttime. Supported formats: 1. One value, that will be used for red, green and blue (e.g. 0.9). 2. Three colon-separated values for red, green and blue respectively (e.g. 0.9:0.9:0.9).\n  -P                          Use to preserve existing gamma ramps before applying adjustments.\n  -h                          Show program help.\n  , --mode=ARG                The program mode. \"continual\" will constantly adjust the screen color temperature using the configured \"provider\". \"print\" will just print parameters and exit. \"oneshot\" will set temperature once using the configured \"provider\". \"reset\" will remove any color temperature adjustments then exit. \"oneshotmanual\" will not use any provider to determine if it\'s night or day and set the temperature specified by option \"temp/oneshotmanual\" immediately.\n  , --provider=ARG            The provider used to decide at what times of day redshift should be enabled/disabled. Currently two options are supported: 1. location - determines the user\'s location and enable/disable redshift depending on the solar elevation. 2. time: Ignore user location and enable redshift if time is between dusk and dawn.\n  , --location-provider=ARG   The location provider to be used. By default, one of the supported location providers on the current OS will be chosen automatically. The provider is used to establish whether it is currently daytime or nighttime.\n  , --elevation-high=ARG      By default, Redshift will use the current elevation of the sun to determine whether it is daytime, night or in transition (dawn/dusk). When the sun is above the degrees specified with the elevation-high key it is considered daytime and below the elevation-low key it is considered night (source: https://github.com/jonls/redshift/wiki/Configuration-file#solar-elevation-thresholds). Affects all location providers.\n  , --elevation-low=ARG       By default, Redshift will use the current elevation of the sun to determine whether it is daytime, night or in transition (dawn/dusk). When the sun is above the degrees specified with the elevation-high key it is considered daytime and below the elevation-low key it is considered night (source: https://github.com/jonls/redshift/wiki/Configuration-file#solar-elevation-thresholds). Affects all location providers.\n  , --help-providers          Prints the help of the location providers.\n  , --lat=ARG                 The location latitude. Only applies to location provider \"manual\". Some locations (e.g. mainland USA) require negative values.\n  , --lon=ARG                 The location longitude. Only applies to location provider \"manual\". Some locations (e.g. parts of Africa) require negative values.\n  , --time-dawn-end=ARG       Instead of using the solar elevation at the user\'s location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).\n  , --time-dawn-start=ARG     Instead of using the solar elevation at the user\'s location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).\n  , --time-dusk-end=ARG       Instead of using the solar elevation at the user\'s location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).\n  , --time-dusk-start=ARG     Instead of using the solar elevation at the user\'s location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).\n  , --temp-day=ARG            The color temperature the screen should have during daytime.\n  , --temp-night=ARG          The color temperature the screen should have during nighttime.\n  , --temp-oneshotmanual=ARG  The color temperature the screen should have when oneshotmanual mode is used.\n  -v                          Verbose output.\n  -V                          Show program version.\n";

static int isHelpMode (int argc, const char * const * argv)
{
	for (int i = 0; i < argc; ++i)
	{
		if (strcmp (argv[i], "--help") == 0)
		{
			return 1;
		}
	}

	return 0;
}



/**
 * Initializes an instance of Elektra for the application '/sw/jonls/redshift/#0/current'.
 *
 * This can be invoked as many times as you want, however it is not a cheap operation,
 * so you should try to reuse the Elektra handle as much as possible.
 *
 * @param elektra A reference to where the Elektra instance shall be stored.
 *                Has to be disposed of with elektraClose().
 * @param error   A reference to an ElektraError pointer. Will be passed to elektraOpen().
 *
 * @retval 0  on success, @p elektra will contain a new Elektra instance coming from elektraOpen(),
 *            @p error will be unchanged
 * @retval -1 on error, @p elektra will be unchanged, @p error will be set
 * @retval 1  help mode, '--help' was specified call printHelpMessage to display
 *            the help message. @p elektra will contain a new Elektra instance. It has to be passed
 *            to printHelpMessage. You also need to elektraClose() it.
 *            @p error will be unchanged
 *
 * @see elektraOpen
 */// 
int loadConfiguration (Elektra ** elektra, 
				 int argc, const char * const * argv, const char * const * envp,
				 ElektraError ** error)
{
	KeySet * defaults = embeddedSpec ();
	

	KeySet * contract = ksNew (4,
	keyNew ("system:/elektra/contract/highlevel/check/spec/mounted", KEY_VALUE, "1", KEY_END),
	keyNew ("system:/elektra/contract/highlevel/check/spec/token", KEY_VALUE, "b3d73b67ece57190da2094a91b17bcbf3a37fb09923ddbf60db79654722aab1f", KEY_END),
	keyNew ("system:/elektra/contract/highlevel/helpmode/ignore/require", KEY_VALUE, "1", KEY_END),
	keyNew ("system:/elektra/contract/mountglobal/gopts", KEY_END),
	KS_END);
;
	Key * parentKey = keyNew ("/sw/jonls/redshift/#0/current", KEY_END);

	elektraGOptsContract (contract, argc, argv, envp, parentKey, NULL);
	

	keyDel (parentKey);

	Elektra * e = elektraOpen ("/sw/jonls/redshift/#0/current", defaults, contract, error);

	if (defaults != NULL)
	{
		ksDel (defaults);
	}

	if (contract != NULL)
	{
		ksDel (contract);
	}

	if (e == NULL)
	{
		*elektra = NULL;
		if (isHelpMode (argc, argv))
		{
			elektraErrorReset (error);
			return 1;
		}
		

		return -1;
	}

	*elektra = e;
	return elektraHelpKey (e) != NULL && strcmp (keyString (elektraHelpKey (e)), "1") == 0 ? 1 : 0;
}

/**
 * Checks whether specload mode was invoked and if so, sends the specification over stdout
 * in the format expected by specload.
 *
 * You MUST not output anything to stdout before invoking this function. Ideally invoking this
 * is the first thing you do in your main()-function.
 *
 * This function will ONLY RETURN, if specload mode was NOT invoked. Otherwise it will call `exit()`.
 *
 * @param argc pass the value of argc from main
 * @param argv pass the value of argv from main
 */
void exitForSpecload (int argc, const char * const * argv)
{
	if (argc != 2 || strcmp (argv[1], "--elektra-spec") != 0)
	{
		return;
	}

	KeySet * spec = embeddedSpec ();

	Key * parentKey = keyNew ("spec:/sw/jonls/redshift/#0/current", KEY_META, "system:/elektra/quickdump/noparent", "", KEY_END);

	KeySet * specloadConf = ksNew (1, keyNew ("system:/sendspec", KEY_END), KS_END);
	ElektraInvokeHandle * specload = elektraInvokeOpen ("specload", specloadConf, parentKey);

	int result = elektraInvoke2Args (specload, "sendspec", spec, parentKey);

	elektraInvokeClose (specload, parentKey);
	keyDel (parentKey);
	ksDel (specloadConf);
	ksDel (spec);

	exit (result == ELEKTRA_PLUGIN_STATUS_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE);
}


/**
 * Outputs the help message to stdout
 *
 * @param elektra  The Elektra instance produced by loadConfiguration.
 * @param usage	   If this is not NULL, it will be used instead of the default usage line.
 * @param prefix   If this is not NULL, it will be inserted between the usage line and the options list.
 */// 
void printHelpMessage (Elektra * elektra, const char * usage, const char * prefix)
{
	if (elektra == NULL)
	{
		printf ("%s", helpFallback);
		return;
	}

	Key * helpKey = elektraHelpKey (elektra);
	if (helpKey == NULL)
	{
		return;
	}

	char * help = elektraGetOptsHelpMessage (helpKey, usage, prefix);
	printf ("%s", help);
	elektraFree (help);
}



// clang-format off

// clang-format on

// -------------------------
// Enum conversion functions
// -------------------------

ELEKTRA_KEY_TO_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod)
{
	const char * string;
	if (!elektraKeyToString (key, &string) || strlen (string) == 0)
	{
		return 0;
	}

	
	if (strcmp (string, "drm") == 0)
	{
		*variable = ELEKTRA_ENUM_ADJUSTMENT_METHOD_DRM;
		return 1;
	}
	if (strcmp (string, "dummy") == 0)
	{
		*variable = ELEKTRA_ENUM_ADJUSTMENT_METHOD_DUMMY;
		return 1;
	}
	if (strcmp (string, "quartz") == 0)
	{
		*variable = ELEKTRA_ENUM_ADJUSTMENT_METHOD_QUARTZ;
		return 1;
	}
	if (strcmp (string, "randr") == 0)
	{
		*variable = ELEKTRA_ENUM_ADJUSTMENT_METHOD_RANDR;
		return 1;
	}
	if (strcmp (string, "vidmode") == 0)
	{
		*variable = ELEKTRA_ENUM_ADJUSTMENT_METHOD_VIDMODE;
		return 1;
	}
	if (strcmp (string, "w32gdi") == 0)
	{
		*variable = ELEKTRA_ENUM_ADJUSTMENT_METHOD_W32GDI;
		return 1;
	}
	if (strcmp (string, "auto") == 0)
	{
		*variable = ELEKTRA_ENUM_ADJUSTMENT_METHOD_AUTO;
		return 1;
	}
	if (strcmp (string, "list") == 0)
	{
		*variable = ELEKTRA_ENUM_ADJUSTMENT_METHOD_LIST;
		return 1;
	}

	return 0;
}

ELEKTRA_TO_STRING_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod)
{
	switch (value)
	{
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_DRM:
		return elektraStrDup ("drm");
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_DUMMY:
		return elektraStrDup ("dummy");
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_QUARTZ:
		return elektraStrDup ("quartz");
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_RANDR:
		return elektraStrDup ("randr");
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_VIDMODE:
		return elektraStrDup ("vidmode");
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_W32GDI:
		return elektraStrDup ("w32gdi");
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_AUTO:
		return elektraStrDup ("auto");
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_LIST:
		return elektraStrDup ("list");
	}

	// should be unreachable
	return elektraStrDup ("");
}

ELEKTRA_TO_CONST_STRING_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod)
{
	switch (value)
	{
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_DRM:
		return "drm";
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_DUMMY:
		return "dummy";
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_QUARTZ:
		return "quartz";
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_RANDR:
		return "randr";
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_VIDMODE:
		return "vidmode";
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_W32GDI:
		return "w32gdi";
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_AUTO:
		return "auto";
	case ELEKTRA_ENUM_ADJUSTMENT_METHOD_LIST:
		return "list";
	}

	// should be unreachable
	return "";
}
ELEKTRA_KEY_TO_SIGNATURE (ElektraEnumMode, EnumMode)
{
	const char * string;
	if (!elektraKeyToString (key, &string) || strlen (string) == 0)
	{
		return 0;
	}

	
	if (strcmp (string, "continual") == 0)
	{
		*variable = ELEKTRA_ENUM_MODE_CONTINUAL;
		return 1;
	}
	if (strcmp (string, "print") == 0)
	{
		*variable = ELEKTRA_ENUM_MODE_PRINT;
		return 1;
	}
	if (strcmp (string, "oneshot") == 0)
	{
		*variable = ELEKTRA_ENUM_MODE_ONESHOT;
		return 1;
	}
	if (strcmp (string, "reset") == 0)
	{
		*variable = ELEKTRA_ENUM_MODE_RESET;
		return 1;
	}
	if (strcmp (string, "oneshotmanual") == 0)
	{
		*variable = ELEKTRA_ENUM_MODE_ONESHOTMANUAL;
		return 1;
	}

	return 0;
}

ELEKTRA_TO_STRING_SIGNATURE (ElektraEnumMode, EnumMode)
{
	switch (value)
	{
	case ELEKTRA_ENUM_MODE_CONTINUAL:
		return elektraStrDup ("continual");
	case ELEKTRA_ENUM_MODE_PRINT:
		return elektraStrDup ("print");
	case ELEKTRA_ENUM_MODE_ONESHOT:
		return elektraStrDup ("oneshot");
	case ELEKTRA_ENUM_MODE_RESET:
		return elektraStrDup ("reset");
	case ELEKTRA_ENUM_MODE_ONESHOTMANUAL:
		return elektraStrDup ("oneshotmanual");
	}

	// should be unreachable
	return elektraStrDup ("");
}

ELEKTRA_TO_CONST_STRING_SIGNATURE (ElektraEnumMode, EnumMode)
{
	switch (value)
	{
	case ELEKTRA_ENUM_MODE_CONTINUAL:
		return "continual";
	case ELEKTRA_ENUM_MODE_PRINT:
		return "print";
	case ELEKTRA_ENUM_MODE_ONESHOT:
		return "oneshot";
	case ELEKTRA_ENUM_MODE_RESET:
		return "reset";
	case ELEKTRA_ENUM_MODE_ONESHOTMANUAL:
		return "oneshotmanual";
	}

	// should be unreachable
	return "";
}
ELEKTRA_KEY_TO_SIGNATURE (ElektraEnumProvider, EnumProvider)
{
	const char * string;
	if (!elektraKeyToString (key, &string) || strlen (string) == 0)
	{
		return 0;
	}

	switch (string[0])
{
case 'l':
*variable = ELEKTRA_ENUM_PROVIDER_LOCATION;
return 1;
case 't':
*variable = ELEKTRA_ENUM_PROVIDER_TIME;
return 1;
}

	

	return 0;
}

ELEKTRA_TO_STRING_SIGNATURE (ElektraEnumProvider, EnumProvider)
{
	switch (value)
	{
	case ELEKTRA_ENUM_PROVIDER_TIME:
		return elektraStrDup ("time");
	case ELEKTRA_ENUM_PROVIDER_LOCATION:
		return elektraStrDup ("location");
	}

	// should be unreachable
	return elektraStrDup ("");
}

ELEKTRA_TO_CONST_STRING_SIGNATURE (ElektraEnumProvider, EnumProvider)
{
	switch (value)
	{
	case ELEKTRA_ENUM_PROVIDER_TIME:
		return "time";
	case ELEKTRA_ENUM_PROVIDER_LOCATION:
		return "location";
	}

	// should be unreachable
	return "";
}
ELEKTRA_KEY_TO_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation)
{
	const char * string;
	if (!elektraKeyToString (key, &string) || strlen (string) == 0)
	{
		return 0;
	}

	switch (string[0])
{
case 'a':
*variable = ELEKTRA_ENUM_PROVIDER_LOCATION_AUTO;
return 1;
case 'c':
*variable = ELEKTRA_ENUM_PROVIDER_LOCATION_CORELOCATION;
return 1;
case 'g':
*variable = ELEKTRA_ENUM_PROVIDER_LOCATION_GEOCLUE2;
return 1;
case 'l':
*variable = ELEKTRA_ENUM_PROVIDER_LOCATION_LIST;
return 1;
case 'm':
*variable = ELEKTRA_ENUM_PROVIDER_LOCATION_MANUAL;
return 1;
}

	

	return 0;
}

ELEKTRA_TO_STRING_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation)
{
	switch (value)
	{
	case ELEKTRA_ENUM_PROVIDER_LOCATION_CORELOCATION:
		return elektraStrDup ("corelocation");
	case ELEKTRA_ENUM_PROVIDER_LOCATION_GEOCLUE2:
		return elektraStrDup ("geoclue2");
	case ELEKTRA_ENUM_PROVIDER_LOCATION_MANUAL:
		return elektraStrDup ("manual");
	case ELEKTRA_ENUM_PROVIDER_LOCATION_AUTO:
		return elektraStrDup ("auto");
	case ELEKTRA_ENUM_PROVIDER_LOCATION_LIST:
		return elektraStrDup ("list");
	}

	// should be unreachable
	return elektraStrDup ("");
}

ELEKTRA_TO_CONST_STRING_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation)
{
	switch (value)
	{
	case ELEKTRA_ENUM_PROVIDER_LOCATION_CORELOCATION:
		return "corelocation";
	case ELEKTRA_ENUM_PROVIDER_LOCATION_GEOCLUE2:
		return "geoclue2";
	case ELEKTRA_ENUM_PROVIDER_LOCATION_MANUAL:
		return "manual";
	case ELEKTRA_ENUM_PROVIDER_LOCATION_AUTO:
		return "auto";
	case ELEKTRA_ENUM_PROVIDER_LOCATION_LIST:
		return "list";
	}

	// should be unreachable
	return "";
}

// -------------------------
// Enum accessor functions
// -------------------------

ELEKTRA_GET_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod)
{
	ElektraEnumAdjustmentMethod result;
	const Key * key = elektraFindKey (elektra, keyname, KDB_TYPE_ENUM);
	if (!ELEKTRA_KEY_TO (EnumAdjustmentMethod) (key, &result))
	{
		elektraFatalError (elektra, elektraErrorConversionFromString (KDB_TYPE_ENUM, keyname, keyString (key)));
		return (ElektraEnumAdjustmentMethod) 0;
	}
	return result;
}

ELEKTRA_GET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod)
{
	ElektraEnumAdjustmentMethod result;
	const Key * key = elektraFindArrayElementKey (elektra, keyname, index, KDB_TYPE_ENUM);
	if (!ELEKTRA_KEY_TO (EnumAdjustmentMethod) (key, &result))
	{
		elektraFatalError (elektra, elektraErrorConversionFromString (KDB_TYPE_ENUM, keyname, keyString (key)));
		return (ElektraEnumAdjustmentMethod) 0;
	}
	return result;
}

ELEKTRA_SET_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod)
{
	char * string = ELEKTRA_TO_STRING (EnumAdjustmentMethod) (value);
	if (string == 0)
	{
		*error = elektraErrorConversionToString (KDB_TYPE_ENUM, keyname);
		return;
	}
	elektraSetRawString (elektra, keyname, string, KDB_TYPE_ENUM, error);
	elektraFree (string);
}

ELEKTRA_SET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod)
{
	char * string = ELEKTRA_TO_STRING (EnumAdjustmentMethod) (value);
	if (string == 0)
	{
		*error = elektraErrorConversionToString (KDB_TYPE_ENUM, keyname);
		return;
	}
	elektraSetRawStringArrayElement (elektra, keyname, index, string, KDB_TYPE_ENUM, error);
	elektraFree (string);
}
ELEKTRA_GET_SIGNATURE (ElektraEnumMode, EnumMode)
{
	ElektraEnumMode result;
	const Key * key = elektraFindKey (elektra, keyname, KDB_TYPE_ENUM);
	if (!ELEKTRA_KEY_TO (EnumMode) (key, &result))
	{
		elektraFatalError (elektra, elektraErrorConversionFromString (KDB_TYPE_ENUM, keyname, keyString (key)));
		return (ElektraEnumMode) 0;
	}
	return result;
}

ELEKTRA_GET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumMode, EnumMode)
{
	ElektraEnumMode result;
	const Key * key = elektraFindArrayElementKey (elektra, keyname, index, KDB_TYPE_ENUM);
	if (!ELEKTRA_KEY_TO (EnumMode) (key, &result))
	{
		elektraFatalError (elektra, elektraErrorConversionFromString (KDB_TYPE_ENUM, keyname, keyString (key)));
		return (ElektraEnumMode) 0;
	}
	return result;
}

ELEKTRA_SET_SIGNATURE (ElektraEnumMode, EnumMode)
{
	char * string = ELEKTRA_TO_STRING (EnumMode) (value);
	if (string == 0)
	{
		*error = elektraErrorConversionToString (KDB_TYPE_ENUM, keyname);
		return;
	}
	elektraSetRawString (elektra, keyname, string, KDB_TYPE_ENUM, error);
	elektraFree (string);
}

ELEKTRA_SET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumMode, EnumMode)
{
	char * string = ELEKTRA_TO_STRING (EnumMode) (value);
	if (string == 0)
	{
		*error = elektraErrorConversionToString (KDB_TYPE_ENUM, keyname);
		return;
	}
	elektraSetRawStringArrayElement (elektra, keyname, index, string, KDB_TYPE_ENUM, error);
	elektraFree (string);
}
ELEKTRA_GET_SIGNATURE (ElektraEnumProvider, EnumProvider)
{
	ElektraEnumProvider result;
	const Key * key = elektraFindKey (elektra, keyname, KDB_TYPE_ENUM);
	if (!ELEKTRA_KEY_TO (EnumProvider) (key, &result))
	{
		elektraFatalError (elektra, elektraErrorConversionFromString (KDB_TYPE_ENUM, keyname, keyString (key)));
		return (ElektraEnumProvider) 0;
	}
	return result;
}

ELEKTRA_GET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumProvider, EnumProvider)
{
	ElektraEnumProvider result;
	const Key * key = elektraFindArrayElementKey (elektra, keyname, index, KDB_TYPE_ENUM);
	if (!ELEKTRA_KEY_TO (EnumProvider) (key, &result))
	{
		elektraFatalError (elektra, elektraErrorConversionFromString (KDB_TYPE_ENUM, keyname, keyString (key)));
		return (ElektraEnumProvider) 0;
	}
	return result;
}

ELEKTRA_SET_SIGNATURE (ElektraEnumProvider, EnumProvider)
{
	char * string = ELEKTRA_TO_STRING (EnumProvider) (value);
	if (string == 0)
	{
		*error = elektraErrorConversionToString (KDB_TYPE_ENUM, keyname);
		return;
	}
	elektraSetRawString (elektra, keyname, string, KDB_TYPE_ENUM, error);
	elektraFree (string);
}

ELEKTRA_SET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumProvider, EnumProvider)
{
	char * string = ELEKTRA_TO_STRING (EnumProvider) (value);
	if (string == 0)
	{
		*error = elektraErrorConversionToString (KDB_TYPE_ENUM, keyname);
		return;
	}
	elektraSetRawStringArrayElement (elektra, keyname, index, string, KDB_TYPE_ENUM, error);
	elektraFree (string);
}
ELEKTRA_GET_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation)
{
	ElektraEnumProviderLocation result;
	const Key * key = elektraFindKey (elektra, keyname, KDB_TYPE_ENUM);
	if (!ELEKTRA_KEY_TO (EnumProviderLocation) (key, &result))
	{
		elektraFatalError (elektra, elektraErrorConversionFromString (KDB_TYPE_ENUM, keyname, keyString (key)));
		return (ElektraEnumProviderLocation) 0;
	}
	return result;
}

ELEKTRA_GET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation)
{
	ElektraEnumProviderLocation result;
	const Key * key = elektraFindArrayElementKey (elektra, keyname, index, KDB_TYPE_ENUM);
	if (!ELEKTRA_KEY_TO (EnumProviderLocation) (key, &result))
	{
		elektraFatalError (elektra, elektraErrorConversionFromString (KDB_TYPE_ENUM, keyname, keyString (key)));
		return (ElektraEnumProviderLocation) 0;
	}
	return result;
}

ELEKTRA_SET_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation)
{
	char * string = ELEKTRA_TO_STRING (EnumProviderLocation) (value);
	if (string == 0)
	{
		*error = elektraErrorConversionToString (KDB_TYPE_ENUM, keyname);
		return;
	}
	elektraSetRawString (elektra, keyname, string, KDB_TYPE_ENUM, error);
	elektraFree (string);
}

ELEKTRA_SET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation)
{
	char * string = ELEKTRA_TO_STRING (EnumProviderLocation) (value);
	if (string == 0)
	{
		*error = elektraErrorConversionToString (KDB_TYPE_ENUM, keyname);
		return;
	}
	elektraSetRawStringArrayElement (elektra, keyname, index, string, KDB_TYPE_ENUM, error);
	elektraFree (string);
}


// clang-format off

// clang-format on

// -------------------------
// Union accessor functions
// -------------------------




// clang-format off

// clang-format on

// -------------------------
// Struct accessor functions
// -------------------------



