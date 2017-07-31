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

   Copyright (c) 2010-2014  Jon Lund Steffensen <jonlst@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>

#ifndef WINVER
# define WINVER  0x0500
#endif
#include <windows.h>
#include <wingdi.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "gamma-w32gdi.h"
#include "colorramp.h"

#define GAMMA_RAMP_SIZE  256


int
w32gdi_init(w32gdi_state_t *state)
{
	state->saved_ramps = NULL;
	state->preserve = 0;

	return 0;
}

int
w32gdi_start(w32gdi_state_t *state, program_mode_t mode)
{
	BOOL r;

	/* Open device context */
	HDC hDC = GetDC(NULL);
	if (hDC == NULL) {
		fputs(_("Unable to open device context.\n"), stderr);
		return -1;
	}

	/* Check support for gamma ramps */
	int cmcap = GetDeviceCaps(hDC, COLORMGMTCAPS);
	if (cmcap != CM_GAMMA_RAMP) {
		fputs(_("Display device does not support gamma ramps.\n"),
		      stderr);
		return -1;
	}

	/* Allocate space for saved gamma ramps */
	state->saved_ramps = malloc(3*GAMMA_RAMP_SIZE*sizeof(WORD));
	if (state->saved_ramps == NULL) {
		perror("malloc");
		ReleaseDC(NULL, hDC);
		return -1;
	}

	/* Save current gamma ramps so we can restore them at program exit */
	r = GetDeviceGammaRamp(hDC, state->saved_ramps);
	if (!r) {
		fputs(_("Unable to save current gamma ramp.\n"), stderr);
		ReleaseDC(NULL, hDC);
		return -1;
	}

	/* Release device context */
	ReleaseDC(NULL, hDC);

	return 0;
}

void
w32gdi_free(w32gdi_state_t *state)
{
	/* Free saved ramps */
	free(state->saved_ramps);
}


void
w32gdi_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with the Windows GDI.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: Windows GDI help output
	   left column must not be translated */
	fputs(_("  preserve={0,1}\tWhether existing gamma should be"
		" preserved\n"),
	      f);
	fputs("\n", f);
}

int
w32gdi_set_option(w32gdi_state_t *state, const char *key, const char *value)
{
	if (strcasecmp(key, "preserve") == 0) {
		state->preserve = atoi(value);
	} else {
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}

	return 0;
}

void
w32gdi_restore(w32gdi_state_t *state)
{
	/* Open device context */
	HDC hDC = GetDC(NULL);
	if (hDC == NULL) {
		fputs(_("Unable to open device context.\n"), stderr);
		return;
	}

	/* Restore gamma ramps */
	BOOL r = SetDeviceGammaRamp(hDC, state->saved_ramps);
	if (!r) fputs(_("Unable to restore gamma ramps.\n"), stderr);

	/* Release device context */
	ReleaseDC(NULL, hDC);
}

int
w32gdi_set_temperature(w32gdi_state_t *state,
		       const color_setting_t *setting)
{
	BOOL r;

	/* Open device context */
	HDC hDC = GetDC(NULL);
	if (hDC == NULL) {
		fputs(_("Unable to open device context.\n"), stderr);
		return -1;
	}

	/* Create new gamma ramps */
	WORD *gamma_ramps = malloc(3*GAMMA_RAMP_SIZE*sizeof(WORD));
	if (gamma_ramps == NULL) {
		perror("malloc");
		ReleaseDC(NULL, hDC);
		return -1;
	}

	WORD *gamma_r = &gamma_ramps[0*GAMMA_RAMP_SIZE];
	WORD *gamma_g = &gamma_ramps[1*GAMMA_RAMP_SIZE];
	WORD *gamma_b = &gamma_ramps[2*GAMMA_RAMP_SIZE];

	if (state->preserve) {
		/* Initialize gamma ramps from saved state */
		memcpy(gamma_ramps, state->saved_ramps,
		       3*GAMMA_RAMP_SIZE*sizeof(WORD));
	} else {
		/* Initialize gamma ramps to pure state */
		for (int i = 0; i < GAMMA_RAMP_SIZE; i++) {
			WORD value = (double)i/GAMMA_RAMP_SIZE *
				(UINT16_MAX+1);
			gamma_r[i] = value;
			gamma_g[i] = value;
			gamma_b[i] = value;
		}
	}

	colorramp_fill_u16(gamma_r, gamma_g, gamma_b, GAMMA_RAMP_SIZE,
			   GAMMA_RAMP_SIZE, GAMMA_RAMP_SIZE, setting);

	/* Set new gamma ramps */
	r = SetDeviceGammaRamp(hDC, gamma_ramps);
	if (!r) {
		/* TODO it happens that SetDeviceGammaRamp returns FALSE on
		   occasions where the adjustment seems to be successful.
		   Does this only happen with multiple monitors connected? */
		fputs(_("Unable to set gamma ramps.\n"), stderr);
		free(gamma_ramps);
		ReleaseDC(NULL, hDC);
		return -1;
	}

	free(gamma_ramps);

	/* Release device context */
	ReleaseDC(NULL, hDC);

	return 0;
}
