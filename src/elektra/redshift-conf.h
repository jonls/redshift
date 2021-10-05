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


#ifndef REDSHIFT_CONF_H
#define REDSHIFT_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <elektra.h>

#include <kdbhelper.h>
#include <string.h>





// clang-format off

// clang-format on

typedef enum
{
	ELEKTRA_ENUM_ADJUSTMENT_METHOD_DRM = 0,
	ELEKTRA_ENUM_ADJUSTMENT_METHOD_DUMMY = 1,
	ELEKTRA_ENUM_ADJUSTMENT_METHOD_QUARTZ = 2,
	ELEKTRA_ENUM_ADJUSTMENT_METHOD_RANDR = 3,
	ELEKTRA_ENUM_ADJUSTMENT_METHOD_VIDMODE = 4,
	ELEKTRA_ENUM_ADJUSTMENT_METHOD_W32GDI = 5,
	ELEKTRA_ENUM_ADJUSTMENT_METHOD_AUTO = 6,
	ELEKTRA_ENUM_ADJUSTMENT_METHOD_LIST = 7,
} ElektraEnumAdjustmentMethod;

typedef enum
{
	ELEKTRA_ENUM_MODE_CONTINUAL = 0,
	ELEKTRA_ENUM_MODE_PRINT = 1,
	ELEKTRA_ENUM_MODE_ONESHOT = 2,
	ELEKTRA_ENUM_MODE_RESET = 3,
	ELEKTRA_ENUM_MODE_ONESHOTMANUAL = 4,
} ElektraEnumMode;

typedef enum
{
	ELEKTRA_ENUM_PROVIDER_TIME = 0,
	ELEKTRA_ENUM_PROVIDER_LOCATION = 1,
} ElektraEnumProvider;

typedef enum
{
	ELEKTRA_ENUM_PROVIDER_LOCATION_CORELOCATION = 0,
	ELEKTRA_ENUM_PROVIDER_LOCATION_GEOCLUE2 = 1,
	ELEKTRA_ENUM_PROVIDER_LOCATION_MANUAL = 2,
	ELEKTRA_ENUM_PROVIDER_LOCATION_AUTO = 3,
	ELEKTRA_ENUM_PROVIDER_LOCATION_LIST = 4,
} ElektraEnumProviderLocation;


#define ELEKTRA_TO_CONST_STRING(typeName) ELEKTRA_CONCAT (ELEKTRA_CONCAT (elektra, typeName), ToConstString)
#define ELEKTRA_TO_CONST_STRING_SIGNATURE(cType, typeName) const char * ELEKTRA_TO_CONST_STRING (typeName) (cType value)

ELEKTRA_KEY_TO_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod);
ELEKTRA_TO_STRING_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod);
ELEKTRA_TO_CONST_STRING_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod);

ELEKTRA_GET_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod);
ELEKTRA_GET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod);
ELEKTRA_SET_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod);
ELEKTRA_SET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumAdjustmentMethod, EnumAdjustmentMethod);

ELEKTRA_KEY_TO_SIGNATURE (ElektraEnumMode, EnumMode);
ELEKTRA_TO_STRING_SIGNATURE (ElektraEnumMode, EnumMode);
ELEKTRA_TO_CONST_STRING_SIGNATURE (ElektraEnumMode, EnumMode);

ELEKTRA_GET_SIGNATURE (ElektraEnumMode, EnumMode);
ELEKTRA_GET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumMode, EnumMode);
ELEKTRA_SET_SIGNATURE (ElektraEnumMode, EnumMode);
ELEKTRA_SET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumMode, EnumMode);

ELEKTRA_KEY_TO_SIGNATURE (ElektraEnumProvider, EnumProvider);
ELEKTRA_TO_STRING_SIGNATURE (ElektraEnumProvider, EnumProvider);
ELEKTRA_TO_CONST_STRING_SIGNATURE (ElektraEnumProvider, EnumProvider);

ELEKTRA_GET_SIGNATURE (ElektraEnumProvider, EnumProvider);
ELEKTRA_GET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumProvider, EnumProvider);
ELEKTRA_SET_SIGNATURE (ElektraEnumProvider, EnumProvider);
ELEKTRA_SET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumProvider, EnumProvider);

ELEKTRA_KEY_TO_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation);
ELEKTRA_TO_STRING_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation);
ELEKTRA_TO_CONST_STRING_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation);

ELEKTRA_GET_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation);
ELEKTRA_GET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation);
ELEKTRA_SET_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation);
ELEKTRA_SET_ARRAY_ELEMENT_SIGNATURE (ElektraEnumProviderLocation, EnumProviderLocation);



// clang-format off

// clang-format on

#define ELEKTRA_UNION_FREE(typeName) ELEKTRA_CONCAT (elektraFree, typeName)
#define ELEKTRA_UNION_FREE_SIGNATURE(cType, typeName, discrType) void ELEKTRA_UNION_FREE (typeName) (cType * ptr, discrType discriminator)

#define ELEKTRA_UNION_GET_SIGNATURE(cType, typeName, discrType)                                                                            \
	cType ELEKTRA_GET (typeName) (Elektra * elektra, const char * keyname, discrType discriminator)
#define ELEKTRA_UNION_GET_ARRAY_ELEMENT_SIGNATURE(cType, typeName, discrType)                                                              \
	cType ELEKTRA_GET_ARRAY_ELEMENT (typeName) (Elektra * elektra, const char * keyname, kdb_long_long_t index, discrType discriminator)
#define ELEKTRA_UNION_SET_SIGNATURE(cType, typeName, discrType)                                                                            \
	void ELEKTRA_SET (typeName) (Elektra * elektra, const char * keyname, cType value, discrType discriminator, ElektraError ** error)
#define ELEKTRA_UNION_SET_ARRAY_ELEMENT_SIGNATURE(cType, typeName, discrType)                                                              \
	void ELEKTRA_SET_ARRAY_ELEMENT (typeName) (Elektra * elektra, const char * keyname, kdb_long_long_t index, cType value,            \
						   discrType discriminator, ElektraError ** error)






// clang-format off

// clang-format on

#define ELEKTRA_STRUCT_FREE(typeName) ELEKTRA_CONCAT (elektraFree, typeName)
#define ELEKTRA_STRUCT_FREE_SIGNATURE(cType, typeName) void ELEKTRA_STRUCT_FREE (typeName) (cType * ptr)






// clang-format off

// clang-format on

// clang-format off

/**
* Tag name for 'adjustment/crtc'
* 
*/// 
#define ELEKTRA_TAG_ADJUSTMENT_CRTC AdjustmentCrtc

/**
* Tag name for 'adjustment/drm/card'
* 
*/// 
#define ELEKTRA_TAG_ADJUSTMENT_DRM_CARD AdjustmentDrmCard

/**
* Tag name for 'adjustment/method'
* 
*/// 
#define ELEKTRA_TAG_ADJUSTMENT_METHOD AdjustmentMethod

/**
* Tag name for 'adjustment/method/help'
* 
*/// 
#define ELEKTRA_TAG_ADJUSTMENT_METHOD_HELP AdjustmentMethodHelp

/**
* Tag name for 'adjustment/screen'
* 
*/// 
#define ELEKTRA_TAG_ADJUSTMENT_SCREEN AdjustmentScreen

/**
* Tag name for 'brightness/day'
* 
*/// 
#define ELEKTRA_TAG_BRIGHTNESS_DAY BrightnessDay

/**
* Tag name for 'brightness/night'
* 
*/// 
#define ELEKTRA_TAG_BRIGHTNESS_NIGHT BrightnessNight

/**
* Tag name for 'fade/fast'
* 
*/// 
#define ELEKTRA_TAG_FADE_FAST FadeFast

/**
* Tag name for 'gamma/day'
* 
*/// 
#define ELEKTRA_TAG_GAMMA_DAY GammaDay

/**
* Tag name for 'gamma/night'
* 
*/// 
#define ELEKTRA_TAG_GAMMA_NIGHT GammaNight

/**
* Tag name for 'gamma/preserve'
* 
*/// 
#define ELEKTRA_TAG_GAMMA_PRESERVE GammaPreserve

/**
* Tag name for 'help'
* 
*/// 
#define ELEKTRA_TAG_HELP Help

/**
* Tag name for 'mode'
* 
*/// 
#define ELEKTRA_TAG_MODE Mode

/**
* Tag name for 'provider'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER Provider

/**
* Tag name for 'provider/location'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER_LOCATION ProviderLocation

/**
* Tag name for 'provider/location/elevation/high'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER_LOCATION_ELEVATION_HIGH ProviderLocationElevationHigh

/**
* Tag name for 'provider/location/elevation/low'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER_LOCATION_ELEVATION_LOW ProviderLocationElevationLow

/**
* Tag name for 'provider/location/help'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER_LOCATION_HELP ProviderLocationHelp

/**
* Tag name for 'provider/location/manual/lat'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER_LOCATION_MANUAL_LAT ProviderLocationManualLat

/**
* Tag name for 'provider/location/manual/lon'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER_LOCATION_MANUAL_LON ProviderLocationManualLon

/**
* Tag name for 'provider/time/dawn/end'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER_TIME_DAWN_END ProviderTimeDawnEnd

/**
* Tag name for 'provider/time/dawn/start'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER_TIME_DAWN_START ProviderTimeDawnStart

/**
* Tag name for 'provider/time/dusk/end'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER_TIME_DUSK_END ProviderTimeDuskEnd

/**
* Tag name for 'provider/time/dusk/start'
* 
*/// 
#define ELEKTRA_TAG_PROVIDER_TIME_DUSK_START ProviderTimeDuskStart

/**
* Tag name for 'temp/day'
* 
*/// 
#define ELEKTRA_TAG_TEMP_DAY TempDay

/**
* Tag name for 'temp/night'
* 
*/// 
#define ELEKTRA_TAG_TEMP_NIGHT TempNight

/**
* Tag name for 'temp/oneshotmanual'
* 
*/// 
#define ELEKTRA_TAG_TEMP_ONESHOTMANUAL TempOneshotmanual

/**
* Tag name for 'verbose'
* 
*/// 
#define ELEKTRA_TAG_VERBOSE Verbose

/**
* Tag name for 'version'
* 
*/// 
#define ELEKTRA_TAG_VERSION Version
// clang-format on


// clang-format off

// clang-format on

// local helper macros to determine the length of a 64 bit integer
#define elektra_len19(x) ((x) < 10000000000000000000ULL ? 19 : 20)
#define elektra_len18(x) ((x) < 1000000000000000000ULL ? 18 : elektra_len19 (x))
#define elektra_len17(x) ((x) < 100000000000000000ULL ? 17 : elektra_len18 (x))
#define elektra_len16(x) ((x) < 10000000000000000ULL ? 16 : elektra_len17 (x))
#define elektra_len15(x) ((x) < 1000000000000000ULL ? 15 : elektra_len16 (x))
#define elektra_len14(x) ((x) < 100000000000000ULL ? 14 : elektra_len15 (x))
#define elektra_len13(x) ((x) < 10000000000000ULL ? 13 : elektra_len14 (x))
#define elektra_len12(x) ((x) < 1000000000000ULL ? 12 : elektra_len13 (x))
#define elektra_len11(x) ((x) < 100000000000ULL ? 11 : elektra_len12 (x))
#define elektra_len10(x) ((x) < 10000000000ULL ? 10 : elektra_len11 (x))
#define elektra_len09(x) ((x) < 1000000000ULL ? 9 : elektra_len10 (x))
#define elektra_len08(x) ((x) < 100000000ULL ? 8 : elektra_len09 (x))
#define elektra_len07(x) ((x) < 10000000ULL ? 7 : elektra_len08 (x))
#define elektra_len06(x) ((x) < 1000000ULL ? 6 : elektra_len07 (x))
#define elektra_len05(x) ((x) < 100000ULL ? 5 : elektra_len06 (x))
#define elektra_len04(x) ((x) < 10000ULL ? 4 : elektra_len05 (x))
#define elektra_len03(x) ((x) < 1000ULL ? 3 : elektra_len04 (x))
#define elektra_len02(x) ((x) < 100ULL ? 2 : elektra_len03 (x))
#define elektra_len01(x) ((x) < 10ULL ? 1 : elektra_len02 (x))
#define elektra_len00(x) ((x) < 0ULL ? 0 : elektra_len01 (x))
#define elektra_len(x) elektra_len00 (x)

#define ELEKTRA_SIZE(tagName) ELEKTRA_CONCAT (elektraSize, tagName)




/**
 * Get the value of key 'adjustment/crtc' (tag #ELEKTRA_TAG_ADJUSTMENT_CRTC).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'adjustment/crtc'.

 */// 
static inline kdb_unsigned_short_t ELEKTRA_GET (ELEKTRA_TAG_ADJUSTMENT_CRTC) (Elektra * elektra )
{
	
	return ELEKTRA_GET (UnsignedShort) (elektra, "adjustment/crtc");
}


/**
 * Set the value of key 'adjustment/crtc' (tag #ELEKTRA_TAG_ADJUSTMENT_CRTC).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'adjustment/crtc'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_ADJUSTMENT_CRTC) (Elektra * elektra,
						      kdb_unsigned_short_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (UnsignedShort) (elektra, "adjustment/crtc", value, error);
}




/**
 * Get the value of key 'adjustment/drm/card' (tag #ELEKTRA_TAG_ADJUSTMENT_DRM_CARD).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'adjustment/drm/card'.

 */// 
static inline kdb_unsigned_short_t ELEKTRA_GET (ELEKTRA_TAG_ADJUSTMENT_DRM_CARD) (Elektra * elektra )
{
	
	return ELEKTRA_GET (UnsignedShort) (elektra, "adjustment/drm/card");
}


/**
 * Set the value of key 'adjustment/drm/card' (tag #ELEKTRA_TAG_ADJUSTMENT_DRM_CARD).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'adjustment/drm/card'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_ADJUSTMENT_DRM_CARD) (Elektra * elektra,
						      kdb_unsigned_short_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (UnsignedShort) (elektra, "adjustment/drm/card", value, error);
}




/**
 * Get the value of key 'adjustment/method' (tag #ELEKTRA_TAG_ADJUSTMENT_METHOD).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'adjustment/method'.

 */// 
static inline ElektraEnumAdjustmentMethod ELEKTRA_GET (ELEKTRA_TAG_ADJUSTMENT_METHOD) (Elektra * elektra )
{
	
	return ELEKTRA_GET (EnumAdjustmentMethod) (elektra, "adjustment/method");
}


/**
 * Set the value of key 'adjustment/method' (tag #ELEKTRA_TAG_ADJUSTMENT_METHOD).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'adjustment/method'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_ADJUSTMENT_METHOD) (Elektra * elektra,
						      ElektraEnumAdjustmentMethod value,  ElektraError ** error)
{
	
	ELEKTRA_SET (EnumAdjustmentMethod) (elektra, "adjustment/method", value, error);
}




/**
 * Get the value of key 'adjustment/method/help' (tag #ELEKTRA_TAG_ADJUSTMENT_METHOD_HELP).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'adjustment/method/help'.

 */// 
static inline kdb_boolean_t ELEKTRA_GET (ELEKTRA_TAG_ADJUSTMENT_METHOD_HELP) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Boolean) (elektra, "adjustment/method/help");
}


/**
 * Set the value of key 'adjustment/method/help' (tag #ELEKTRA_TAG_ADJUSTMENT_METHOD_HELP).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'adjustment/method/help'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_ADJUSTMENT_METHOD_HELP) (Elektra * elektra,
						      kdb_boolean_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Boolean) (elektra, "adjustment/method/help", value, error);
}




/**
 * Get the value of key 'adjustment/screen' (tag #ELEKTRA_TAG_ADJUSTMENT_SCREEN).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'adjustment/screen'.

 */// 
static inline kdb_unsigned_short_t ELEKTRA_GET (ELEKTRA_TAG_ADJUSTMENT_SCREEN) (Elektra * elektra )
{
	
	return ELEKTRA_GET (UnsignedShort) (elektra, "adjustment/screen");
}


/**
 * Set the value of key 'adjustment/screen' (tag #ELEKTRA_TAG_ADJUSTMENT_SCREEN).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'adjustment/screen'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_ADJUSTMENT_SCREEN) (Elektra * elektra,
						      kdb_unsigned_short_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (UnsignedShort) (elektra, "adjustment/screen", value, error);
}




/**
 * Get the value of key 'brightness/day' (tag #ELEKTRA_TAG_BRIGHTNESS_DAY).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'brightness/day'.

 */// 
static inline kdb_float_t ELEKTRA_GET (ELEKTRA_TAG_BRIGHTNESS_DAY) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Float) (elektra, "brightness/day");
}


/**
 * Set the value of key 'brightness/day' (tag #ELEKTRA_TAG_BRIGHTNESS_DAY).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'brightness/day'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_BRIGHTNESS_DAY) (Elektra * elektra,
						      kdb_float_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Float) (elektra, "brightness/day", value, error);
}




/**
 * Get the value of key 'brightness/night' (tag #ELEKTRA_TAG_BRIGHTNESS_NIGHT).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'brightness/night'.

 */// 
static inline kdb_float_t ELEKTRA_GET (ELEKTRA_TAG_BRIGHTNESS_NIGHT) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Float) (elektra, "brightness/night");
}


/**
 * Set the value of key 'brightness/night' (tag #ELEKTRA_TAG_BRIGHTNESS_NIGHT).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'brightness/night'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_BRIGHTNESS_NIGHT) (Elektra * elektra,
						      kdb_float_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Float) (elektra, "brightness/night", value, error);
}




/**
 * Get the value of key 'fade/fast' (tag #ELEKTRA_TAG_FADE_FAST).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'fade/fast'.

 */// 
static inline kdb_boolean_t ELEKTRA_GET (ELEKTRA_TAG_FADE_FAST) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Boolean) (elektra, "fade/fast");
}


/**
 * Set the value of key 'fade/fast' (tag #ELEKTRA_TAG_FADE_FAST).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'fade/fast'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_FADE_FAST) (Elektra * elektra,
						      kdb_boolean_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Boolean) (elektra, "fade/fast", value, error);
}




/**
 * Get the value of key 'gamma/day' (tag #ELEKTRA_TAG_GAMMA_DAY).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'gamma/day'.
 *   The returned pointer may become invalid, if the internal state of @p elektra
 *   is modified. All calls to elektraSet* modify this state.
 */// 
static inline const char * ELEKTRA_GET (ELEKTRA_TAG_GAMMA_DAY) (Elektra * elektra )
{
	
	return ELEKTRA_GET (String) (elektra, "gamma/day");
}


/**
 * Set the value of key 'gamma/day' (tag #ELEKTRA_TAG_GAMMA_DAY).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'gamma/day'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_GAMMA_DAY) (Elektra * elektra,
						      const char * value,  ElektraError ** error)
{
	
	ELEKTRA_SET (String) (elektra, "gamma/day", value, error);
}




/**
 * Get the value of key 'gamma/night' (tag #ELEKTRA_TAG_GAMMA_NIGHT).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'gamma/night'.
 *   The returned pointer may become invalid, if the internal state of @p elektra
 *   is modified. All calls to elektraSet* modify this state.
 */// 
static inline const char * ELEKTRA_GET (ELEKTRA_TAG_GAMMA_NIGHT) (Elektra * elektra )
{
	
	return ELEKTRA_GET (String) (elektra, "gamma/night");
}


/**
 * Set the value of key 'gamma/night' (tag #ELEKTRA_TAG_GAMMA_NIGHT).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'gamma/night'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_GAMMA_NIGHT) (Elektra * elektra,
						      const char * value,  ElektraError ** error)
{
	
	ELEKTRA_SET (String) (elektra, "gamma/night", value, error);
}




/**
 * Get the value of key 'gamma/preserve' (tag #ELEKTRA_TAG_GAMMA_PRESERVE).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'gamma/preserve'.

 */// 
static inline kdb_boolean_t ELEKTRA_GET (ELEKTRA_TAG_GAMMA_PRESERVE) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Boolean) (elektra, "gamma/preserve");
}


/**
 * Set the value of key 'gamma/preserve' (tag #ELEKTRA_TAG_GAMMA_PRESERVE).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'gamma/preserve'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_GAMMA_PRESERVE) (Elektra * elektra,
						      kdb_boolean_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Boolean) (elektra, "gamma/preserve", value, error);
}




/**
 * Get the value of key 'help' (tag #ELEKTRA_TAG_HELP).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'help'.

 */// 
static inline kdb_boolean_t ELEKTRA_GET (ELEKTRA_TAG_HELP) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Boolean) (elektra, "help");
}


/**
 * Set the value of key 'help' (tag #ELEKTRA_TAG_HELP).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'help'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_HELP) (Elektra * elektra,
						      kdb_boolean_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Boolean) (elektra, "help", value, error);
}




/**
 * Get the value of key 'mode' (tag #ELEKTRA_TAG_MODE).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'mode'.

 */// 
static inline ElektraEnumMode ELEKTRA_GET (ELEKTRA_TAG_MODE) (Elektra * elektra )
{
	
	return ELEKTRA_GET (EnumMode) (elektra, "mode");
}


/**
 * Set the value of key 'mode' (tag #ELEKTRA_TAG_MODE).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'mode'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_MODE) (Elektra * elektra,
						      ElektraEnumMode value,  ElektraError ** error)
{
	
	ELEKTRA_SET (EnumMode) (elektra, "mode", value, error);
}




/**
 * Get the value of key 'provider' (tag #ELEKTRA_TAG_PROVIDER).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider'.

 */// 
static inline ElektraEnumProvider ELEKTRA_GET (ELEKTRA_TAG_PROVIDER) (Elektra * elektra )
{
	
	return ELEKTRA_GET (EnumProvider) (elektra, "provider");
}


/**
 * Set the value of key 'provider' (tag #ELEKTRA_TAG_PROVIDER).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER) (Elektra * elektra,
						      ElektraEnumProvider value,  ElektraError ** error)
{
	
	ELEKTRA_SET (EnumProvider) (elektra, "provider", value, error);
}




/**
 * Get the value of key 'provider/location' (tag #ELEKTRA_TAG_PROVIDER_LOCATION).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider/location'.

 */// 
static inline ElektraEnumProviderLocation ELEKTRA_GET (ELEKTRA_TAG_PROVIDER_LOCATION) (Elektra * elektra )
{
	
	return ELEKTRA_GET (EnumProviderLocation) (elektra, "provider/location");
}


/**
 * Set the value of key 'provider/location' (tag #ELEKTRA_TAG_PROVIDER_LOCATION).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider/location'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER_LOCATION) (Elektra * elektra,
						      ElektraEnumProviderLocation value,  ElektraError ** error)
{
	
	ELEKTRA_SET (EnumProviderLocation) (elektra, "provider/location", value, error);
}




/**
 * Get the value of key 'provider/location/elevation/high' (tag #ELEKTRA_TAG_PROVIDER_LOCATION_ELEVATION_HIGH).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider/location/elevation/high'.

 */// 
static inline kdb_float_t ELEKTRA_GET (ELEKTRA_TAG_PROVIDER_LOCATION_ELEVATION_HIGH) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Float) (elektra, "provider/location/elevation/high");
}


/**
 * Set the value of key 'provider/location/elevation/high' (tag #ELEKTRA_TAG_PROVIDER_LOCATION_ELEVATION_HIGH).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider/location/elevation/high'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER_LOCATION_ELEVATION_HIGH) (Elektra * elektra,
						      kdb_float_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Float) (elektra, "provider/location/elevation/high", value, error);
}




/**
 * Get the value of key 'provider/location/elevation/low' (tag #ELEKTRA_TAG_PROVIDER_LOCATION_ELEVATION_LOW).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider/location/elevation/low'.

 */// 
static inline kdb_float_t ELEKTRA_GET (ELEKTRA_TAG_PROVIDER_LOCATION_ELEVATION_LOW) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Float) (elektra, "provider/location/elevation/low");
}


/**
 * Set the value of key 'provider/location/elevation/low' (tag #ELEKTRA_TAG_PROVIDER_LOCATION_ELEVATION_LOW).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider/location/elevation/low'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER_LOCATION_ELEVATION_LOW) (Elektra * elektra,
						      kdb_float_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Float) (elektra, "provider/location/elevation/low", value, error);
}




/**
 * Get the value of key 'provider/location/help' (tag #ELEKTRA_TAG_PROVIDER_LOCATION_HELP).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider/location/help'.

 */// 
static inline kdb_boolean_t ELEKTRA_GET (ELEKTRA_TAG_PROVIDER_LOCATION_HELP) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Boolean) (elektra, "provider/location/help");
}


/**
 * Set the value of key 'provider/location/help' (tag #ELEKTRA_TAG_PROVIDER_LOCATION_HELP).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider/location/help'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER_LOCATION_HELP) (Elektra * elektra,
						      kdb_boolean_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Boolean) (elektra, "provider/location/help", value, error);
}




/**
 * Get the value of key 'provider/location/manual/lat' (tag #ELEKTRA_TAG_PROVIDER_LOCATION_MANUAL_LAT).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider/location/manual/lat'.

 */// 
static inline kdb_float_t ELEKTRA_GET (ELEKTRA_TAG_PROVIDER_LOCATION_MANUAL_LAT) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Float) (elektra, "provider/location/manual/lat");
}


/**
 * Set the value of key 'provider/location/manual/lat' (tag #ELEKTRA_TAG_PROVIDER_LOCATION_MANUAL_LAT).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider/location/manual/lat'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER_LOCATION_MANUAL_LAT) (Elektra * elektra,
						      kdb_float_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Float) (elektra, "provider/location/manual/lat", value, error);
}




/**
 * Get the value of key 'provider/location/manual/lon' (tag #ELEKTRA_TAG_PROVIDER_LOCATION_MANUAL_LON).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider/location/manual/lon'.

 */// 
static inline kdb_float_t ELEKTRA_GET (ELEKTRA_TAG_PROVIDER_LOCATION_MANUAL_LON) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Float) (elektra, "provider/location/manual/lon");
}


/**
 * Set the value of key 'provider/location/manual/lon' (tag #ELEKTRA_TAG_PROVIDER_LOCATION_MANUAL_LON).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider/location/manual/lon'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER_LOCATION_MANUAL_LON) (Elektra * elektra,
						      kdb_float_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Float) (elektra, "provider/location/manual/lon", value, error);
}




/**
 * Get the value of key 'provider/time/dawn/end' (tag #ELEKTRA_TAG_PROVIDER_TIME_DAWN_END).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider/time/dawn/end'.
 *   The returned pointer may become invalid, if the internal state of @p elektra
 *   is modified. All calls to elektraSet* modify this state.
 */// 
static inline const char * ELEKTRA_GET (ELEKTRA_TAG_PROVIDER_TIME_DAWN_END) (Elektra * elektra )
{
	
	return ELEKTRA_GET (String) (elektra, "provider/time/dawn/end");
}


/**
 * Set the value of key 'provider/time/dawn/end' (tag #ELEKTRA_TAG_PROVIDER_TIME_DAWN_END).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider/time/dawn/end'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER_TIME_DAWN_END) (Elektra * elektra,
						      const char * value,  ElektraError ** error)
{
	
	ELEKTRA_SET (String) (elektra, "provider/time/dawn/end", value, error);
}




/**
 * Get the value of key 'provider/time/dawn/start' (tag #ELEKTRA_TAG_PROVIDER_TIME_DAWN_START).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider/time/dawn/start'.
 *   The returned pointer may become invalid, if the internal state of @p elektra
 *   is modified. All calls to elektraSet* modify this state.
 */// 
static inline const char * ELEKTRA_GET (ELEKTRA_TAG_PROVIDER_TIME_DAWN_START) (Elektra * elektra )
{
	
	return ELEKTRA_GET (String) (elektra, "provider/time/dawn/start");
}


/**
 * Set the value of key 'provider/time/dawn/start' (tag #ELEKTRA_TAG_PROVIDER_TIME_DAWN_START).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider/time/dawn/start'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER_TIME_DAWN_START) (Elektra * elektra,
						      const char * value,  ElektraError ** error)
{
	
	ELEKTRA_SET (String) (elektra, "provider/time/dawn/start", value, error);
}




/**
 * Get the value of key 'provider/time/dusk/end' (tag #ELEKTRA_TAG_PROVIDER_TIME_DUSK_END).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider/time/dusk/end'.
 *   The returned pointer may become invalid, if the internal state of @p elektra
 *   is modified. All calls to elektraSet* modify this state.
 */// 
static inline const char * ELEKTRA_GET (ELEKTRA_TAG_PROVIDER_TIME_DUSK_END) (Elektra * elektra )
{
	
	return ELEKTRA_GET (String) (elektra, "provider/time/dusk/end");
}


/**
 * Set the value of key 'provider/time/dusk/end' (tag #ELEKTRA_TAG_PROVIDER_TIME_DUSK_END).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider/time/dusk/end'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER_TIME_DUSK_END) (Elektra * elektra,
						      const char * value,  ElektraError ** error)
{
	
	ELEKTRA_SET (String) (elektra, "provider/time/dusk/end", value, error);
}




/**
 * Get the value of key 'provider/time/dusk/start' (tag #ELEKTRA_TAG_PROVIDER_TIME_DUSK_START).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'provider/time/dusk/start'.
 *   The returned pointer may become invalid, if the internal state of @p elektra
 *   is modified. All calls to elektraSet* modify this state.
 */// 
static inline const char * ELEKTRA_GET (ELEKTRA_TAG_PROVIDER_TIME_DUSK_START) (Elektra * elektra )
{
	
	return ELEKTRA_GET (String) (elektra, "provider/time/dusk/start");
}


/**
 * Set the value of key 'provider/time/dusk/start' (tag #ELEKTRA_TAG_PROVIDER_TIME_DUSK_START).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'provider/time/dusk/start'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_PROVIDER_TIME_DUSK_START) (Elektra * elektra,
						      const char * value,  ElektraError ** error)
{
	
	ELEKTRA_SET (String) (elektra, "provider/time/dusk/start", value, error);
}




/**
 * Get the value of key 'temp/day' (tag #ELEKTRA_TAG_TEMP_DAY).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'temp/day'.

 */// 
static inline kdb_unsigned_short_t ELEKTRA_GET (ELEKTRA_TAG_TEMP_DAY) (Elektra * elektra )
{
	
	return ELEKTRA_GET (UnsignedShort) (elektra, "temp/day");
}


/**
 * Set the value of key 'temp/day' (tag #ELEKTRA_TAG_TEMP_DAY).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'temp/day'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_TEMP_DAY) (Elektra * elektra,
						      kdb_unsigned_short_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (UnsignedShort) (elektra, "temp/day", value, error);
}




/**
 * Get the value of key 'temp/night' (tag #ELEKTRA_TAG_TEMP_NIGHT).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'temp/night'.

 */// 
static inline kdb_unsigned_short_t ELEKTRA_GET (ELEKTRA_TAG_TEMP_NIGHT) (Elektra * elektra )
{
	
	return ELEKTRA_GET (UnsignedShort) (elektra, "temp/night");
}


/**
 * Set the value of key 'temp/night' (tag #ELEKTRA_TAG_TEMP_NIGHT).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'temp/night'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_TEMP_NIGHT) (Elektra * elektra,
						      kdb_unsigned_short_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (UnsignedShort) (elektra, "temp/night", value, error);
}




/**
 * Get the value of key 'temp/oneshotmanual' (tag #ELEKTRA_TAG_TEMP_ONESHOTMANUAL).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'temp/oneshotmanual'.

 */// 
static inline kdb_unsigned_short_t ELEKTRA_GET (ELEKTRA_TAG_TEMP_ONESHOTMANUAL) (Elektra * elektra )
{
	
	return ELEKTRA_GET (UnsignedShort) (elektra, "temp/oneshotmanual");
}


/**
 * Set the value of key 'temp/oneshotmanual' (tag #ELEKTRA_TAG_TEMP_ONESHOTMANUAL).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'temp/oneshotmanual'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_TEMP_ONESHOTMANUAL) (Elektra * elektra,
						      kdb_unsigned_short_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (UnsignedShort) (elektra, "temp/oneshotmanual", value, error);
}




/**
 * Get the value of key 'verbose' (tag #ELEKTRA_TAG_VERBOSE).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'verbose'.

 */// 
static inline kdb_boolean_t ELEKTRA_GET (ELEKTRA_TAG_VERBOSE) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Boolean) (elektra, "verbose");
}


/**
 * Set the value of key 'verbose' (tag #ELEKTRA_TAG_VERBOSE).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'verbose'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_VERBOSE) (Elektra * elektra,
						      kdb_boolean_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Boolean) (elektra, "verbose", value, error);
}




/**
 * Get the value of key 'version' (tag #ELEKTRA_TAG_VERSION).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().

 *
 * @return the value of 'version'.

 */// 
static inline kdb_boolean_t ELEKTRA_GET (ELEKTRA_TAG_VERSION) (Elektra * elektra )
{
	
	return ELEKTRA_GET (Boolean) (elektra, "version");
}


/**
 * Set the value of key 'version' (tag #ELEKTRA_TAG_VERSION).
 *
 * @param elektra Instance of Elektra. Create with loadConfiguration().
 * @param value   The value of 'version'.

 * @param error   Pass a reference to an ElektraError pointer.
 *                Will only be set in case of an error.
 */// 
static inline void ELEKTRA_SET (ELEKTRA_TAG_VERSION) (Elektra * elektra,
						      kdb_boolean_t value,  ElektraError ** error)
{
	
	ELEKTRA_SET (Boolean) (elektra, "version", value, error);
}


#undef elektra_len19
#undef elektra_len18
#undef elektra_len17
#undef elektra_len16
#undef elektra_len15
#undef elektra_len14
#undef elektra_len13
#undef elektra_len12
#undef elektra_len11
#undef elektra_len10
#undef elektra_len09
#undef elektra_len08
#undef elektra_len07
#undef elektra_len06
#undef elektra_len05
#undef elektra_len04
#undef elektra_len03
#undef elektra_len02
#undef elektra_len01
#undef elektra_len00
#undef elektra_len


int loadConfiguration (Elektra ** elektra,
				 int argc, const char * const * argv, const char * const * envp,
				 
				 ElektraError ** error);
void printHelpMessage (Elektra * elektra, const char * usage, const char * prefix);
void exitForSpecload (int argc, const char * const * argv);




/**
 * @param elektra The elektra instance initialized with loadConfiguration().
 * @param tag     The tag to look up.
 *
 * @return The value stored at the given key.
 *   The lifetime of returned pointers is documented in the ELEKTRA_GET(*) functions above.
 */// 
#define elektraGet(elektra, tag) ELEKTRA_GET (tag) (elektra)


/**
 * @param elektra The elektra instance initialized with loadConfiguration().
 * @param tag     The tag to look up.
 * @param ...     Variable arguments depending on the given tag.
 *
 * @return The value stored at the given key.
 *   The lifetime of returned pointers is documented in the ELEKTRA_GET(*) functions above.
 */// 
#define elektraGetV(elektra, tag, ...) ELEKTRA_GET (tag) (elektra, __VA_ARGS__)


/**
 * @param elektra The elektra instance initialized with loadConfiguration().
 * @param result  Points to the struct into which results will be stored.
 *   The lifetime of pointers in this struct is documented in the ELEKTRA_GET(*) functions above.
 * @param tag     The tag to look up.
 */// 
#define elektraFillStruct(elektra, result, tag) ELEKTRA_GET (tag) (elektra, result)


/**
 * @param elektra The elektra instance initialized with loadConfiguration().
 * @param result  Points to the struct into which results will be stored.
 *   The lifetime of pointers in this struct is documented in the ELEKTRA_GET(*) functions above.
 * @param tag     The tag to look up.
 * @param ...     Variable arguments depending on the given tag.
 */// 
#define elektraFillStructV(elektra, result, tag, ...) ELEKTRA_GET (tag) (elektra, result, __VA_ARGS__)


/**
 * @param elektra The elektra instance initialized with the loadConfiguration().
 * @param tag     The tag to write to.
 * @param value   The new value.
 * @param error   Pass a reference to an ElektraError pointer.
 */// 
#define elektraSet(elektra, tag, value, error) ELEKTRA_SET (tag) (elektra, value, error)


/**
 * @param elektra The elektra instance initialized with the loadConfiguration().
 * @param tag     The tag to write to.
 * @param value   The new value.
 * @param error   Pass a reference to an ElektraError pointer.
 * @param ...     Variable arguments depending on the given tag.
 */// 
#define elektraSetV(elektra, tag, value, error, ...) ELEKTRA_SET (tag) (elektra, value, __VA_ARGS__, error)


/**
 * @param elektra The elektra instance initialized with loadConfiguration().
 * @param tag     The array tag to look up.
 *
 * @return The size of the array below the given key.
 */// 
#define elektraSize(elektra, tag) ELEKTRA_SIZE (tag) (elektra)


/**
 * @param elektra The elektra instance initialized with loadConfiguration().
 * @param tag     The array tag to look up.
 * @param ...     Variable arguments depending on the given tag.
 *
 * @return The size of the array below the given key.
 */// 
#define elektraSizeV(elektra, tag, ...) ELEKTRA_SIZE (tag) (elektra, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // REDSHIFT_CONF_H
