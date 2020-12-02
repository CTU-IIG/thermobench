cxx = meson.get_compiler('cpp')

tinyrenderer = executable(
	'tinyrenderer',
	[
		'geometry.cpp',
		'main.cpp',
		'model.cpp',
		'our_gl.cpp',
		'tgaimage.cpp',
	],
	dependencies : dependency('OpenMP', required : false),
	cpp_args : cxx.get_supported_arguments(
		[
			'-std=c++14',
			'-Wall',
			'-Wextra',
			'-Wno-non-virtual-dtor',
			'-pedantic',
			'-std=c++14',
			'-O3',
			'-fopenmp',
		]),
)
