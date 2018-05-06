/* config-ini.c -- INI config file parser
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

   Copyright (c) 2010  Jon Lund Steffensen <jonlst@gmail.com>
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef _WIN32
# include <pwd.h>
#endif

#include "config-ini.h"

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#define MAX_CONFIG_PATH  4096
#define MAX_LINE_LENGTH   512


static FILE *
open_config_file(const char *filepath)
{
	FILE *f = NULL;

	/* If a path is not specified (filepath is NULL) then
	   the configuration file is searched for in the directories
	   specified by the XDG Base Directory Specification
	   <http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html>.

	   If HOME is not set, getpwuid() is consulted for the home directory. On
	   windows platforms the %localappdata% is used in place of XDG_CONFIG_HOME.
	*/

	if (filepath == NULL) {
		FILE *f = NULL;
		char cp[MAX_CONFIG_PATH];
		char *env;

		if (f == NULL && (env = getenv("XDG_CONFIG_HOME")) != NULL &&
		    env[0] != '\0') {
			snprintf(cp, sizeof(cp),
					 "%s/redshift/redshift.conf", env);
			f = fopen(cp, "r");
			if (f == NULL) {
				/* Fall back to formerly used path. */
				snprintf(cp, sizeof(cp),
						 "%s/redshift.conf", env);
				f = fopen(cp, "r");
			}
		}

#ifdef _WIN32
		if (f == NULL && (env = getenv("localappdata")) != NULL &&
		    env[0] != '\0') {
			snprintf(cp, sizeof(cp),
				 "%s\\redshift.conf", env);
			f = fopen(cp, "r");
		}
#endif
		if (f == NULL && (env = getenv("HOME")) != NULL &&
		    env[0] != '\0') {
			snprintf(cp, sizeof(cp),
				 "%s/.config/redshift/redshift.conf", env);
			f = fopen(cp, "r");
			if (f == NULL) {
				/* Fall back to formerly used path. */
				snprintf(cp, sizeof(cp),
					 "%s/.config/redshift.conf", env);
				f = fopen(cp, "r");
			}
		}
#ifndef _WIN32

		if (f == NULL) {
			struct passwd *pwd = getpwuid(getuid());
			char *home = pwd->pw_dir;
			snprintf(cp, sizeof(cp),
				 "%s/.config/redshift/redshift.conf", home);
			f = fopen(cp, "r");
			if (f == NULL) {
				/* Fall back to formerly used path. */
				snprintf(cp, sizeof(cp),
					 "%s/.config/redshift.conf", home);
				f = fopen(cp, "r");
			}
		}

		if (f == NULL && (env = getenv("XDG_CONFIG_DIRS")) != NULL &&
		    env[0] != '\0') {
			char *begin = env;
			while (1) {
				char *end = strchr(begin, ':');
				if (end == NULL) end = strchr(begin, '\0');

				int len = end - begin;
				if (len > 0) {
					snprintf(cp, sizeof(cp),
						 "%.*s/redshift/redshift.conf", len, begin);
					f = fopen(cp, "r");
					if (f != NULL) {
						/* Fall back to formerly used path. */
						snprintf(cp, sizeof(cp),
							 "%.*s/redshift.conf", len, begin);
						f = fopen(cp, "r");
					}
					if (f != NULL) break;
				}

				if (end[0] == '\0') break;
				begin = end + 1;
			}
		}

		if (f == NULL) {
			snprintf(cp, sizeof(cp),
				 "%s/redshift.conf", "/etc");
			f = fopen(cp, "r");
		}
#endif

		return f;
	} else {
		f = fopen(filepath, "r");
		if (f == NULL) {
			perror("fopen");
			return NULL;
		}
	}

	return f;
}

int
config_ini_init(config_ini_state_t *state, const char *filepath)
{
	config_ini_section_t *section = NULL;
	state->sections = NULL;

	FILE *f = open_config_file(filepath);
	if (f == NULL) {
		/* Only a serious error if a file was explicitly requested. */
		if (filepath != NULL) return -1;
		return 0;
	}

	char line[MAX_LINE_LENGTH];
	char *s;

	while (1) {
		/* Handle the file input linewise. */
		char *r = fgets(line, sizeof(line), f);
		if (r == NULL) break;

		/* Strip leading blanks and trailing newline. */
		s = line + strspn(line, " \t");
		s[strcspn(s, "\r\n")] = '\0';

		/* Skip comments and empty lines. */
		if (s[0] == ';' || s[0] == '\0') continue;

		if (s[0] == '[') {
			/* Read name of section. */
			const char *name = s+1;
			char *end = strchr(s, ']');
			if (end == NULL || end[1] != '\0' || end == name) {
				fputs(_("Malformed section header in config"
					" file.\n"), stderr);
				fclose(f);
				config_ini_free(state);
				return -1;
			}

			*end = '\0';

			/* Create section. */
			section = malloc(sizeof(config_ini_section_t));
			if (section == NULL) {
				fclose(f);
				config_ini_free(state);
				return -1;
			}

			/* Insert into section list. */
			section->name = NULL;
			section->settings = NULL;
			section->next = state->sections;
			state->sections = section;

			/* Copy section name. */
			section->name = malloc(end - name + 1);
			if (section->name == NULL) {
				fclose(f);
				config_ini_free(state);
				return -1;
			}

			memcpy(section->name, name, end - name + 1);
		} else {
			/* Split assignment at equals character. */
			char *end = strchr(s, '=');
			if (end == NULL || end == s) {
				fputs(_("Malformed assignment in config"
					" file.\n"), stderr);
				fclose(f);
				config_ini_free(state);
				return -1;
			}

			*end = '\0';
			char *value = end + 1;

			if (section == NULL) {
				fputs(_("Assignment outside section in config"
					" file.\n"), stderr);
				fclose(f);
				config_ini_free(state);
				return -1;
			}

			/* Create section. */
			config_ini_setting_t *setting =
				malloc(sizeof(config_ini_setting_t));
			if (setting == NULL) {
				fclose(f);
				config_ini_free(state);
				return -1;
			}

			/* Insert into section list. */
			setting->name = NULL;
			setting->value = NULL;
			setting->next = section->settings;
			section->settings = setting;

			/* Copy name of setting. */
			setting->name = malloc(end - s + 1);
			if (setting->name == NULL) {
				fclose(f);
				config_ini_free(state);
				return -1;
			}

			memcpy(setting->name, s, end - s + 1);

			/* Copy setting value. */
			size_t value_len = strlen(value) + 1;
			setting->value = malloc(value_len);
			if (setting->value == NULL) {
				fclose(f);
				config_ini_free(state);
				return -1;
			}

			memcpy(setting->value, value, value_len);
		}
	}

	fclose(f);

	return 0;
}

void
config_ini_free(config_ini_state_t *state)
{
	config_ini_section_t *section = state->sections;

	while (section != NULL) {
		config_ini_setting_t *setting = section->settings;
		config_ini_section_t *section_prev = section;

		while (setting != NULL) {
			config_ini_setting_t *setting_prev = setting;
			free(setting->name);
			free(setting->value);
			setting = setting->next;
			free(setting_prev);
		}

		free(section->name);
		section = section->next;
		free(section_prev);
	}
}

config_ini_section_t *
config_ini_get_section(config_ini_state_t *state, const char *name)
{
	config_ini_section_t *section = state->sections;
	while (section != NULL) {
		if (strcasecmp(section->name, name) == 0) {
			return section;
		}
		section = section->next;
	}

	return NULL;
}
