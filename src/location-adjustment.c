#include <stdio.h>
#include <stdlib.h>

#include "location-adjustment.h"

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

