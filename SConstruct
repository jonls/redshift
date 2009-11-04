
sources = ['redshift.c', 'solar.c', 'colortemp.c']

env = Environment()
env.ParseConfig('pkg-config --cflags --libs xcb xcb-randr')
env.Program('redshift', sources,
            CFLAGS='-std=c99 -D_BSD_SOURCE',
            LINKFLAGS='-lm')
