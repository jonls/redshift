#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config-ini.h"
#include "gamma-adjustment.h"

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
