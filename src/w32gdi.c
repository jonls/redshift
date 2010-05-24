/* w32gdi.c -- Windows GDI gamma adjustment source
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

#include <stdlib.h>
#include <stdio.h>

#define WINVER  0x0500
#include <windows.h>
#include <wingdi.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include "w32gdi.h"
#include "colorramp.h"

#define GAMMA_RAMP_SIZE  256


int
w32gdi_init(w32gdi_state_t *state, int screen_num, int crtc_num)
{
	BOOL r;

	/* Open device context */
	state->hDC = GetDC(NULL);
	if (state->hDC == NULL) {
		fputs(_("Unable to open device context.\n"), stderr);
		return -1;
	}

	/* Check support for gamma ramps */
	int cmcap = GetDeviceCaps(state->hDC, COLORMGMTCAPS);
	if (cmcap != CM_GAMMA_RAMP) {
		fputs(_("Display device does not support gamma ramps.\n"),
		      stderr);
		return -1;
	}

	/* Allocate space for saved gamma ramps */
	state->saved_ramps = malloc(3*GAMMA_RAMP_SIZE*sizeof(WORD));
	if (state->saved_ramps == NULL) {
		perror("malloc");
		ReleaseDC(NULL, state->hDC);
		return -1;
	}

	/* Save current gamma ramps so we can restore them at program exit */
	r = GetDeviceGammaRamp(state->hDC, state->saved_ramps);
	if (!r) {
		fputs(_("Unable to save current gamma ramp.\n"), stderr);
		ReleaseDC(NULL, state->hDC);
		return -1;
	}

	return 0;
}

void
w32gdi_free(w32gdi_state_t *state)
{
	/* Free saved ramps */
	free(state->saved_ramps);

	/* Release device context */
	ReleaseDC(NULL, state->hDC);
}

void
w32gdi_restore(w32gdi_state_t *state)
{
	/* Restore gamma ramps */
	BOOL r = SetDeviceGammaRamp(state->hDC, state->saved_ramps);
	if (!r) fputs(_("Unable to restore gamma ramps.\n"), stderr);
}

int
w32gdi_set_temperature(w32gdi_state_t *state, int temp, float gamma[3])
{
	BOOL r;

	/* Create new gamma ramps */
	WORD *gamma_ramps = malloc(3*GAMMA_RAMP_SIZE*sizeof(WORD));
	if (gamma_ramps == NULL) {
		perror("malloc");
		return -1;
	}

	WORD *gamma_r = &gamma_ramps[0*GAMMA_RAMP_SIZE];
	WORD *gamma_g = &gamma_ramps[1*GAMMA_RAMP_SIZE];
	WORD *gamma_b = &gamma_ramps[2*GAMMA_RAMP_SIZE];

	colorramp_fill(gamma_r, gamma_g, gamma_b, GAMMA_RAMP_SIZE,
		       temp, gamma);

	/* Set new gamma ramps */
	r = SetDeviceGammaRamp(state->hDC, gamma_ramps);
	if (!r) {
		fputs(_("Unable to set gamma ramps.\n"), stderr);
		free(gamma_ramps);
		return -1;
	}

	free(gamma_ramps);

	return 0;
}
