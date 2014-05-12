/* gamma-w32gdi.c -- Windows GDI gamma adjustment source
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
   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WINVER
# define WINVER  0x0500
#endif
#ifdef FAKE_W32GDI
#  include "fake-w32gdi.h"
#else
#  include <windows.h>
#  include <wingdi.h>
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "gamma-w32gdi.h"

#define GAMMA_RAMP_SIZE  256


static void
w32gdi_free_crtc(void *data)
{
	/* Release device context. */
	ReleaseDC(NULL, data);
}

static int
w32gdi_open_site(gamma_server_state_t *state, char *site, gamma_site_state_t *site_out)
{
	(void) state;
	(void) site;
	site_out->data = NULL;
	site_out->partitions_available = 1;
	return 0;
}

static int
w32gdi_open_partition(gamma_server_state_t *state, gamma_site_state_t *site,
		      size_t partition, gamma_partition_state_t *partition_out)
{
	(void) state;
	(void) site;
	(void) partition;

	BOOL r;

	partition_out->data = NULL;
	partition_out->crtcs_available = 0;

	/* Count number of displays */
	DISPLAY_DEVICE display;
	display.cb = sizeof(DISPLAY_DEVICE);
	while (1) {
		r = EnumDisplayDevices(NULL, partition_out->crtcs_available, &display, 0);
		if (!r) break;
		partition_out->crtcs_available++;
	}

	return 0;
}

static int
w32gdi_open_crtc(gamma_server_state_t *state, gamma_site_state_t *site,
		 gamma_partition_state_t *partition, size_t crtc, gamma_crtc_state_t *crtc_out)
{
	(void) state;
	(void) site;
	(void) partition;

	crtc_out->data = NULL;

	int r;

	/* Open device context. */
	DISPLAY_DEVICE display;
	display.cb = sizeof(DISPLAY_DEVICE);
	r = (int)EnumDisplayDevices(NULL, crtc, &display, 0);
	if (!r) {
		fputs(_("Cannot find display, are you unplugging stuff?\n"), stderr);
		return -1;
	}
	if (!(display.StateFlags & DISPLAY_DEVICE_ACTIVE)) {
		fputs(_("Cannot to open device context, it is not active.\n"), stderr);
		return -1;
	}
	HDC hDC = CreateDC(TEXT("DISPLAY"), display.DeviceName, NULL, NULL);
	if (hDC == NULL) {
		fputs(_("Unable to open device context.\n"), stderr);
		return -1;
	}

	/* Check support for gamma ramps. */
	int cmcap = GetDeviceCaps(hDC, COLORMGMTCAPS);
	if (cmcap != CM_GAMMA_RAMP) {
		fputs(_("Display device does not support gamma ramps.\n"),
		      stderr);
		ReleaseDC(NULL, hDC);
		return -1;
	}

	/* Specify gamma ramp dimensions. */
	crtc_out->saved_ramps.red_size   = GAMMA_RAMP_SIZE;
	crtc_out->saved_ramps.green_size = GAMMA_RAMP_SIZE;
	crtc_out->saved_ramps.blue_size  = GAMMA_RAMP_SIZE;

	/* Allocate space for saved gamma ramps. */
	crtc_out->saved_ramps.red = malloc(3*GAMMA_RAMP_SIZE*sizeof(WORD));
	if (crtc_out->saved_ramps.red == NULL) {
		perror("malloc");
		ReleaseDC(NULL, hDC);
		return -1;
	}
	crtc_out->saved_ramps.green = crtc_out->saved_ramps.red   + GAMMA_RAMP_SIZE;
	crtc_out->saved_ramps.blue  = crtc_out->saved_ramps.green + GAMMA_RAMP_SIZE;

	/* Save current gamma ramps so we can restore them at program exit. */
	r = GetDeviceGammaRamp(hDC, crtc_out->saved_ramps.red);
	if (!r) {
		fputs(_("Unable to save current gamma ramp.\n"), stderr);
		ReleaseDC(NULL, hDC);
		return -1;
	}

	crtc_out->data = hDC;
	return 0;
}

static int
w32gdi_set_ramps(gamma_server_state_t *state, gamma_crtc_state_t *crtc, gamma_ramps_t ramps)
{
	(void) state;
	int r = SetDeviceGammaRamp(crtc->data, ramps.red);
	if (!r) {
		/* TODO it happens that SetDeviceGammaRamp returns FALSE on
		   occasions where the adjustment seems to be successful.
		   Does this only happen with multiple monitors connected? */
		fputs(_("Unable to set gamma ramps.\n"), stderr);
	}
	return !r;
}

static int
w32gdi_set_option(gamma_server_state_t *state, const char *key, char *value, ssize_t section)
{
	if (strcasecmp(key, "crtc") == 0) {
		ssize_t crtc = strcasecmp(value, "all") ? (ssize_t)atoi(value) : -1;
		if (crtc < 0 && strcasecmp(value, "all")) {
			/* TRANSLATORS: `all' must not be translated. */
			fprintf(stderr, _("CRTC must be `all' or a non-negative integer.\n"));
			return -1;
		}
		if (section >= 0) {
			state->selections[section].crtc = crtc;
		} else {
			for (size_t i = 0; i < state->selections_made; i++)
				state->selections[i].crtc = crtc;
		}
		return 0;
	}
	return 1;
}


int
w32gdi_init(gamma_server_state_t *state)
{
	int r;
	r = gamma_init(state);
	if (r != 0) return r;

	state->free_crtc_data = w32gdi_free_crtc;
	state->open_site      = w32gdi_open_site;
	state->open_partition = w32gdi_open_partition;
	state->open_crtc      = w32gdi_open_crtc;
	state->set_ramps      = w32gdi_set_ramps;
	state->set_option     = w32gdi_set_option;

	return 0;
}

int
w32gdi_start(gamma_server_state_t *state)
{
	return gamma_resolve_selections(state);
}

void
w32gdi_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with the Windows GDI.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: Windows GDI help output
	   left column must not be translated. */
	fputs(_("  crtc=N\tX monitor to apply adjustments to\n"), f);
	fputs("\n", f);
}
