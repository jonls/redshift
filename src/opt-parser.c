/* opt-parser.c -- getopt wrapper source
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
#include "opt-parser.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>


int
parseopt(int argc, char *const *argv, const char *shortopts, const char **args, int *args_count)
{
	int opt;
	char *p;

	*args_count = 0;
	opt = getopt(argc, argv, shortopts);
	if (opt < 0)
		return opt;

	p = strchr(shortopts, opt);
	if ((p == NULL) || (p[1] != ':'))
		return opt;

	args[(*args_count)++] = optarg;
	while (optind < argc && argv[optind][0] != '-') {
		args[(*args_count)++] = argv[optind++];
	}

	return opt;
}


char *
coalesce_args(const char *const *args, int args_count, char delimiter, char final)
{
	size_t n = args_count ? (args_count + 1) : 2;
	size_t i;
	char *rc;
	char *rc_;

	for (i = 0; i < args_count; i++)
		n += strlen(args[i]);

	rc = rc_ = malloc(n * sizeof(char));
	if (rc == NULL)
		return NULL;

	for (i = 0; i < args_count; i++) {
		n = strlen(args[i]);
		memcpy(rc_, args[i], n * sizeof(char));
		rc_ += n;
		*rc_++ = delimiter;
	}
	if (args_count > 0)
		rc_--;

	*rc_++ = final;
	*rc_++ = '\0';

	return rc;
}

