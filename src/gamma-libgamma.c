/* gamma-libgamma.c -- libgamma gamma adjustment
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
   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#include "gamma-libgamma.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>


#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif


#if LIBGAMMA_ERROR_MIN < -46
# warning New error codes have been added to libgamma.
#endif
#if LIBGAMMA_METHOD_COUNT > 6
# warning New adjust methods has been added to libgamma
#endif
#if LIBGAMMA_CONNECTOR_TYPE_COUNT > 20
# warning New connector types have been added to libgamma.
#endif
#if LIBGAMMA_SUBPIXEL_ORDER_COUNT > 6
# warning New subpixel orders have been added to libgamma.
#endif
#if LIBGAMMA_CRTC_INFO_COUNT > 13
# warning New CRTC information fields have been added to libgamma.
#endif



static void
gamma_libgamma_perror(const char* name, int error_code)
{
#define p(error, text)  case error:  fprintf(stderr, "%s: %s\n", name, text);  return

	switch (error_code) {
	p(LIBGAMMA_NO_SUCH_ADJUSTMENT_METHOD,
	  _("The selected adjustment method does not exist or has been excluded at compile-time."));

	p(LIBGAMMA_NO_SUCH_SITE,
	  _("The selected display does not exist."));

	p(LIBGAMMA_NO_SUCH_PARTITION,
	  _("The selected screen/graphics card does not exist."));

	p(LIBGAMMA_NO_SUCH_CRTC,
	  _("The selected monitor does not exist."));

	p(LIBGAMMA_IMPOSSIBLE_AMOUNT,
	  _("Counter overflowed when counting the number of available items."));

	p(LIBGAMMA_CONNECTOR_DISABLED,
	  _("The selected connector is disabled, it does not have a CRTC."));

	p(LIBGAMMA_OPEN_CRTC_FAILED,
	  _("The selected CRTC could not be opened, reason unknown."));

	p(LIBGAMMA_CRTC_INFO_NOT_SUPPORTED,
	  _("The CRTC information field is not supported by the adjustment method."));

	p(LIBGAMMA_GAMMA_RAMP_READ_FAILED,
	  _("Failed to read the current gamma ramps for the selected monitor, reason unknown."));

	p(LIBGAMMA_GAMMA_RAMP_WRITE_FAILED,
	  _("Failed to write the current gamma ramps for the selected monitor, reason unknown."));

	p(LIBGAMMA_GAMMA_RAMP_SIZE_CHANGED,
	  _("The specified ramp sizes does not match the ramps sizes returned by the adjustment methods in response to the query/command."));

	p(LIBGAMMA_MIXED_GAMMA_RAMP_SIZE,
	  _("The specified ramp sizes are not identical which is required by the adjustment method."));

	p(LIBGAMMA_WRONG_GAMMA_RAMP_SIZE,
	  _("The specified ramp sizes are not supported by the adjustment method."));

	p(LIBGAMMA_SINGLETON_GAMMA_RAMP,
	  _("The adjustment method reported that the gamma ramps smaller than 2 stops, this is not possible."));

	p(LIBGAMMA_LIST_CRTCS_FAILED,
	  _("The adjustment method failed to list available CRTCs, reason unknown."));

	p(LIBGAMMA_ACQUIRING_MODE_RESOURCES_FAILED,
	  _("Failed to acquire mode resources from the adjustment method."));

	p(LIBGAMMA_NEGATIVE_PARTITION_COUNT,
	  _("The adjustment method reported that a negative number of screens/graphics cards exists in the display."));

	p(LIBGAMMA_NEGATIVE_CRTC_COUNT,
	  _("The adjustment method reported that a negative number of CRTCs exists in the screen/graphics card."));

	p(LIBGAMMA_DEVICE_RESTRICTED,
	  _("Device cannot be access becauses of insufficient permissions."));

	p(LIBGAMMA_DEVICE_ACCESS_FAILED,
	  _("Device cannot be access, reason unknown."));

	p(LIBGAMMA_GRAPHICS_CARD_REMOVED,
	  _("The graphics card appears to have been removed."));

	p(LIBGAMMA_STATE_UNKNOWN,
	  _("The state of the requested information is unknown."));

	p(LIBGAMMA_CONNECTOR_UNKNOWN,
	  _("Failed to determine which connector the CRTC belongs to."));

	p(LIBGAMMA_CONNECTOR_TYPE_NOT_RECOGNISED,
	  _("The detected connector type is not listed in libgamma and has to be updated."));

	p(LIBGAMMA_SUBPIXEL_ORDER_NOT_RECOGNISED,
	  _("The detected subpixel order is not listed in libgamma and has to be updated."));

	p(LIBGAMMA_EDID_LENGTH_UNSUPPORTED,
	  _("The length of the EDID does not match that of any supported EDID structure revision."));

	p(LIBGAMMA_EDID_WRONG_MAGIC_NUMBER,
	  _("The magic number in the EDID does not match that of any supported EDID structure revision."));

	p(LIBGAMMA_EDID_REVISION_UNSUPPORTED,
	  _("The EDID structure revision used by the monitor is not supported."));

	p(LIBGAMMA_GAMMA_NOT_SPECIFIED,
	  _("The gamma characteristics field in the EDID is left unspecified."));

	p(LIBGAMMA_EDID_CHECKSUM_ERROR,
	  _("The checksum in the EDID is incorrect, all request information has been provided by you cannot count on it."));

	p(LIBGAMMA_GAMMA_RAMPS_SIZE_QUERY_FAILED,
	  _("Failed to query the gamma ramps size from the adjustment method, reason unknown."));

	p(LIBGAMMA_OPEN_PARTITION_FAILED,
	  _("The selected screen/graphics card could not be opened, reason unknown."));

	p(LIBGAMMA_OPEN_SITE_FAILED,
	  _("The selected display could not be opened, reason unknown."));

	p(LIBGAMMA_PROTOCOL_VERSION_QUERY_FAILED,
	  _("Failed to query the adjustment method for its protocol version, reason unknown."));

	p(LIBGAMMA_PROTOCOL_VERSION_NOT_SUPPORTED,
	  _("The adjustment method's version of its protocol is not supported."));

	p(LIBGAMMA_LIST_PARTITIONS_FAILED,
	  _("The adjustment method failed to list available screens/graphics cards, reason unknown."));

	p(LIBGAMMA_NULL_PARTITION,
	  _("Screen/graphics card exists by index, but the screen/graphics card at that index does not exist."));

	p(LIBGAMMA_NOT_CONNECTED,
	  _("There is not monitor connected to the connector of the selected CRTC."));

	p(LIBGAMMA_REPLY_VALUE_EXTRACTION_FAILED,
	  _("Data extraction from a reply from the adjustment method failed, reason unknown."));

	p(LIBGAMMA_EDID_NOT_FOUND,
	  _("No EDID property was found not on the output."));

	p(LIBGAMMA_LIST_PROPERTIES_FAILED,
	  _("Failed to list properties on the output, reason unknown."));

	p(LIBGAMMA_PROPERTY_VALUE_QUERY_FAILED,
	  _("Failed to query a property's value from the output, reason unknown."));

	p(LIBGAMMA_OUTPUT_INFORMATION_QUERY_FAILED,
	  _("A request for information on an output failed, reason unknown."));

	case LIBGAMMA_GAMMA_NOT_SPECIFIED_AND_EDID_CHECKSUM_ERROR:
		gamma_libgamma_perror(name, LIBGAMMA_GAMMA_NOT_SPECIFIED);
		gamma_libgamma_perror(name, LIBGAMMA_EDID_CHECKSUM_ERROR);
		return;

	case LIBGAMMA_DEVICE_REQUIRE_GROUP:
		if (libgamma_group_name == NULL) {
			fprintf(stderr,
				_("%s: Device cannot be access, membership of group %ji is required.\n"),
				name, (intmax_t)libgamma_group_gid);
		} else {
			fprintf(stderr,
				_("%s: Device cannot be access, membership of group %ji (%s) is required.\n"),
				name, (intmax_t)libgamma_group_gid, libgamma_group_name);
		}
		return;
	
	case LIBGAMMA_ERRNO_SET:
	default:
		libgamma_perror(name, error_code);
		return;
	}

#undef p
}


int gamma_libgamma_get_method(const char* subsystem)
{
#define __test(NAME, METHOD)			\
	if (!strcmp(subsystem, NAME))		\
		return LIBGAMMA_METHOD_##METHOD

	__test("randr",   X_RANDR);
	__test("vidmode", X_VIDMODE);
	__test("drm",     LINUX_DRM);
	__test("wingdi",  W32_GDI);
	__test("quartz",  QUARTZ_CORE_GRAPHICS);
	__test("dummy",   DUMMY);

	fprintf(stderr, _("An undefined subsystem of libgamma has been requested.\n"));
	abort();
	return -1;
#undef __test
}


const char *gamma_libgamma_get_method_name(int method)
{
#define __test(METHOD, NAME)		\
	case LIBGAMMA_METHOD_##METHOD:	\
		return NAME;
	switch (method) {
	__test(X_RANDR,              _("X.org's RandR extension"));
	__test(X_VIDMODE,            _("X.org's VidMode extension"));
	__test(LINUX_DRM,            _("Direct Rendering Manager"));
	__test(W32_GDI,              _("Windows GDI"));
	__test(QUARTZ_CORE_GRAPHICS, _("Quartz"));
	__test(DUMMY,                _("the dummy adjustment method"));
	default:
		fprintf(stderr, _("An undefined subsystem of libgamma has been requested.\n"));
		abort();
		return NULL;
	}
#undef __test
}


int gamma_libgamma_auto(const char* subsystem)
{
	int method = gamma_libgamma_get_method(subsystem);
	int methods_[LIBGAMMA_METHOD_COUNT];
	int* methods = NULL;
	size_t i, n = libgamma_list_methods(methods_, LIBGAMMA_METHOD_COUNT, 0);
	if (n > LIBGAMMA_METHOD_COUNT) {
		methods = malloc(n * sizeof(int));
		if (methods == NULL) {
			perror("malloc");
			return -1;
		}
		libgamma_list_methods(methods, n, 0);
	} else {
		methods = methods_;
	}
	for (i = 0; i < n; i++)
		if (methods[i] == method) {
			if (methods != methods_)
				free(methods);
			return 1;
		}
	if (methods != methods_)
		free(methods);
	return 0;
}


int gamma_libgamma_is_available(const char* subsystem)
{
	int method = gamma_libgamma_get_method(subsystem);
	return libgamma_is_method_available(method);
}


int gamma_libgamma_get_priority(const char *subsystem)
{
	int method = gamma_libgamma_get_method(subsystem);
	int methods_[LIBGAMMA_METHOD_COUNT];
	int* methods = NULL;
	size_t i, n = libgamma_list_methods(methods_, LIBGAMMA_METHOD_COUNT, 0);
	if (n > LIBGAMMA_METHOD_COUNT) {
		methods = malloc(n * sizeof(int));
		if (methods == NULL) {
			perror("malloc");
			return -1;
		}
		libgamma_list_methods(methods, n, 0);
	} else {
		methods = methods_;
	}
	for (i = 0; i < n; i++)
		if (methods[i] == method) {
			if (methods != methods_)
				free(methods);
			return (int)i;
		}
	abort();
	return -1;
}


static int
gamma_libgamma_get_ramps(libgamma_crtc_state_t *crtc_state, gamma_ramps_t ramps)
{
	libgamma_gamma_ramps16_t ramps_;
	int r;

	ramps_.red_size   = ramps.red_size;
	ramps_.green_size = ramps.green_size;
	ramps_.blue_size  = ramps.blue_size;
	ramps_.red   = ramps.red;
	ramps_.green = ramps.green;
	ramps_.blue  = ramps.blue;

	r = libgamma_crtc_get_gamma_ramps16(crtc_state, &ramps_);
	if (r) {
		gamma_libgamma_perror("libgamma_crtc_get_gamma_ramps16", r);
		return -1;
	}

	return 0;
}


static int
gamma_libgamma_set_ramps(gamma_server_state_t *state, gamma_crtc_state_t *crtc, gamma_ramps_t ramps)
{
	(void) state;

	libgamma_crtc_state_t *crtc_state = crtc->data;
	libgamma_gamma_ramps16_t ramps_;
	int r;

	ramps_.red_size   = ramps.red_size;
	ramps_.green_size = ramps.green_size;
	ramps_.blue_size  = ramps.blue_size;
	ramps_.red   = ramps.red;
	ramps_.green = ramps.green;
	ramps_.blue  = ramps.blue;

	r = libgamma_crtc_set_gamma_ramps16(crtc_state, ramps_);
	if (r) {
		gamma_libgamma_perror("libgamma_crtc_set_gamma_ramps16", r);
		return -1;
	}

	return 0;
}


static void
gamma_libgamma_free_state(void *data)
{
	free(data);
}

static void
gamma_libgamma_free_site(void *data)
{
	libgamma_site_free(data);
}

static void
gamma_libgamma_free_partition(void *data)
{
	libgamma_partition_free(data);
}

static void
gamma_libgamma_free_crtc(void *data)
{
	libgamma_crtc_free(data);
}


static int
gamma_libgamma_open_site(gamma_server_state_t *state, char *site, gamma_site_state_t *site_out)
{
	gamma_libgamma_state_data_t *state_data = state->data;
	char *site_id = NULL;
	libgamma_site_state_t *site_state = NULL;
	int r;

	site_id = site == NULL ? NULL : strdup(site);
	if (site && !site_id) {
		perror("strdup");
		goto fail;
	}

	site_state = malloc(sizeof(libgamma_site_state_t));
	if (site_state == NULL) {
		perror("malloc");
		goto fail;
	}

        r = libgamma_site_initialise(site_state, state_data->method, site_id);
	if (r) {
		gamma_libgamma_perror("libgamma_site_initialise", r);
		goto fail;
	}

	site_out->data = site_state;
	site_out->partitions_available = site_state->partitions_available;

	return 0;

fail:
	free(site_id);
	free(site_state);
	return -1;
}


static int
gamma_libgamma_open_partition(gamma_server_state_t *state, gamma_site_state_t *site,
			      size_t partition, gamma_partition_state_t *partition_out)
{
	(void) state;

	libgamma_partition_state_t *partition_state = NULL;
	int r;

	partition_state = malloc(sizeof(libgamma_partition_state_t));
	if (partition_state == NULL) {
		perror("malloc");
		goto fail;
	}

	r = libgamma_partition_initialise(partition_state, site->data, partition);
	if (r) {
		gamma_libgamma_perror("libgamma_partition_initialise", r);
		goto fail;
	}

	partition_out->data = partition_state;
	partition_out->crtcs_available = partition_state->crtcs_available;

	return 0;

fail:
	free(partition_state);
	return -1;
}



static int
gamma_libgamma_open_crtc(gamma_server_state_t *state, gamma_site_state_t *site,
			 gamma_partition_state_t *partition, size_t crtc, gamma_crtc_state_t *crtc_out)
{
	(void) state;
	(void) site;

	libgamma_crtc_state_t *crtc_state = NULL;
	libgamma_crtc_information_t info;
	size_t grand_size;
	int r;

	crtc_out->saved_ramps.red = NULL;

	crtc_state = malloc(sizeof(libgamma_crtc_state_t));
	if (crtc_state == NULL) {
		perror("malloc");
		return -1;
	}
 
	r = libgamma_crtc_initialise(crtc_state, partition->data, crtc);
	if (r) {
		gamma_libgamma_perror("libgamma_crtc_initialise", r);
		free(crtc_state);
		return -1;
	}

	libgamma_get_crtc_information(&info, crtc_state, LIBGAMMA_CRTC_INFO_GAMMA_SIZE);
	if (info.gamma_size_error) {
		gamma_libgamma_perror("libgamma_get_crtc_information", info.gamma_size_error);
		goto fail;
	}

	grand_size  = crtc_out->saved_ramps.red_size   = info.red_gamma_size;
	grand_size += crtc_out->saved_ramps.green_size = info.green_gamma_size;
	grand_size += crtc_out->saved_ramps.blue_size  = info.blue_gamma_size;

	crtc_out->saved_ramps.red   = malloc(grand_size * sizeof(uint16_t));
	crtc_out->saved_ramps.green = crtc_out->saved_ramps.red   + crtc_out->saved_ramps.red_size;
	crtc_out->saved_ramps.blue  = crtc_out->saved_ramps.green + crtc_out->saved_ramps.green_size;
	if (crtc_out->saved_ramps.red == NULL) {
		perror("malloc");
		goto fail;
	}

	r = gamma_libgamma_get_ramps(crtc_state, crtc_out->saved_ramps);
	if (r < 0) goto fail;

	crtc_out->data = crtc_state;

	return 0;

fail:
	free(crtc_out->saved_ramps.red);
	crtc_out->saved_ramps.red   = NULL;
	crtc_out->saved_ramps.green = NULL;
	crtc_out->saved_ramps.blue  = NULL;
	libgamma_crtc_destroy(crtc_state);
	return -1;
}


static void
gamma_libgamma_invalid_partition(gamma_server_state_t *state, const gamma_site_state_t *site, size_t partition)
{
	gamma_libgamma_state_data_t *state_data = state->data;
	libgamma_method_capabilities_t caps = state_data->caps;

	if (caps.partitions_are_graphics_cards) {
		fprintf(stderr, _("Card %ld does not exist. "),
			partition);
		if (site->partitions_available > 1) {
			fprintf(stderr, _("Valid cards are [0-%ld].\n"),
				site->partitions_available - 1);
		} else {
			fprintf(stderr, _("Only card 0 exists.\n"));
		}

	} else if (caps.multiple_crtcs) {
		fprintf(stderr, _("Screen %ld does not exist. "),
			partition);
		if (site->partitions_available > 1) {
			fprintf(stderr, _("Valid screens are [0-%ld].\n"),
				site->partitions_available - 1);
		} else {
			fprintf(stderr, _("Only screen 0 exists, did you mean CRTC %ld?\n"),
				partition);
		}

	} else {
		fprintf(stderr, _("Screen %ld does not exist. "),
			partition);
		if (site->partitions_available > 1) {
			fprintf(stderr, _("Valid screens are [0-%ld].\n"),
				site->partitions_available - 1);
		} else {
			fprintf(stderr, _("Only screen 0 exists, did you mean CRTC %ld?\n"),
				partition);
			if (state_data->method == LIBGAMMA_METHOD_X_VIDMODE)
				fprintf(stderr, _("If so, you need to use `randr' instead of `vidmode'.\n"));
		}
	}
}


static int
gamma_libgamma_set_option(gamma_server_state_t *state, const char *key, char *value, ssize_t section)
{
	gamma_libgamma_state_data_t *state_data = state->data;
	libgamma_method_capabilities_t caps = state_data->caps;

	if (caps.partitions_are_graphics_cards && strcasecmp(key, "card") == 0) {
		return gamma_select_partitions(state, value, ',', section, _("card"));

	} else if (caps.multiple_partitions && strcasecmp(key, "screen") == 0) {
		return gamma_select_partitions(state, value, ',', section, _("Screen"));

	} else if (caps.multiple_crtcs && caps.multiple_sites && strcasecmp(key, "crtc") == 0) {
		return gamma_select_crtcs(state, value, ',', section, _("CRTC"));

	} else if (caps.multiple_crtcs && strcasecmp(key, "display") == 0) {
		return gamma_select_crtcs(state, value, ',', section, _("Display"));

	} else if (caps.multiple_sites && strcasecmp(key, "display") == 0) {
		return gamma_select_sites(state, value, ',', section);
	}

	return 1;
}


int
gamma_libgamma_init(gamma_server_state_t *state, const char *subsystem)
{
	gamma_libgamma_state_data_t *state_data = NULL;
	char *default_site;
	int r;

	r = gamma_init(state);
	if (r != 0) return r;

	state->data = state_data = malloc(sizeof(gamma_libgamma_state_data_t));
	if (state_data == NULL) {
		perror("malloc");
		return -1;
	}

	state_data->method = gamma_libgamma_get_method(subsystem);
	libgamma_method_capabilities(&(state_data->caps), state_data->method);

	state->free_state_data         = gamma_libgamma_free_state;
	state->free_site_data          = gamma_libgamma_free_site;
	state->free_partition_data     = gamma_libgamma_free_partition;
	state->free_crtc_data          = gamma_libgamma_free_crtc;
	state->open_site               = gamma_libgamma_open_site;
	state->open_partition          = gamma_libgamma_open_partition;
	state->open_crtc               = gamma_libgamma_open_crtc;
	state->invalid_partition       = gamma_libgamma_invalid_partition;
	state->set_ramps               = gamma_libgamma_set_ramps;
	state->set_option              = gamma_libgamma_set_option;

	if (state_data->caps.multiple_partitions)
		state->invalid_partition = NULL;

	state->selections->sites = malloc(1 * sizeof(char *));
	if (state->selections->sites == NULL) {
		perror("malloc");
		return -1;
	}
	default_site = libgamma_method_default_site(state_data->method);
	if (default_site != NULL) {
		state->selections->sites[0] = strdup(default_site);
		if (state->selections->sites[0] == NULL) {
			perror("strdup");
			return -1;
		}
	} else {
		state->selections->sites[0] = NULL;
	}
	state->selections->sites_count = 1;

	return 0;
}


int
gamma_libgamma_start(gamma_server_state_t *state)
{
	return gamma_resolve_selections(state);
}


void
gamma_libgamma_print_help(FILE *f, const char *subsystem)
{
	int method = gamma_libgamma_get_method(subsystem);
	libgamma_method_capabilities_t caps;

	libgamma_method_capabilities(&caps, method);

	fprintf(f, _("Adjust gamma ramps with %s.\n"), gamma_libgamma_get_method_name(method));
	fputs("\n", f);

	/* TRANSLATORS: libgamma help output
	   left column must not be translated */

	if (caps.multiple_crtcs && caps.multiple_sites)
		fputs(_(" crtc=N\tList of comma separated CRTCs to apply adjustments to\n"), f);
	else if (caps.multiple_crtcs)
		fputs(_(" display=N\tList of comma separated displays to apply adjustments to\n"), f);

	if (caps.partitions_are_graphics_cards)
		fputs(_(" card=N\tList of comma separated graphics cards to apply adjustments to\n"), f);
	else if (caps.multiple_partitions)
		fputs(_(" screen=N\tList of comma separated screens to apply adjustments to\n"), f);

	if (caps.multiple_sites)
		fputs(_(" display=NAME\tList of comma separated displays to apply adjustments to\n"), f);

	fputs("\n", f);
}

