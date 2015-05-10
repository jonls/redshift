#include <stdio.h>
#include <stdlib.h>
#include "config-ini.h"
#include "location.h"
#include <string.h>

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

void
print_provider_list()
{
	fputs(_("Available location providers:\n"), stdout);
	for (int i = 0; location_providers[i].name != NULL; i++) {
		printf("  %s\n", location_providers[i].name);
	}

	fputs("\n", stdout);
	fputs(_("Specify colon-separated options with"
		"`-l PROVIDER:OPTIONS'.\n"), stdout);
	/* TRANSLATORS: `help' must not be translated. */
	fputs(_("Try `-l PROVIDER:help' for help.\n"), stdout);
}

const location_provider_t *
find_location_provider(const char *name)
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

int
provider_try_start(const location_provider_t *provider,
		   location_state_t *state,
		   config_ini_state_t *config, char *args)
{
	int r;

	r = provider->init(state);
	if (r < 0) {
		fprintf(stderr, _("Initialization of %s failed.\n"),
			provider->name);
		return -1;
	}

	/* Set provider options from config file. */
	config_ini_section_t *section =
		config_ini_get_section(config, provider->name);
	if (section != NULL) {
		config_ini_setting_t *setting = section->settings;
		while (setting != NULL) {
			r = provider->set_option(state, setting->name,
						 setting->value);
			if (r < 0) {
				provider->free(state);
				fprintf(stderr, _("Failed to set %s"
						  " option.\n"),
					provider->name);
				/* TRANSLATORS: `help' must not be
				   translated. */
				fprintf(stderr, _("Try `-l %s:help' for more"
						  " information.\n"),
					provider->name);
				return -1;
			}
			setting = setting->next;
		}
	}

	/* Set provider options from command line. */
	const char *manual_keys[] = { "lat", "lon" };
	int i = 0;
	while (args != NULL) {
		char *next_arg = strchr(args, ':');
		if (next_arg != NULL) *(next_arg++) = '\0';

		const char *key = args;
		char *value = strchr(args, '=');
		if (value == NULL) {
			/* The options for the "manual" method can be set
			   without keys on the command line for convencience
			   and for backwards compatability. We add the proper
			   keys here before calling set_option(). */
			if (strcmp(provider->name, "manual") == 0 &&
				i < sizeof(manual_keys)/sizeof(manual_keys[0])) {
				key = manual_keys[i];
				value = args;
			} else {
				fprintf(stderr, _("Failed to parse option `%s'.\n"),
					args);
				return -1;
			}
		} else {
			*(value++) = '\0';
		}

		r = provider->set_option(state, key, value);
		if (r < 0) {
			provider->free(state);
			fprintf(stderr, _("Failed to set %s option.\n"),
				provider->name);
			/* TRANSLATORS: `help' must not be translated. */
			fprintf(stderr, _("Try `-l %s:help' for more"
					  " information.\n"), provider->name);
			return -1;
		}

		args = next_arg;
		i += 1;
	}

	/* Start provider. */
	r = provider->start(state);
	if (r < 0) {
		provider->free(state);
		fprintf(stderr, _("Failed to start provider %s.\n"),
			provider->name);
		return -1;
	}

	return 0;
}

const location_provider_t *get_first_valid_provider(location_state_t *location_state, config_ini_state_t *config) {
	const location_provider_t *provider, *p;
	provider=NULL;
	int r, i;
	for (i = 0; location_providers[i].name != NULL; i++) {
		p = &location_providers[i];
		fprintf(stderr, _("Trying location provider `%s'...\n"), p->name);
		r = provider_try_start(p, location_state, config, NULL);
		if (r < 0) {
			fputs(_("Trying next provider...\n"), stderr);
			continue;
		}

		/* Found provider that works. */
		provider=p;
		printf(_("Using provider `%s'.\n"), provider->name);
		break;
	}

	/* Print error if no providers were successful at this point. */
	if (provider == NULL) {
		fputs(_("No more location providers to try.\n"), stderr);
	}
	/* Returns a valid provider or NULL */
	return provider;
}

