/* opt-parser.h -- getopt wrapper header
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
#ifndef REDSHIFT_OPT_PARSER_H
#define REDSHIFT_OPT_PARSER_H


int parseopt(int argc, char *const *argv, const char *shortopts, const char **args, int *args_count);

char *coalesce_args(const char *const *args, int args_count, char delimiter, char final);


#endif /* ! REDSHIFT_OPT_PARSER_H */
