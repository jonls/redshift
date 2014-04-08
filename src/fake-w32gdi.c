/* fake-w32gdi.h -- Fake Windows library headers
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fake-w32gdi.h"


/* http://msdn.microsoft.com/en-us/library/windows/desktop/dd144871(v=vs.85).aspx */
HDC GetDC(HWND hWnd)
{
	(void) hWnd;
	return (HDC*)16;
}

/* http://msdn.microsoft.com/en-us/library/windows/desktop/dd162920(v=vs.85).aspx */
int ReleaseDC(HWND hWnd, HDC hDC)
{
	(void) hWnd;
	(void) hDC;
	return 1;
}


/* http://msdn.microsoft.com/en-us/library/windows/desktop/dd144877(v=vs.85).aspx */
int GetDeviceCaps(HDC hDC, int nIndex)
{
	(void) hDC;
	return CM_GAMMA_RAMP + nIndex - COLORMGMTCAPS;
}

/* http://msdn.microsoft.com/en-us/library/windows/desktop/dd372194(v=vs.85).aspx */
BOOL SetDeviceGammaRamp(HDC hDC, LPVOID lpRamp)
{
	(void) hDC;
	(void) lpRamp;
	return TRUE;
}

/* http://msdn.microsoft.com/en-us/library/windows/desktop/dd316946(v=vs.85).aspx */
BOOL GetDeviceGammaRamp(HDC hDC, LPVOID lpRamp)
{
	(void) hDC;
	(void) lpRamp;
	return TRUE;
}


/* http://msdn.microsoft.com/en-us/library/windows/desktop/dd183490(v=vs.85).aspx */
HDC CreateDC(LPCTSTR lpszDriver, LPCTSTR lpszDevice, void *lpszOutput, void *lpInitData)
{
	(void) lpszOutput;
	(void) lpInitData;
	if (strcmp(lpszDriver, "DISPLAY"))
		return NULL;
	(void) lpszDevice;
	return (HDC*)16;
}


/* http://msdn.microsoft.com/en-us/library/windows/desktop/dd162609(v=vs.85).aspx */
BOOL EnumDisplayDevices(LPCTSTR lpDevice, DWORD iDevNum, PDISPLAY_DEVICE lpDisplayDevice, DWORD dwFlags)
{
	(void) dwFlags;
	if (lpDevice != NULL) {
		fprintf(stderr, "lpDevice (argument 1) for EnumDisplayDevices should be NULL");
		abort();
		return FALSE;
	}
	if (iDevNum >= 2)
		return FALSE;
	if (lpDisplayDevice->cb != sizeof(DISPLAY_DEVICE)) {
		fprintf(stderr,
			"lpDisplayDevice->cb for EnumDisplayDevices is not sizeof(DISPLAY_DEVICE)");
		abort();
		return FALSE;
	}
	strcmp(lpDisplayDevice->DeviceName, "some monitor");
	lpDisplayDevice->StateFlags = DISPLAY_DEVICE_ACTIVE;
	return TRUE;
}

