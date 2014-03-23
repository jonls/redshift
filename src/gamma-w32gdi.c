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
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#include "redshift.h"

#define GAMMA_RAMP_SIZE  256


int
w32gdi_init(w32gdi_state_t *state)
{
	state->saved_ramps = NULL;
	state->gamma[0] = DEFAULT_GAMMA;
	state->gamma[1] = DEFAULT_GAMMA;
	state->gamma[2] = DEFAULT_GAMMA;
	state->gamma_r = NULL;
	state->gamma_g = NULL;
	state->gamma_b = NULL;

	return 0;
}

int
w32gdi_start(w32gdi_state_t *state)
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

	/* Allocate space for gamma ramps */
	WORD* gamma_ramps = malloc(3*GAMMA_RAMP_SIZE*sizeof(WORD));
	if (gamma_ramps == NULL) {
		perror("malloc");
		ReleaseDC(NULL, hDC);
		return -1;
	}

	state->gamma_r = gamma_ramps;
	state->gamma_g = state->gamma_r + GAMMA_RAMP_SIZE;
	state->gamma_b = state->gamma_g + GAMMA_RAMP_SIZE;

	return 0;
}

void
w32gdi_free(w32gdi_state_t *state)
{
	/* Free saved ramps */
	free(state->saved_ramps);
	free(state->gamma_r);
}


void
w32gdi_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with the Windows GDI.\n"), f);
	fputs("\n", f);
}

int
w32gdi_set_option(w32gdi_state_t *state, const char *key, const char *value, int section)
{
	if (strcasecmp(key, "gamma") == 0) {
		float gamma[3];
		if (parse_gamma_string(value, gamma) < 0) {
			fputs(_("Malformed gamma setting.\n"),
			      stderr);
			return -1;
		}
		if (gamma[0] < MIN_GAMMA || gamma[0] > MAX_GAMMA ||
		    gamma[1] < MIN_GAMMA || gamma[1] > MAX_GAMMA ||
		    gamma[2] < MIN_GAMMA || gamma[2] > MAX_GAMMA) {
			fprintf(stderr,
				_("Gamma value must be between %.1f and %.1f.\n"),
				MIN_GAMMA, MAX_GAMMA);
			return -1;
		}
		state->gamma[0] = gamma[0];
		state->gamma[1] = gamma[1];
		state->gamma[2] = gamma[2];
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
w32gdi_set_temperature(w32gdi_state_t *state, int temp, float brightness)
{
	BOOL r;

	/* Open device context */
	HDC hDC = GetDC(NULL);
	if (hDC == NULL) {
		fputs(_("Unable to open device context.\n"), stderr);
		return -1;
	}

	/* Create new gamma ramps */
	colorramp_fill(state->gamma_r, state->gamma_g, state->gamma_b,
		       GAMMA_RAMP_SIZE, temp, brightness, state->gamma);

	/* Set new gamma ramps */
	r = SetDeviceGammaRamp(hDC, state->gamma_r);
	if (!r) {
		/* TODO it happens that SetDeviceGammaRamp returns FALSE on
		   occasions where the adjustment seems to be successful.
		   Does this only happen with multiple monitors connected? */
		fputs(_("Unable to set gamma ramps.\n"), stderr);
		free(state->gamma_r);
		ReleaseDC(NULL, hDC);
		return -1;
	}

	/* Release device context */
	ReleaseDC(NULL, hDC);

	return 0;
}
