version_h = vcs_tag(
	input : 'version.h.in',
	output : 'version.h',
	command : ['git', 'describe', '--always'],
)

cxx = meson.get_compiler('cpp')
ev_dep = cxx.find_library('ev', required : false)
if not ev_dep.found()
	libev_proj = subproject('libev')
	ev_dep = libev_proj.get_variable('libev_dep')
	if meson.version().version_compare('>=0.52')
		# By default, gcc does not report warnings in system header. Treat
		# libev as system header to not see warnings from there.
		ev_dep = ev_dep.as_system()
	endif
endif
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
