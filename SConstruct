
sources = ['redshift.c', 'solar.c', 'colortemp.c']

env = Environment()
env.ParseConfig('pkg-config --cflags --libs xcb xcb-randr')
env.Program('redshift', sources,
            CFLAGS='-std=gnu99 -O2 -Wall',
            LINKFLAGS='-lm')
