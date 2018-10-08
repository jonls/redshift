#!/usr/bin/env python3

from mesonbuild.scripts import destdir_join
import compileall, sys, os

compileall.compile_dir(
    destdir_join(os.environ.get('DESTDIR', ''), sys.argv[1]))
