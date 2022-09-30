project('simple', 'c')
cc = meson.get_compiler('c')
glib_dep = dependency('glib-2.0')
jansson_dep = dependency('jansson')
math_dep = cc.find_library('m')

executable('testapp', 'main.c', 'config.c',
                     dependencies: [ glib_dep, jansson_dep, math_dep])
