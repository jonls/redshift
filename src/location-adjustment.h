#ifndef LOCATION_ADJUSTMENT_H
#define LOCATION_ADJUSTMENT_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "redshift.h"

/* Location provider */
typedef int location_provider_init_func(void *state);
typedef int location_provider_start_func(void *state);
typedef void location_provider_free_func(void *state);
typedef void location_provider_print_help_func(FILE *f);
typedef int location_provider_set_option_func(void *state, const char *key,
					      const char *value);
typedef int location_provider_get_location_func(void *state, location_t *loc);

typedef struct {
	char *name;

	/* Initialize state. Options can be set between init and start. */
	location_provider_init_func *init;
	/* Allocate storage and make connections that depend on options. */
	location_provider_start_func *start;
	/* Free all allocated storage and close connections. */
	location_provider_free_func *free;

	/* Print help on options for this location provider. */
	location_provider_print_help_func *print_help;
	/* Set an option key, value-pair. */
	location_provider_set_option_func *set_option;

	/* Get current location. */
	location_provider_get_location_func *get_location;
} location_provider_t;


/* Location Headers */
#include "location-manual.h"

#ifdef ENABLE_GEOCLUE
# include "location-geoclue.h"
#endif

#ifdef ENABLE_GEOCLUE2
# include "location-geoclue2.h"
#endif

#ifdef ENABLE_CORELOCATION
# include "location-corelocation.h"
#endif



/* Union of state data for location providers */
typedef union {
	location_manual_state_t manual;
#ifdef ENABLE_GEOCLUE
	location_geoclue_state_t geoclue;
#endif
} location_state_t;


/* Location provider method structs */
static const location_provider_t location_providers[] = {
#ifdef ENABLE_GEOCLUE
	{
		"geoclue",
		(location_provider_init_func *)location_geoclue_init,
		(location_provider_start_func *)location_geoclue_start,
		(location_provider_free_func *)location_geoclue_free,
		(location_provider_print_help_func *)
		location_geoclue_print_help,
		(location_provider_set_option_func *)
		location_geoclue_set_option,
		(location_provider_get_location_func *)
		location_geoclue_get_location
	},
#endif
#ifdef ENABLE_GEOCLUE2
	{
		"geoclue2",
		(location_provider_init_func *)location_geoclue2_init,
		(location_provider_start_func *)location_geoclue2_start,
		(location_provider_free_func *)location_geoclue2_free,
		(location_provider_print_help_func *)
		location_geoclue2_print_help,
		(location_provider_set_option_func *)
		location_geoclue2_set_option,
		(location_provider_get_location_func *)
		location_geoclue2_get_location
	},
#endif
#ifdef ENABLE_CORELOCATION
	{
		"corelocation",
		(location_provider_init_func *)location_corelocation_init,
		(location_provider_start_func *)location_corelocation_start,
		(location_provider_free_func *)location_corelocation_free,
		(location_provider_print_help_func *)
		location_corelocation_print_help,
		(location_provider_set_option_func *)
		location_corelocation_set_option,
		(location_provider_get_location_func *)
		location_corelocation_get_location
	},
#endif
	{
		"manual",
		(location_provider_init_func *)location_manual_init,
		(location_provider_start_func *)location_manual_start,
		(location_provider_free_func *)location_manual_free,
		(location_provider_print_help_func *)
		location_manual_print_help,
		(location_provider_set_option_func *)
		location_manual_set_option,
		(location_provider_get_location_func *)
		location_manual_get_location
	},
	{ NULL }
};

void print_provider_list();
const location_provider_t *find_location_provider(const char *name);
int provider_try_start(const location_provider_t *provider, location_state_t *state, config_ini_state_t *config, char *args);
const location_provider_t *get_first_valid_provider(location_state_t *location_state, config_ini_state_t *config);

#endif /* LOCATION_ADJUSTMENT_H */
