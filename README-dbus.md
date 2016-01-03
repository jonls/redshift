# DBus gamma provider for Redshift #

This provider emits colour temperature values onto DBus.

This can then be used to export the colour ramping features of Redshift into another application.  An example application of this would be a program to change the colour temperature of network-controllable lighting.

## Building Redshift to support DBus

Redshift will require that you install the DBus headers.  This is normally in a development package in your package manager, named something like `libdbus-1-dev`.

In order to build Redshift with DBus support, run the following:

```
./bootstrap
./configure --enable-dbus
make -j2
make install
```

### Running without Xorg / X11

You may wish to supply additional arguments to `./configure` in order to disable other gamma providers, if you are running Redshift on a system without Xorg:

```
./configure --enable-dbus --disable-drm --disable-randr --disable-vidmode --disable-quartz --disable-wingdi --disable-gui --disable-ubuntu
```

Note: you may execute with `--disable-ubuntu`, even if you are installing on Ubuntu Server.  This installs extra icons which are only relevant to systems with Xorg.

### Configuring DBus to permit a user to broadcast on the System bus

Normally, DBus will prohibit users from broadcasting to the System bus.  You can add a file like `/etc/dbus-1/system.d/redshift.conf` to change the permissions:

```xml
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
	<policy user="redshift"> <!-- This is the user redshift runs as. -->
		<allow own="dk.jonls.redshift.dbus"/>
		<allow send_destination="dk.jonls.redshift.dbus" send_interface="dk.jonls.Redshift"/>
		<allow send_destination="dk.jonls.redshift.dbus" send_interface="org.freedesktop.DBus.Introspectable"/>
	</policy>
</busconfig>
```

## Running Redshift to support DBus

If you're running headless, you probably don't have extra desktop geolocation providers like Geoclue avaliable.  You'll need to manually enter your location, in latitude and longitude, as a parameter to Redshift.

You can choose whether you want to run on the System or the Session bus.  You'll need to write some extra configuration for your DBus daemon in order to permit this.

For example, if you live in Sydney, Australia, and want to emit messages on the System bus, you would run Redshift with the following command:

```
redshift -m dbus -l -33.867716:151.207699
```

If you want to instead emit these messages on the Session bus (useful for local testing):

```
redshift -m dbus:session=1 -l -33.867716:151.207699
```

## DBus signals format

Redshift will emit messages from `dk.jonls.redshift.dbus` to `/dk/jonls/Redshift`, with the `dk.jonls.Redshift` interface.

You can test this with `mdbus2` with the following:

```
mdbus2 -s -l dk.jonls.redshift.dbus
```

Omit the `-s` option to work with the Session bus rather than the System bus.

The signals will be of type `dk.jonls.Redshift.Temperature`, which takes a single int16 argument, the desired colour temperature.

