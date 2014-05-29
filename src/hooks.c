/* redshift.h -- Main program header
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

   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#include "hooks.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
extern char **environ;



static char **hooks[3] = { NULL, NULL, NULL };
static size_t hooks_n[3] = { 0, 0, 0 };



#ifndef _WIN32


void
run_hooks(int event, int silence)
{
	char *command[] = { "sh", "-c", "", NULL };
	pid_t pid = silence ? fork() : vfork();
	if (pid == (pid_t)-1)
		perror(silence ? "fork" : "vfork");
	if (pid) return;

	/* Make sure out does not interfere with front-ends */
	if (silence) {
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}

	size_t i;
	for (i = 0; i < hooks_n[event]; i++) {
		command[2] = hooks[event][i];
		pid_t pid = vfork();
		if (pid == 0) {
			execve("/bin/sh", command, environ);
			exit(1);
		}
	}

	exit(0);
}

int
add_hook(int event, const char *action)
{
	size_t i = hooks_n[event]++;
	hooks[event] = realloc(hooks[event], hooks_n[event] * sizeof(char*));
	if (hooks[event] == NULL) {
		perror("realloc");
		return -1;
	}
	hooks[event][i] = strdup(action);
	if (hooks[event][i] == NULL) {
		perror("strdup");
		return -1;
	}
	return 0;
}

void
free_hooks(void)
{
	size_t i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < hooks_n[i]; j++) {
			free(hooks[i][j]);
		}
		free(hooks[i]);
	}
}


#else /* ! _WIN32 */


void
run_hooks(int event, int silence)
{
	(void) event;
	(void) silence;
}

int
add_hook(int event, const char *action)
{
	(void) event;
	(void) action;
	return 0;
}

void
free_hooks(void)
{
	/* do nothing */
}


#endif /* ! _WIN32 */

