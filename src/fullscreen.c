/* fullscreen.h -- Fullscreen detector
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

   Copyright (c) 2021  Angelo Elias Dalzotto <angelodalzotto97@gmail.com>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#ifndef _WIN32
# include <X11/Xlib.h>
#endif

#include "fullscreen.h"
#include "redshift.h"

#ifndef _WIN32
static Display *display;
#endif

static int
fullscreen_init()
{
#ifndef _WIN32
	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, _("X request failed: %s\n"), "XOpenDisplay");
		return -1;
	}
#endif

	return 0;
}

static int
fullscreen_check()
{
#ifndef _WIN32
	Window window;
	int revert_to = RevertToParent;
	int result = XGetInputFocus(display, &window, &revert_to);	

	int win_x, win_y, win_w, win_h, win_b, win_d;
	int scr_x, scr_y, scr_w, scr_h, scr_b, scr_d;
	if (result) {
		Window rootWindow;
		result = XGetGeometry(display, window, &rootWindow, &win_x, &win_y, &win_w, &win_h, &win_b, &win_d);
		if (rootWindow) {
			result = XGetGeometry(display, rootWindow, &rootWindow, &scr_x, &scr_y, &scr_w, &scr_h, &scr_b, &scr_d);
		}
	}

	if (result && win_w == scr_w && win_h == scr_h) {
		return 1;
	} else {
#endif
		return 0;
#ifndef _WIN32
	}
#endif
}

const fullscreen_t fullscreen = {
	"fullscreen",
	(fullscreen_init_func *)fullscreen_init,
	(fullscreen_check_func *)fullscreen_check,
};
