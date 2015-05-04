#ifndef REDSHIFT_GAMMA_H
#define REDSHIFT_GAMMA_H

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Gamma adjustment method */
typedef int gamma_method_init_func(void *state);
typedef int gamma_method_start_func(void *state);
typedef void gamma_method_free_func(void *state);
typedef void gamma_method_print_help_func(FILE *f);
typedef int gamma_method_set_option_func(void *state, const char *key,
					 const char *value);
typedef void gamma_method_restore_func(void *state);
typedef int gamma_method_set_temperature_func(void *state,
					      const color_setting_t *setting);

typedef struct {
	char *name;

	/* If true, this method will be tried if none is explicitly chosen. */
	int autostart;

	/* Initialize state. Options can be set between init and start. */
	gamma_method_init_func *init;
	/* Allocate storage and make connections that depend on options. */
	gamma_method_start_func *start;
	/* Free all allocated storage and close connections. */
	gamma_method_free_func *free;

	/* Print help on options for this adjustment method. */
	gamma_method_print_help_func *print_help;
	/* Set an option key, value-pair */
	gamma_method_set_option_func *set_option;

	/* Restore the adjustment to the state before start was called. */
	gamma_method_restore_func *restore;
	/* Set a specific color temperature. */
	gamma_method_set_temperature_func *set_temperature;
} gamma_method_t;

#include "gamma-dummy.h"

#ifdef ENABLE_DRM
# include "gamma-drm.h"
#endif

#ifdef ENABLE_RANDR
# include "gamma-randr.h"
#endif

#ifdef ENABLE_VIDMODE
# include "gamma-vidmode.h"
#endif

#ifdef ENABLE_QUARTZ
# include "gamma-quartz.h"
#endif

#ifdef ENABLE_WINGDI
# include "gamma-w32gdi.h"
#endif

/* Union of state data for gamma adjustment methods */
typedef union {
#ifdef ENABLE_DRM
	drm_state_t drm;
#endif
#ifdef ENABLE_RANDR
	randr_state_t randr;
#endif
#ifdef ENABLE_VIDMODE
	vidmode_state_t vidmode;
#endif
#ifdef ENABLE_QUARTZ
	quartz_state_t quartz;
#endif
#ifdef ENABLE_WINGDI
	w32gdi_state_t w32gdi;
#endif
} gamma_state_t;

/* Gamma adjustment method structs */
static const gamma_method_t gamma_methods[] = {
#ifdef ENABLE_DRM
	{
		"drm", 0,
		(gamma_method_init_func *)drm_init,
		(gamma_method_start_func *)drm_start,
		(gamma_method_free_func *)drm_free,
		(gamma_method_print_help_func *)drm_print_help,
		(gamma_method_set_option_func *)drm_set_option,
		(gamma_method_restore_func *)drm_restore,
		(gamma_method_set_temperature_func *)drm_set_temperature
	},
#endif
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
#ifdef ENABLE_QUARTZ
	{
		"quartz", 1,
		(gamma_method_init_func *)quartz_init,
		(gamma_method_start_func *)quartz_start,
		(gamma_method_free_func *)quartz_free,
		(gamma_method_print_help_func *)quartz_print_help,
		(gamma_method_set_option_func *)quartz_set_option,
		(gamma_method_restore_func *)quartz_restore,
		(gamma_method_set_temperature_func *)quartz_set_temperature
	},
#endif
#ifdef ENABLE_WINGDI
	{
		"wingdi", 1,
		(gamma_method_init_func *)w32gdi_init,
		(gamma_method_start_func *)w32gdi_start,
		(gamma_method_free_func *)w32gdi_free,
		(gamma_method_print_help_func *)w32gdi_print_help,
		(gamma_method_set_option_func *)w32gdi_set_option,
		(gamma_method_restore_func *)w32gdi_restore,
		(gamma_method_set_temperature_func *)w32gdi_set_temperature
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

#endif /* REDSHIFT_GAMMA_H */
