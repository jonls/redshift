#!/usr/bin/env python3

# This script generates POTFILES for Meson from POTFILES.in for intltool

import os

print('Generating POTFILES for Meson...')
po_dir = os.path.dirname(__file__)
infile = os.path.join(po_dir, 'POTFILES.in')
outfile = os.path.join(po_dir, 'POTFILES')
with open(infile, 'r') as f_in:
    with open(outfile, 'w') as f_out:
        for l in f_in:
            if l.startswith('data/'):
                idx_last_sep = l.rindex('/')
                f_out.write('{}meson_{}'.format(
                    l[:idx_last_sep + 1], l[idx_last_sep + 1:]))
            else:
                f_out.write(l)
