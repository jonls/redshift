/* signals.h -- Signal processing header
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

   Copyright (c) 2009-2015  Jon Lund Steffensen <jonlst@gmail.com>
   Copyright (c) 2015  Mattias Andrée <maandree@member.fsf.org>
*/
#ifndef REDSHIFT_SIGNALS_H
#define REDSHIFT_SIGNALS_H


#if defined(HAVE_SIGNAL_H) && !defined(__WIN32__)

extern volatile sig_atomic_t exiting;
extern volatile sig_atomic_t disable;

#else /* ! HAVE_SIGNAL_H || __WIN32__ */
extern int exiting;
extern int disable;
#endif /* ! HAVE_SIGNAL_H || __WIN32__ */


int signals_install_handlers(void);


#endif /* REDSHIFT_SIGNALS_H */
