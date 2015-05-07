#ifndef GAMMA_H
#define GAMMA_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "redshift.h"

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

void print_method_list();
const gamma_method_t *find_gamma_method(const char *name);
const gamma_method_t *get_first_valid_method(gamma_state_t *gamma_state, config_ini_state_t *config); 
int method_try_start(const gamma_method_t *method, gamma_state_t *state, config_ini_state_t *config, char *args);
gamma_state_t *gammma_state_new(void);
void gamma_state_finalize(gamma_state_t *state);

#endif /* GAMMA_H */
