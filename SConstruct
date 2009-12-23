
sources = ['src/redshift.c',
           'src/solar.c',
           'src/colorramp.c',
           'src/randr.c',
           'src/vidmode.c']

env = Environment()
env.ParseConfig('pkg-config --cflags --libs xcb xcb-randr')
env.ParseConfig('pkg-config --cflags --libs x11 xxf86vm')
env.Program('redshift', sources,
            CFLAGS='-std=gnu99 -O2 -Wall',
            LINKFLAGS='-lm')
