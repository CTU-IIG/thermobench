automark_benchmarks = [
	'pntrch', 'puwmod', 'rspeed', 'tblook', 'ttsprk', 'a2time',
	'aifirf', 'bitmnp', 'canrdr', 'idctrn', 'iirflt', 'matrix',
]

wl_idx_4K = {}
wl_idx_4M = {}
foreach b : automark_benchmarks
	wl_idx_4K += { b : 4 }
	wl_idx_4M += { b : 5 }
endforeach

wl_idx_4K += { 'idctrn' : 5 }
wl_idx_4K += { 'iirflt' : 5 }
wl_idx_4K += { 'rspeed' : 2 }
wl_idx_4K += { 'ttsprk' : 2 }

wl_idx_4M += { 'idctrn' : 6 }
wl_idx_4M += { 'iirflt' : 6 }
wl_idx_4M += { 'rspeed' : 3 }
wl_idx_4M += { 'ttsprk' : 3 }

wl_idx = { '4K' : wl_idx_4K, '4M' : wl_idx_4M }

common_flags = ['-Wall', '-Wno-long-long', '-std=gnu99', '-O2', '-fno-asm', '-fsigned-char']
platform_flags = ['-DEE_SIZEOF_LONG=8', '-DEE_SIZEOF_PTR=8', '-DEE_PTR_ALIGN=8', '-DHAVE_SYS_STAT_H=1', '-DGCC_INLINE_MACRO=1', '-DNO_RESTRICT_QUALIFIER=1']

autobench_common = static_library('autobench_common', [
					 '../mith/src/th_getopt.c',
					 '../mith/src/mith_lib.c',
					 '../mith/src/th_math.c',
					 '../mith/src/th_rand.c',
					 '../mith/src/th_bignum.c',
					 '../mith/src/md5.c',
					 '../mith/src/th_encode.c',
					 '../mith/src/mith_workload.c',
					 '../mith/src/th_lib.c',
					 '../mith/al/src/al_smp.c',
					 '../mith/al/src/al_file.c',
					 '../mith/al/src/al_single.c',
					 '../mith/al/src/th_al.c',
				  ],
				  include_directories : include_directories('../mith/include', '../mith/al/include'),
				  c_args : common_flags + platform_flags,
				 )
common_dep = declare_dependency(
	link_with : autobench_common,
	include_directories : include_directories('../mith/include', '../mith/al/include'),
)

sources = {
  'a2time' : [ '../benchmarks/automotive/a2time01/algotst_799.c', '../benchmarks/automotive/a2time01/bmark_lite_799.c' ],
  'aifftr' : [ '../benchmarks/automotive/aifftr01/algotst_798.c', '../benchmarks/automotive/aifftr01/bmark_lite_798.c' ],
  'aifirf' : [ '../benchmarks/automotive/aifirf01/algotst_797.c', '../benchmarks/automotive/aifirf01/bmark_lite_797.c' ],
  'aiifft' : [ '../benchmarks/automotive/aiifft01/algotst_796.c', '../benchmarks/automotive/aiifft01/bmark_lite_796.c' ],
  'basefp' : [ '../benchmarks/automotive/basefp01/algotst_795.c', '../benchmarks/automotive/basefp01/bmark_lite_795.c' ],
  'bitmnp' : [ '../benchmarks/automotive/bitmnp01/algotst_794.c', '../benchmarks/automotive/bitmnp01/bmark_lite_794.c' ],
  'cacheb' : [ '../benchmarks/automotive/cacheb01/algotst_793.c', '../benchmarks/automotive/cacheb01/bmark_lite_793.c' ],
  'canrdr' : [ '../benchmarks/automotive/canrdr01/algotst_792.c', '../benchmarks/automotive/canrdr01/bmark_lite_792.c' ],
  'idctrn' : [ '../benchmarks/automotive/idctrn01/algotst_791.c', '../benchmarks/automotive/idctrn01/bmark_lite_791.c' ],
  'iirflt' : [ '../benchmarks/automotive/iirflt01/algotst_790.c', '../benchmarks/automotive/iirflt01/bmark_lite_790.c' ],
  'matrix' : [ '../benchmarks/automotive/matrix01/algotst_789.c', '../benchmarks/automotive/matrix01/bmark_lite_789.c' ],
  'pntrch' : [ '../benchmarks/automotive/pntrch01/algotst_788.c', '../benchmarks/automotive/pntrch01/bmark_lite_788.c' ],
  'puwmod' : [ '../benchmarks/automotive/puwmod01/algotst_787.c', '../benchmarks/automotive/puwmod01/bmark_lite_787.c' ],
  'rspeed' : [ '../benchmarks/automotive/rspeed01/algotst_786.c', '../benchmarks/automotive/rspeed01/bmark_lite_786.c' ],
  'tblook' : [ '../benchmarks/automotive/tblook01/algotst_785.c', '../benchmarks/automotive/tblook01/bmark_lite_785.c' ],
  'ttsprk' : [ '../benchmarks/automotive/ttsprk01/algotst_784.c', '../benchmarks/automotive/ttsprk01/bmark_lite_784.c' ],
}

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
rt_dep = cc.find_library('rt', required : false)
threads_dep = dependency('threads')

run_command('sh', '-c', 'mkdir -p $MESON_BUILD_ROOT/$MESON_SUBDIR/../data')

foreach benchmark : automark_benchmarks
	foreach workload_size : [ '4K', '4M' ]
		executable(benchmark + '-' + workload_size,
			   [ 'main.c' ] + sources.get(benchmark),
			   dependencies : [ threads_dep, tbwrap_dep, common_dep, m_dep, rt_dep ],
			   c_args : common_flags + platform_flags + [
					    '-DDEFINE_PARAMS_FUN=define_params_' + benchmark,
					    '-DWL_IDX=@0@'.format(wl_idx.get(workload_size).get(benchmark)),
					    '-DINIT_FUN=bmark_init_' + benchmark,
					    '-DRT_FUN=t_run_test_' + benchmark,
					    '-DCLEAN_FUN=bmark_clean_' + benchmark,
					    '-DFINI_FUN=bmark_fini_' + benchmark,
					    '-DVERIFY_FUN=bmark_verify_' + benchmark,
					    '-DBENCHMARK_NAME="' + benchmark + '"',
					    '-DWORKLOAD_SIZE="' + workload_size + '"',
				    ],
			  )
	endforeach

	# Create a symlink to the data files - the path is hardcoded in the binaries :-(
	run_command('sh', '-c',
		    'ln -sf $MESON_SOURCE_ROOT/$MESON_SUBDIR/../benchmarks/automotive/@0@01/data_@0@ $MESON_BUILD_ROOT/$MESON_SUBDIR/../data'.format(benchmark))
endforeach
