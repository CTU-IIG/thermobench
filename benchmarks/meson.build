subdir('cl-bench/src')
subdir('CPU')
subdir('mem')
subdir('sched')

fs = import('fs')

if fs.exists('tinyrenderer/meson.build')
	subdir('tinyrenderer')
endif

# CoreMark

coremark_srcs = []

foreach s : ''' core_list_join.c
		core_main.c
		core_matrix.c
		core_state.c
		core_util.c
		linux64/core_portme.c
		'''.split()
	coremark_srcs += join_paths('coremark', s)
endforeach

if run_command('test', '-f', coremark_srcs[0]).returncode() == 0
	coremark_deps = [
		dependency('threads'),
	]

	foreach mt : [ 1, 8 ]
		foreach it : [ 0, 1024*1024*1024 ]
			executable('coremark@0@@1@'.format(mt == 1 ? '' : '-@0@thr'.format(mt),
							   it == 0 ? '' : '-long'),
				   coremark_srcs,
				   c_args : [ '-DFLAGS_STR="N/A"',
					      '-DMULTITHREAD=@0@'.format(mt),
					      '-DUSE_PTHREAD',
					      '-DITERATIONS=@0@'.format(it),
					      #'-DPRINT_ARGS',
					    ],
				   include_directories : [
					   include_directories('coremark'),
					   include_directories('coremark/linux64'),
				   ],
				   dependencies : coremark_deps,
				  )
		endforeach
	endforeach
endif
