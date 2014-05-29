/* hooks.c -- Hooks triggered by events
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

   Copyright (c) 2014  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#ifndef _WIN32
# include <pwd.h>
#endif

#include "hooks.h"
#include "redshift.h"

#define MAX_HOOK_PATH  4096


/* Names of periods supplied to scripts. */
static const char *period_names[] = {
	"none",
	"daytime",
	"night",
	"transition"
};


/* Try to open the directory containing hooks. HP is a string
   of MAX_HOOK_PATH length that will be filled with the path
   of the returned directory. */
static DIR *
open_hooks_dir(char *hp)
{
	char *env;

	if ((env = getenv("XDG_CONFIG_HOME")) != NULL &&
	    env[0] != '\0') {
		snprintf(hp, MAX_HOOK_PATH, "%s/redshift/hooks", env);
		return opendir(hp);
	}

	if ((env = getenv("HOME")) != NULL &&
	    env[0] != '\0') {
		snprintf(hp, MAX_HOOK_PATH, "%s/.config/redshift/hooks", env);
		return opendir(hp);
	}

#ifndef _WIN32
	struct passwd *pwd = getpwuid(getuid());
	snprintf(hp, MAX_HOOK_PATH, "%s/.config/redshift/hooks", pwd->pw_dir);
	return opendir(hp);
#else
	return NULL;
#endif
}

/* Run hooks with a signal that the period changed. */
void
hooks_signal_period_change(period_t prev_period, period_t period)
{
	char hooksdir_path[MAX_HOOK_PATH];
	DIR *hooks_dir = open_hooks_dir(hooksdir_path);
	if (hooks_dir == NULL) return;

	struct dirent* ent;
	while ((ent = readdir(hooks_dir)) != NULL) {
		/* Skip hidden and special files (., ..) */
		if (ent->d_name[0] == '\0' || ent->d_name[0] == '.') continue;

		char *hook_name = ent->d_name;
		char hook_path[MAX_HOOK_PATH];
		snprintf(hook_path, sizeof(hook_path), "%s/%s",
			 hooksdir_path, hook_name);

#ifndef _WIN32
		/* Fork and exec the hook. We close stdout
		   so the hook cannot interfere with the normal
		   output. */
		pid_t pid = fork();
		if (pid == (pid_t)-1) {
			perror("fork");
			continue;
		} else if (pid == 0) { /* Child */
			close(STDOUT_FILENO);

			int r = execl(hook_path, hook_name,
				      "period-changed",
				      period_names[prev_period],
				      period_names[period], NULL);
			if (r < 0 && errno != EACCES) perror("execl");

			/* Only reached on error */
			_exit(EXIT_FAILURE);
		}
#endif
	}
}
