version_h = vcs_tag(
	input : 'version.h.in',
	output : 'version.h',
	command : ['git', 'describe', '--always'],
)

cxx = meson.get_compiler('cpp')
ev_dep = cxx.find_library('ev')
deps = [ ev_dep ]
if not cxx.has_function('pthread_atfork')
	threads_dep = dependency('threads')
	deps += threads_dep
endif


executable('thermobench', [
		  'thermobench.cpp',
		  version_h,
	   ],
	   dependencies : deps)
