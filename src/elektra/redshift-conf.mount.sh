#!/bin/sh

if [ -z "$APP_PATH" ]; then
	# TODO: set APP_PATH to the installed path of your application
	APP_PATH='/usr/local/bin/sw_redshift__0_current'
fi

if ! [ -f "$APP_PATH" ]; then
	echo "ERROR: APP_PATH points to non-existent file" 1>&2
	exit 1
fi

error_other_mp() {
	echo "ERROR: another mountpoint already exists on spec:/sw/redshift/#0/current. Please umount first." 1>&2
	exit 1
}

if kdb mount -13 | grep -Fxq 'spec:/sw/redshift/#0/current'; then
	if ! kdb mount | grep -Fxq 'sw_redshift__0_current.overlay.spec.eqd on spec:/sw/redshift/#0/current with name spec:/sw/redshift/#0/current'; then
		error_other_mp
	fi

	MP=$(echo "spec:/sw/redshift/#0/current" | sed 's:\\:\\\\:g' | sed 's:/:\\/:g')
	if [ -n "$(kdb get "system:/elektra/mountpoints/$MP/getplugins/#5#specload#specload#/config/file")" ]; then
		error_other_mp
	fi
	if [ "$(kdb get "system:/elektra/mountpoints/$MP/getplugins/#5#specload#specload#/config/app")" != "$APP_PATH" ]; then
		error_other_mp
	fi
	if [ -n "$(kdb ls "system:/elektra/mountpoints/$MP/getplugins/#5#specload#specload#/config/app/args")" ]; then
		error_other_mp
	fi
else
	sudo kdb mount -R noresolver "sw_redshift__0_current.overlay.spec.eqd" "spec:/sw/redshift/#0/current" specload "app=$APP_PATH"
fi

if kdb mount -13 | grep -Fxq '/sw/redshift/#0/current'; then
	if ! kdb mount | grep -Fxq 'redshift.ecf on /sw/redshift/#0/current with name /sw/redshift/#0/current'; then
		echo "ERROR: another mountpoint already exists on /sw/redshift/#0/current. Please umount first." 1>&2
		exit 1
	fi
else
	sudo kdb spec-mount '/sw/redshift/#0/current'
fi
