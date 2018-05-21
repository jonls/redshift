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

   Copyright (c) 2014-2017  Jon Lund Steffensen <jonlst@gmail.com>
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

#include "location-corelocation.h"
#include "pipeutils.h"
#include "redshift.h"

#include <stdio.h>
#include <unistd.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif


typedef struct {
  NSThread *thread;
  NSLock *lock;
  int pipe_fd_read;
  int pipe_fd_write;
  int available;
  int error;
  float latitude;
  float longitude;
} location_corelocation_state_t;


@interface LocationDelegate : NSObject <CLLocationManagerDelegate>
@property (strong, nonatomic) CLLocationManager *locationManager;
@property (nonatomic) location_corelocation_state_t *state;
@end

@implementation LocationDelegate;

- (void)start
{
  self.locationManager = [[CLLocationManager alloc] init];
  self.locationManager.delegate = self;
  self.locationManager.distanceFilter = 50000;
  self.locationManager.desiredAccuracy = kCLLocationAccuracyKilometer;

  CLAuthorizationStatus authStatus =
    [CLLocationManager authorizationStatus];

  if (authStatus != kCLAuthorizationStatusNotDetermined &&
      authStatus != kCLAuthorizationStatusAuthorized) {
    fputs(_("Not authorized to obtain location"
            " from CoreLocation.\n"), stderr);
    [self markError];
  } else {
    [self.locationManager startUpdatingLocation];
  }
}

- (void)markError
{
  [self.state->lock lock];

  self.state->error = 1;

  [self.state->lock unlock];

  pipeutils_signal(self.state->pipe_fd_write);
}

- (void)markUnavailable
{
  [self.state->lock lock];

  self.state->available = 0;

  [self.state->lock unlock];

  pipeutils_signal(self.state->pipe_fd_write);
}

- (void)locationManager:(CLLocationManager *)manager
    didUpdateLocations:(NSArray *)locations
{
  CLLocation *newLocation = [locations firstObject];

  [self.state->lock lock];

  self.state->latitude = newLocation.coordinate.latitude;
  self.state->longitude = newLocation.coordinate.longitude;
  self.state->available = 1;

  [self.state->lock unlock];

  pipeutils_signal(self.state->pipe_fd_write);
}

- (void)locationManager:(CLLocationManager *)manager
    didFailWithError:(NSError *)error
{
  fprintf(stderr, _("Error obtaining location from CoreLocation: %s\n"),
         [[error localizedDescription] UTF8String]);
  if ([error code] == kCLErrorDenied) {
    [self markError];
  } else {
    [self markUnavailable];
  }
}

- (void)locationManager:(CLLocationManager *)manager
    didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
  if (status == kCLAuthorizationStatusNotDetermined) {
    fputs(_("Waiting for authorization to obtain location...\n"), stderr);
  } else if (status != kCLAuthorizationStatusAuthorized) {
    fputs(_("Request for location was not authorized!\n"), stderr);
    [self markError];
  }
}

@end


// Callback when the pipe is closed.
//
// Stops the run loop causing the thread to end.
static void
pipe_close_callback(
    CFFileDescriptorRef fdref, CFOptionFlags callBackTypes, void *info)
{
  CFFileDescriptorInvalidate(fdref);
  CFRelease(fdref);

  CFRunLoopStop(CFRunLoopGetCurrent());
}


@interface LocationThread : NSThread
@property (nonatomic) location_corelocation_state_t *state;
@end

@implementation LocationThread;

// Run loop for location provider thread.
- (void)main
{
  @autoreleasepool {
    LocationDelegate *locationDelegate = [[LocationDelegate alloc] init];
    locationDelegate.state = self.state;

    // Start the location delegate on the run loop in this thread.
    [locationDelegate performSelector:@selector(start)
      withObject:nil afterDelay:0];

    // Create a callback that is triggered when the pipe is closed. This will
    // trigger the main loop to quit and the thread to stop.
    CFFileDescriptorRef fdref = CFFileDescriptorCreate(
      kCFAllocatorDefault, self.state->pipe_fd_write, false,
      pipe_close_callback, NULL);
    CFFileDescriptorEnableCallBacks(fdref, kCFFileDescriptorReadCallBack);
    CFRunLoopSourceRef source = CFFileDescriptorCreateRunLoopSource(
      kCFAllocatorDefault, fdref, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);

    // Run the loop
    CFRunLoopRun();

    close(self.state->pipe_fd_write);
  }
}

@end


static int
location_corelocation_init(location_corelocation_state_t **state)
{
  *state = malloc(sizeof(location_corelocation_state_t));
  if (*state == NULL) return -1;
  return 0;
}

static int
location_corelocation_start(location_corelocation_state_t *state)
{
  state->pipe_fd_read = -1;
  state->pipe_fd_write = -1;

  state->available = 0;
  state->error = 0;
  state->latitude = 0;
  state->longitude = 0;

  int pipefds[2];
  int r = pipeutils_create_nonblocking(pipefds);
  if (r < 0) {
    fputs(_("Failed to start CoreLocation provider!\n"), stderr);
    return -1;
  }

  state->pipe_fd_read = pipefds[0];
  state->pipe_fd_write = pipefds[1];

  pipeutils_signal(state->pipe_fd_write);

  state->lock = [[NSLock alloc] init];

  LocationThread *thread = [[LocationThread alloc] init];
  thread.state = state;
  [thread start];
  state->thread = thread;

  return 0;
}

static void
location_corelocation_free(location_corelocation_state_t *state)
{
  if (state->pipe_fd_read != -1) {
    close(state->pipe_fd_read);
  }

  free(state);
}

static void
location_corelocation_print_help(FILE *f)
{
  fputs(_("Use the location as discovered by the Corelocation provider.\n"), f);
  fputs("\n", f);
}

static int
location_corelocation_set_option(
    location_corelocation_state_t *state, const char *key, const char *value)
{
  fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
  return -1;
}

static int
location_corelocation_get_fd(location_corelocation_state_t *state)
{
  return state->pipe_fd_read;
}

static int
location_corelocation_handle(
    location_corelocation_state_t *state,
    location_t *location, int *available)
{
  pipeutils_handle_signal(state->pipe_fd_read);

  [state->lock lock];

  int error = state->error;
  location->lat = state->latitude;
  location->lon = state->longitude;
  *available = state->available;

  [state->lock unlock];

  if (error) return -1;

  return 0;
}


const location_provider_t corelocation_location_provider = {
  "corelocation",
  (location_provider_init_func *)location_corelocation_init,
  (location_provider_start_func *)location_corelocation_start,
  (location_provider_free_func *)location_corelocation_free,
  (location_provider_print_help_func *)location_corelocation_print_help,
  (location_provider_set_option_func *)location_corelocation_set_option,
  (location_provider_get_fd_func *)location_corelocation_get_fd,
  (location_provider_handle_func *)location_corelocation_handle
};
