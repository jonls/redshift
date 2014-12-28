/* location-corelocation.m -- CoreLocation (OSX) location provider source
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

   Copyright (c) 2014  Jon Lund Steffensen <jonlst@gmail.com>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#include "location-corelocation.h"

#include <stdio.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif


@interface Delegate : NSObject <CLLocationManagerDelegate>
@property (strong, nonatomic) CLLocationManager *locationManager;
@property (nonatomic) BOOL success;
@property (nonatomic) float latitude;
@property (nonatomic) float longitude;
@end

@implementation Delegate;

- (void)start
{
	self.locationManager = [[CLLocationManager alloc] init];
	self.locationManager.delegate = self;

	CLAuthorizationStatus authStatus =
		[CLLocationManager authorizationStatus];

	if (authStatus != kCLAuthorizationStatusNotDetermined &&
	    authStatus != kCLAuthorizationStatusAuthorized) {
		fputs(_("Not authorized to obtain location"
			" from CoreLocation.\n"), stderr);
		CFRunLoopStop(CFRunLoopGetCurrent());
	}

	[self.locationManager startUpdatingLocation];
}

- (void)stop
{
	[self.locationManager stopUpdatingLocation];
	CFRunLoopStop(CFRunLoopGetCurrent());
}

- (void)locationManager:(CLLocationManager *)manager
     didUpdateLocations:(NSArray *)locations
{
	CLLocation *newLocation = [locations firstObject];
	self.latitude = newLocation.coordinate.latitude;
	self.longitude = newLocation.coordinate.longitude;
	self.success = YES;

	[self stop];
}

- (void)locationManager:(CLLocationManager *)manager
       didFailWithError:(NSError *)error
{
	fprintf(stderr, _("Error obtaining location from CoreLocation: %s\n"),
	       [[error localizedDescription] UTF8String]);
	[self stop];
}

- (void)locationManager:(CLLocationManager *)manager
       didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
	if (status == kCLAuthorizationStatusNotDetermined) {
		fputs(_("Waiting for authorization to obtain location...\n"),
		      stderr);
	} else if (status != kCLAuthorizationStatusAuthorized) {
		fputs(_("Request for location was not authorized!\n"),
		      stderr);
		[self stop];
	}
}

@end


int
location_corelocation_init(void *state)
{
	return 0;
}

int
location_corelocation_start(void *state)
{
	return 0;
}

void
location_corelocation_free(void *state)
{
}

void
location_corelocation_print_help(FILE *f)
{
	fputs(_("Use the location as discovered by the Corelocation provider.\n"), f);
	fputs("\n", f);

	fprintf(f, _("NOTE: currently Redshift doesn't recheck %s once started,\n"
		     "which means it has to be restarted to take notice after travel.\n"),
		"CoreLocation");
	fputs("\n", f);
}

int
location_corelocation_set_option(void *state,
				 const char *key, const char *value)
{
	fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
	return -1;
}

int
location_corelocation_get_location(void *state,
				   float *lat, float *lon)
{
	int result = -1;

	@autoreleasepool {
		Delegate *delegate = [[Delegate alloc] init];
		[delegate performSelectorOnMainThread:@selector(start) withObject:nil waitUntilDone:NO];
		CFRunLoopRun();

		if (delegate.success) {
			*lat = delegate.latitude;
			*lon = delegate.longitude;
			result = 0;
		}
	}

	return result;
}
