/* pipeutils.h -- Utilities for using pipes as signals
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

   Copyright (c) 2017  Jon Lund Steffensen <jonlst@gmail.com>
*/

#ifndef REDSHIFT_PIPEUTILS_H
#define REDSHIFT_PIPEUTILS_H

int pipeutils_create_nonblocking(int pipefds[2]);

void pipeutils_signal(int write_fd);
void pipeutils_handle_signal(int read_fd);

#endif /* ! REDSHIFT_PIPEUTILS_H */
