
Build from repository
---------------------

``` shell
$ ./bootstrap
$ ./configure
```

The bootstrap script will use autotools to set up the build environment
and create the `configure` script.

Use `./configure --help` for options. Use `--prefix` to make an install in
your home directory. This is necessary to test python scripts. The systemd
user unit directory should be set to avoid writing to the system location.

Systemd will look for the unit files in `~/.config/systemd/user` so this
directory can be used as a target if the unit files will be used. Otherwise
the location can be set to `no` to disable the systemd files.

Example:

``` shell
$ ./configure --prefix=$HOME/redshift/root \
   --with-systemduserunitdir=$HOME/.config/systemd/user
```

Now, build the files:

``` shell
$ make
```

The main redshift program can be run at this point. To install to the
prefix directory run:

``` shell
$ make install
```

You can now run the python script. Example:

``` shell
$ $HOME/redshift/root/bin/redshift-gtk
```

Dependencies
------------

* autotools, gettext
* libdrm (Optional, for DRM support)
* libxcb, libxcb-randr (Optional, for RandR support)
* libX11, libXxf86vm (Optional, for VidMode support)
* geoclue (Optional, for geoclue support)

* python3, pygobject, pyxdg (Optional, for GUI support)
* appindicator (Optional, for Ubuntu-style GUI status icon)


Creating a new release
----------------------

1. Select a commit in master to branch from, or if making a bugfix release
   use previous release tag as base (e.g. for 1.9.1 use 1.9 as base)
2. Create release branch `release-X.Y`
3. Apply any bugfixes for release
4. Import updated translations from launchpad and commit. Remember to update
   `po/LINGUAS` if new languages were added
5. Update version in `configure.ac` and create entry in NEWS
6. Run `make distcheck`
7. Commit and tag release (`vX.Y` or `vX.Y.Z`)
8. Push tag to Github and also upload source dist file to Github

Also remember to check before release that

* Windows build is ok
* Build files for distributions are updated


Notes
-----
* verbose flag is (currently) only held in redshift.c; thus, write all
  verbose messages there.
