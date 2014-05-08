/* systemtime.c -- Portable system time source
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

#ifndef _WIN32
# include <time.h>
#endif

#ifdef __MACH__
# include <mach/clock.h>
# include <mach/mach.h>
#endif

#include "systemtime.h"

int
systemtime_get_time(double *t)
{

#ifdef _WIN32
	FILETIME now;
	ULARGE_INTEGER i;
	GetSystemTimeAsFileTime(&now);
	i.LowPart = now.dwLowDateTime;
	i.HighPart = now.dwHighDateTime;

	/* FILETIME is tenths of microseconds since 1601-01-01 UTC */
	*t = (i.QuadPart / 10000000.0) - 11644473600.0;

#else
	struct timespec now;

	#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
		clock_serv_t cclock;
		mach_timespec_t mts;
		host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
		clock_get_time(cclock, &mts);
		mach_port_deallocate(mach_task_self(), cclock);
		now.tv_sec = mts.tv_sec;
		now.tv_nsec = mts.tv_nsec;
		
	#else // Regular Linux
		int r = clock_gettime(CLOCK_REALTIME, &now);
		if (r < 0) {
			perror("clock_gettime");
			return -1;
		}

		*t = now.tv_sec + (now.tv_nsec / 1000000000.0);

	#endif

#endif

	return 0;
}
