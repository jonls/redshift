/* fake-quartz.h -- Fake Mac OS X library headers
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
#ifndef REDSHIFT_FAKE_QUARTZ_H
#define REDSHIFT_FAKE_QUARTZ_H


/* This header file contains some capabilities of
   <CoreGraphics/CGDirectDisplay.h> and <CoreGraphics/CGError.h>,
   and can be used modify gamma ramps without Mac OS X and Quartz
   but with its API.
   
   The content of this file is based on the documentation found on:
   
   https://developer.apple.com/library/mac/documentation/GraphicsImaging/Reference/Quartz_Services_Ref/Reference/reference.html
   
   https://developer.apple.com/library/mac/documentation/CoreGraphics/Reference/CoreGraphicsConstantsRef/Reference/reference.html#//apple_ref/c/tdef/CGError
*/


#include <stdint.h>


typedef int32_t CGError;
#define kCGErrorSuccess 0

typedef float CGGammaValue;
typedef uint32_t CGDirectDisplayID;


CGError
CGGetOnlineDisplayList(uint32_t max_size, CGDirectDisplayID *displays_out, uint32_t *count_out);

CGError
CGSetDisplayTransferByTable(CGDirectDisplayID display, uint32_t gamma_size, const CGGammaValue *red,
			    const CGGammaValue *green, const CGGammaValue *blue);

CGError
CGGetDisplayTransferByTable(CGDirectDisplayID display, uint32_t gamma_size, CGGammaValue *red,
			    CGGammaValue *green, CGGammaValue *blue, uint32_t *gamma_size_out);

void
CGDisplayRestoreColorSyncSettings(void);

uint32_t
CGDisplayGammaTableCapacity(CGDirectDisplayID display) __attribute__((const));


/* The follow part most only be used when this module is used,
   it cannot be used when the real CoreGraphics is used.
   CoreGraphics does not have this function, it is added so
   that there is a way to cleanly close the X connection
   and free resources needed by this module. */
void
close_fake_quartz(void);


#endif /* ! REDSHIFT_FAKE_QUARTZ_H */

