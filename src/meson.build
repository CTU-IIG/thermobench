version_h = vcs_tag(
	input : 'version.h.in',
	output : 'version.h',
	command : ['git', 'describe', '--always'],
)

ev_dep = meson.get_compiler('cpp').find_library('ev')

executable('thermobench', [
		  'thermobench.cpp',
		  version_h,
	   ],
	   dependencies : [
		   ev_dep
	   ])
