#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config-ini.h"
#include "gamma-adjustment.h"

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

void
print_method_list()
{
	fputs(_("Available adjustment methods:\n"), stdout);
	for (int i = 0; gamma_methods[i].name != NULL; i++) {
		printf("  %s\n", gamma_methods[i].name);
	}

	fputs("\n", stdout);
	fputs(_("Specify colon-separated options with"
		" `-m METHOD:OPTIONS'.\n"), stdout);
	/* TRANSLATORS: `help' must not be translated. */
	fputs(_("Try `-m METHOD:help' for help.\n"), stdout);
}

const gamma_method_t *
find_gamma_method(const char *name)
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

int
method_try_start(const gamma_method_t *method,
		 gamma_state_t *state,
		 config_ini_state_t *config, char *args)
{
	int r;

	r = method->init(state);
	if (r < 0) {
		fprintf(stderr, _("Initialization of %s failed.\n"),
			method->name);
		return -1;
	}

	/* Set method options from config file. */
	config_ini_section_t *section =
		config_ini_get_section(config, method->name);
	if (section != NULL) {
		config_ini_setting_t *setting = section->settings;
		while (setting != NULL) {
			r = method->set_option(state, setting->name,
						 setting->value);
			if (r < 0) {
				method->free(state);
				fprintf(stderr, _("Failed to set %s"
						  " option.\n"),
					method->name);
				/* TRANSLATORS: `help' must not be
				   translated. */
				fprintf(stderr, _("Try `-m %s:help' for more"
						  " information.\n"),
					method->name);
				return -1;
			}
			setting = setting->next;
		}
	}

	/* Set method options from command line. */
	while (args != NULL) {
		char *next_arg = strchr(args, ':');
		if (next_arg != NULL) *(next_arg++) = '\0';

		const char *key = args;
		char *value = strchr(args, '=');
		if (value == NULL) {
			fprintf(stderr, _("Failed to parse option `%s'.\n"),
				args);
			return -1;
		} else {
			*(value++) = '\0';
		}

		r = method->set_option(state, key, value);
		if (r < 0) {
			method->free(state);
			fprintf(stderr, _("Failed to set %s option.\n"),
				method->name);
			/* TRANSLATORS: `help' must not be translated. */
			fprintf(stderr, _("Try -m %s:help' for more"
					  " information.\n"), method->name);
			return -1;
		}

		args = next_arg;
	}

	/* Start method. */
	r = method->start(state);
	if (r < 0) {
		method->free(state);
		fprintf(stderr, _("Failed to start adjustment method %s.\n"),
			method->name);
		return -1;
	}

	return 0;
}

const gamma_method_t *get_first_valid_method(gamma_state_t *gamma_state, config_ini_state_t *config) {
	const gamma_method_t *m, *method; 
	method = NULL;
	int r, i;
	for (i = 0; gamma_methods[i].name != NULL; i++) {
		m = &gamma_methods[i];
		if (!m->autostart) continue;

		r = method_try_start(m, gamma_state, config, NULL);
		if (r < 0) {
			fputs(_("Trying next method...\n"), stderr);
			method=NULL;
			continue;
		}
	
		/* Found method that works. */
		method = m;
		printf(_("Using method `%s'.\n"), method->name);
		break;
	}

	/* Print error if no methods were successful at this point. */
	if (method == NULL) {
		fputs(_("No more methods to try.\n"), stderr);
	}
	/* Return valid method or NULL */
	return method;
}

gamma_state_t *gammma_state_new(void) 
{
	gamma_state_t *gamma_state = malloc(sizeof(gamma_state_t));
	return gamma_state;
}

void gamma_state_finalize(gamma_state_t *state) 
{
	if (state != NULL) {
		free(state);
	}
}
