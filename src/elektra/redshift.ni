; Elektra specification file for redshift@490ba2aae9cfee097a88b6e2be98aeb1ce990050 (= redshift v1.12 + some small commits)
;
; A specification file describes:
; 1. which configuration settings an application supports,
; 2. their defaults and
; 3. their associated CLI options.
; It also "(...) defines how the configuration settings should be validated"
; (source: https://www.libelektra.org/tutorials/validate-configuration)
;
; For details on specification files see https://www.libelektra.org/tutorials/writing-a-specification-for-your-configuration)

; Redshift uses no environment variables.
; Note: When renaming enum values, make sure to also update any code that uses them.
; (e.g. the string values of enum [provider/location] are used in options.c).

[]
mountpoint = redshift.ecf

[mode]
type = enum
check/enum = #4
check/enum/#0 = continual
check/enum/#1 = print
check/enum/#2 = oneshot
check/enum/#3 = reset
check/enum/#4 = oneshotmanual
description = The program mode. "continual" will constantly adjust the screen color temperature using the configured "provider". "print" will just print parameters and exit. "oneshot" will set temperature once using the configured "provider". "reset" will remove any color temperature adjustments then exit. "oneshotmanual" will not use any provider to determine if it's night or day and set the temperature specified by option "temp/oneshotmanual" immediately.
default = continual
example = oneshot
opt/long = mode
opt/arg = required

[verbose]
type = boolean
description = Verbose output.
default = 0
example = 1
opt = v
opt/arg = none

[version]
type = boolean
description = Show program version.
default = 0
example = 1
opt = V
opt/arg = none

; HL API by default only prints help, when long CLI option "--help" is given. For redshift we also want "-h" to work.
[help]
type = boolean
description = Show program help.
default = 0
example = 1
opt = h
opt/arg = none

[temp/day]
type = unsigned_short
description = The color temperature the screen should have during daytime.
default = 6500
example = 6500
check/type = unsigned_short
check/range = 1000-25000
opt/long = temp-day
opt/arg = required

[temp/night]
type = unsigned_short
description = The color temperature the screen should have during nighttime.
default = 4500
example = 4500
check/type = unsigned_short
check/range = 1000-25000
opt/long = temp-night
opt/arg = required

[temp/oneshotmanual]
type = unsigned_short
description = The color temperature the screen should have when oneshotmanual mode is used.
default = 6500
example = 6500
check/type = unsigned_short
check/range = 1000-25000
opt/long = temp-oneshotmanual
opt/arg = required

[fastfade]
type = boolean
description = Enable fast fades between color temperatures (e.g. from daytime to nighttime). When disabled, fades will be slow and more pleasant.
default = 0
example = 1
opt = f
opt/long = fastfade
opt/arg = none

[brightness/day]
type = float
description = The screen brightness during daytime. If both day and night brightness are set, these will overrule the value of brightness.
default = 1.0
example = 0.8
check/type = float
check/range = 0-1
opt/long = brightness-day
opt/arg = required

[brightness/night]
type = float
description = The screen brightness during nighttime. If both day and night brightness are set, these will overrule the value of brightness.
default = 1.0
example = 0.8
check/type = float
check/range = 0-1
opt/long = brightness-night
opt/arg = required

[gamma/day]
type = string
description = The screen gamma during daytime. Supported formats: 1. One value, that will be used for red, green and blue (e.g. 0.9). 2. Three colon-separated values for red, green and blue respectively (e.g. 0.9:0.9:0.9).
default = 1.0:1.0:1.0
example = 0.9
; Regex ensures format "float" or "float:float:float". Adapted from https://www.regular-expressions.info/floatingpoint.html
check/validation = ^([0-9]*[\.,]?[0-9]+)(:([0-9]*[\.,]?[0-9]+):([0-9]*[\.,]?[0-9]+))?$
check/validation/message = The gamma value you provided is in an unsupported format. Supported formats are: 1. One value, that will be used for red, green and blue (e.g. 0.9). 2. Three colon-separated values for red, green and blue respectively (e.g. 0.9:0.9:0.9).
opt/long = gamma-day
opt/arg = required

[gamma/night]
type = string
description = The screen gamma during nighttime. Supported formats: 1. One value, that will be used for red, green and blue (e.g. 0.9). 2. Three colon-separated values for red, green and blue respectively (e.g. 0.9:0.9:0.9).
default = 1.0:1.0:1.0
example = 0.9
; Regex ensures format "float" or "float:float:float". Adapted from https://www.regular-expressions.info/floatingpoint.html
check/validation = ^([0-9]*[\.,]?[0-9]+)(:([0-9]*[\.,]?[0-9]+):([0-9]*[\.,]?[0-9]+))?$
check/validation/message = The gamma value you provided is in an unsupported format. Supported formats are: 1. One value, that will be used for red, green and blue (e.g. 0.9). 2. Three colon-separated values for red, green and blue respectively (e.g. 0.9:0.9:0.9).
opt/long = gamma-night
opt/arg = required

[gamma/preserve]
type = boolean
description = Use to preserve existing gamma ramps before applying adjustments.
default = 1
example = 0
opt = P
opt/arg = none

[adjustment/method]
type = enum
check/enum = #7
check/enum/#0 = drm
check/enum/#1 = dummy
check/enum/#2 = quartz
check/enum/#3 = randr
check/enum/#4 = vidmode
check/enum/#5 = w32gdi
check/enum/#6 = auto
check/enum/#7 = list
description = The method used to adjust screen color temperature. By default, one of the supported methods on the current OS will be chosen automatically. For details see section "Alternative Features" in file DESIGN in root directory.
default = auto
example = randr
; default = w32gdi - For Windows the default is the only supported method 'w32gdi'
; default = randr - For Linux the default is 'randr'
; default = quartz - For macOS the default is 'quartz'
opt = m
opt/long = method
opt/arg = required

[adjustment/method/help]
type = boolean
description = Prints the help of the adjustment methods.
default = 0
example = 1
opt/long = help-methods
opt/arg = none

[provider]
type = enum
check/enum = #1
check/enum/#0 = time
check/enum/#1 = location
description = The provider used to decide at what times of day redshift should be enabled/disabled. Currently two options are supported: 1. location - determines the user's location and enable/disable redshift depending on the solar elevation. 2. time: Ignore user location and enable redshift if time is between dusk and dawn.
default = location
example = time
opt/long = provider
opt/arg = required

[provider/time/dawn/start]
type = string
check/date = ISO8601
check/date/format = timeofday
description = Instead of using the solar elevation at the user's location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).
default = 05:00
example = 06:00
opt/long = time-dawn-start
opt/arg = required

[provider/time/dawn/end]
type = string
check/date = ISO8601
check/date/format = timeofday
description = Instead of using the solar elevation at the user's location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).
default = 06:30
example = 07:30
opt/long = time-dawn-end
opt/arg = required

[provider/time/dusk/start]
type = string
check/date = ISO8601
check/date/format = timeofday
description = Instead of using the solar elevation at the user's location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).
default = 19:00
example = 20:00
opt/long = time-dusk-start
opt/arg = required

[provider/time/dusk/end]
type = string
check/date = ISO8601
check/date/format = timeofday
description = Instead of using the solar elevation at the user's location, the time intervals of dawn and dusk can be specified manually (source: https://github.com/jonls/redshift/wiki/Configuration-file#custom-dawndusk-intervals).
default = 20:30
example = 21:30
opt/long = time-dusk-end
opt/arg = required

[provider/location]
type = enum
check/enum = #4
check/enum/#0 = corelocation
check/enum/#1 = geoclue2
check/enum/#2 = manual
check/enum/#3 = auto
check/enum/#4 = list
description = The location provider to be used. By default, one of the supported location providers on the current OS will be chosen automatically. The provider is used to establish whether it is currently daytime or nighttime.
default = auto
example = geoclue2
opt/long = location-provider
opt/arg = required

[provider/location/help]
type = boolean
description = Prints the help of the location providers.
default = 0
example = 1
opt/long = help-providers
opt/arg = none

[provider/location/elevation/high]
type = float
description = By default, Redshift will use the current elevation of the sun to determine whether it is daytime, night or in transition (dawn/dusk). When the sun is above the degrees specified with the elevation-high key it is considered daytime and below the elevation-low key it is considered night (source: https://github.com/jonls/redshift/wiki/Configuration-file#solar-elevation-thresholds). Affects all location providers.
default = 3.0
example = 3.5
opt/long = elevation-high
opt/arg = required

[provider/location/elevation/low]
type = float
description = By default, Redshift will use the current elevation of the sun to determine whether it is daytime, night or in transition (dawn/dusk). When the sun is above the degrees specified with the elevation-high key it is considered daytime and below the elevation-low key it is considered night (source: https://github.com/jonls/redshift/wiki/Configuration-file#solar-elevation-thresholds). Affects all location providers.
default = -6.0
example = -5.0
opt/long = elevation-low
opt/arg = required

[provider/location/manual/lat]
type = float
description = The location latitude. Only applies to location provider "manual". Some locations (e.g. mainland USA) require negative values.
check/type = float
check/range = -90.0-90.0
; Latitude of Berlin:
default = 52.520008
example = 52.520008
opt/long = lat
opt/arg = required

[provider/location/manual/lon]
type = float
description = The location longitude. Only applies to location provider "manual". Some locations (e.g. parts of Africa) require negative values.
check/type = float
check/range = -180.0-180.0
; Longitude of berlin:
default = 13.404954
example = 13.404954
opt/long = lon
opt/arg = required

; Note: other location providers have no config parameters/cli arguments

[adjustment/crtc]
type = unsigned_short
description = CRTC to apply adjustments to.
default = 0
example = 1
opt/long = crtc
opt/arg = required

[adjustment/screen]
type = unsigned_short
description = X screen to apply adjustments to.
default = 0
example = 1
opt/long = screen
opt/arg = required

[adjustment/drm/card]
type = unsigned_short
description = Graphics card to apply adjustments to.
default = 0
example = 1
opt/long = drm-card
opt/arg = required

; Note: adjustment methods dummy, quartz and w32gdi have no config parameters/cli arguments or none that have an effect.
