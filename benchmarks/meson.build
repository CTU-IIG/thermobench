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

libtbwrap = library('tbwrap', [ 'tbwrap.c'])

if get_option('tacle-bench').enabled()
	tacle_benchmarks = {
		'app/lift': [ 'lift.c', 'liftlibcontrol.c', 'liftlibio.c' ],
		'app/powerwindow': [ 'powerwindow.c', 'powerwindow_const_params.c', 'powerwindow_controlexclusion.c', 'powerwindow_debounce.c', 'powerwindow_inputs.c', 'powerwindow_powerwindow_control.c', 'powerwindow_PW_Control_DRV.c', 'powerwindow_PW_Control_PSG_BackL.c', 'powerwindow_PW_Control_PSG_BackR.c', 'powerwindow_PW_Control_PSG_Front.c', 'wcclib.c' ],
		'kernel/binarysearch': [ 'binarysearch.c' ],
		'kernel/bitcount': [ 'bitcnt_1.c', 'bitcnt_2.c', 'bitcnt_3.c', 'bitcnt_4.c', 'bitcount.c' ],
		'kernel/bitonic': [ 'bitonic.c' ],
		'kernel/bsort': [ 'bsort.c' ],
		'kernel/complex_updates': [ 'complex_updates.c' ],
		'kernel/cosf': [ 'cosf.c', 'wcclibm.c' ],
		'kernel/countnegative': [ 'countnegative.c' ],
		'kernel/cubic': [ 'cubic.c', 'wcclibm.c' ],
		'kernel/deg2rad': [ 'deg2rad.c' ],
		'kernel/fac': [ 'fac.c' ],
		'kernel/fft': [ 'fft.c', 'fft_input.c' ],
		'kernel/filterbank': [ 'filterbank.c' ],
		'kernel/fir2dim': [ 'fir2dim.c' ],
		'kernel/iir': [ 'iir.c' ],
		'kernel/insertsort': [ 'insertsort.c' ],
		'kernel/isqrt': [ 'basicmath_libc.c', 'isqrt.c', 'wcclibm.c' ],
		'kernel/jfdctint': [ 'jfdctint.c' ],
		'kernel/lms': [ 'lms.c' ],
		'kernel/ludcmp': [ 'ludcmp.c' ],
		'kernel/matrix1': [ 'matrix1.c' ],
		'kernel/md5': [ 'md5.c' ],
		'kernel/minver': [ 'minver.c' ],
		'kernel/pm': [ 'pm.c', 'pm_input.c', 'pm_libm.c', 'pm_stdlib.c' ],
		'kernel/prime': [ 'prime.c' ],
		'kernel/quicksort': [ 'input.c', 'quicksort.c', 'quicksortlibm.c', 'quicksortstdlib.c' ],
		'kernel/rad2deg': [ 'rad2deg.c' ],
		'kernel/recursion': [ 'recursion.c' ],
		'kernel/sha': [ 'input_small.c', 'memcpy.c', 'memhelper.c', 'memset.c', 'sha.c' ],
		'kernel/st': [ 'st.c' ],

		# 'parallel/DEBIE/code': [ 'parallel/DEBIE/code/class.c', 'parallel/DEBIE/code/classtab.c', 'parallel/DEBIE/code/debie.c', 'parallel/DEBIE/code/health.c', 'parallel/DEBIE/code/hw_if.c', 'parallel/DEBIE/code/measure.c', 'parallel/DEBIE/code/tc_hand.c', 'parallel/DEBIE/code/telem.c' ],
		# 'parallel/DEBIE/code/arch/arm7-lpc2138-mam': [ 'parallel/DEBIE/code/arch/arm7-lpc2138-mam/cpulib.c', 'parallel/DEBIE/code/arch/arm7-lpc2138-mam/crt_asyst.c', 'parallel/DEBIE/code/arch/arm7-lpc2138-mam/target.c' ],
		# 'parallel/DEBIE/code/arch/arm7-x1': [ 'parallel/DEBIE/code/arch/arm7-x1/target.c', 'parallel/DEBIE/code/arch/arm7-x1/wcc_memcpy.c' ],
		# 'parallel/DEBIE/code/arch/arm7-x2': [ 'parallel/DEBIE/code/arch/arm7-x2/target.c', 'parallel/DEBIE/code/arch/arm7-x2/wcc_memcpy.c' ],
		# 'parallel/DEBIE/code/arch/arm7-x4': [ 'parallel/DEBIE/code/arch/arm7-x4/target.c', 'parallel/DEBIE/code/arch/arm7-x4/wcc_memcpy.c' ],
		# 'parallel/DEBIE/code/arch/arm7-x8': [ 'parallel/DEBIE/code/arch/arm7-x8/target.c', 'parallel/DEBIE/code/arch/arm7-x8/wcc_memcpy.c' ],
		# 'parallel/DEBIE/code/arch/arm7': [ 'parallel/DEBIE/code/arch/arm7/target.c', 'parallel/DEBIE/code/arch/arm7/wcc_memcpy.c' ],
		# 'parallel/DEBIE/code/arch/mpc5554': [ 'parallel/DEBIE/code/arch/mpc5554/target.c' ],
		# 'parallel/DEBIE/code/arch/tc1796': [ 'parallel/DEBIE/code/arch/tc1796/target.c', 'parallel/DEBIE/code/arch/tc1796/wcc_memcpy.c' ],
		# 'parallel/DEBIE/code/arch/tc1797': [ 'parallel/DEBIE/code/arch/tc1797/target.c', 'parallel/DEBIE/code/arch/tc1797/wcc_memcpy.c' ],
		# 'parallel/DEBIE/code/arch/x86': [ 'parallel/DEBIE/code/arch/x86/target.c', 'parallel/DEBIE/code/arch/x86/wcc_memcpy.c' ],
		# 'parallel/DEBIE/code/harness': [ 'parallel/DEBIE/code/harness/harness.c' ],
		# 'parallel/PapaBench/sw/airborne/autopilot': [ 'parallel/PapaBench/sw/airborne/autopilot/ad7714.c', 'parallel/PapaBench/sw/airborne/autopilot/adc.c', 'parallel/PapaBench/sw/airborne/autopilot/estimator.c', 'parallel/PapaBench/sw/airborne/autopilot/gps_sirf.c', 'parallel/PapaBench/sw/airborne/autopilot/gps_ubx.c', 'parallel/PapaBench/sw/airborne/autopilot/if_calib.c', 'parallel/PapaBench/sw/airborne/autopilot/infrared.c', 'parallel/PapaBench/sw/airborne/autopilot/link_fbw.c', 'parallel/PapaBench/sw/airborne/autopilot/main.c', 'parallel/PapaBench/sw/airborne/autopilot/mainloop.c', 'parallel/PapaBench/sw/airborne/autopilot/modem.c', 'parallel/PapaBench/sw/airborne/autopilot/nav.c', 'parallel/PapaBench/sw/airborne/autopilot/pid.c', 'parallel/PapaBench/sw/airborne/autopilot/spi.c', 'parallel/PapaBench/sw/airborne/autopilot/uart.c' ],
		# 'parallel/PapaBench/sw/airborne/fly_by_wire': [ 'parallel/PapaBench/sw/airborne/fly_by_wire/adc_fbw.c', 'parallel/PapaBench/sw/airborne/fly_by_wire/main.c', 'parallel/PapaBench/sw/airborne/fly_by_wire/ppm.c', 'parallel/PapaBench/sw/airborne/fly_by_wire/servo.c', 'parallel/PapaBench/sw/airborne/fly_by_wire/spi.c', 'parallel/PapaBench/sw/airborne/fly_by_wire/uart.c' ],
		# 'parallel/PapaBench/sw/lib/c': [ 'parallel/PapaBench/sw/lib/c/downlink.c', 'parallel/PapaBench/sw/lib/c/geometry.c', 'parallel/PapaBench/sw/lib/c/logger.c', 'parallel/PapaBench/sw/lib/c/math.c', 'parallel/PapaBench/sw/lib/c/network.c', 'parallel/PapaBench/sw/lib/c/protocol.c', 'parallel/PapaBench/sw/lib/c/traces.c', 'parallel/PapaBench/sw/lib/c/transport.c' ],
		# 'parallel/PapaBench/sw/lib/crt0': [ 'parallel/PapaBench/sw/lib/crt0/powerpc-elf-crt0.c' ],
		#  executable('tacle-bench_parallel_rosace_thread1', [ 'parallel/rosace/thread1/assemblage.c', 'parallel/rosace/thread1/assemblage_includes.c', 'parallel/rosace/thread1/common.c', 'parallel/rosace/thread1/io.c', 'parallel/rosace/thread1/math_all.c', 'parallel/rosace/thread1/ros_th1.c' ], include_directories )
		#  executable('tacle-bench_parallel_rosace_thread2', [ 'parallel/rosace/thread2/assemblage.c', 'parallel/rosace/thread2/assemblage_includes.c', 'parallel/rosace/thread2/common.c', 'parallel/rosace/thread2/io.c', 'parallel/rosace/thread2/math_all.c', 'parallel/rosace/thread2/ros_th2.c' ], include_directories : [ include_directories('parallel/rosace/thread2') ])
		#  executable('tacle-bench_parallel_rosace_thread3', [ 'parallel/rosace/thread3/assemblage.c', 'parallel/rosace/thread3/assemblage_includes.c', 'parallel/rosace/thread3/common.c', 'parallel/rosace/thread3/io.c', 'parallel/rosace/thread3/math_all.c', 'parallel/rosace/thread3/ros_th3.c' ], include_directories : [ include_directories('parallel/rosace/thread3') ])
		#  executable('tacle-bench_parallel_rosace_thread4', [ 'parallel/rosace/thread4/assemblage.c', 'parallel/rosace/thread4/assemblage_includes.c', 'parallel/rosace/thread4/common.c', 'parallel/rosace/thread4/io.c', 'parallel/rosace/thread4/math_all.c', 'parallel/rosace/thread4/ros_th4.c' ], include_directories : [ include_directories('parallel/rosace/thread4') ])
		#  executable('tacle-bench_parallel_rosace_thread5', [ 'parallel/rosace/thread5/assemblage.c', 'parallel/rosace/thread5/assemblage_includes.c', 'parallel/rosace/thread5/common.c', 'parallel/rosace/thread5/io.c', 'parallel/rosace/thread5/math_all.c', 'parallel/rosace/thread5/ros_th5.c' ], include_directories : [ include_directories('parallel/rosace/thread5') ])

		'sequential/adpcm_dec': [ 'adpcm_dec.c' ],
		'sequential/adpcm_enc': [ 'adpcm_enc.c' ],
		'sequential/ammunition': [ 'ammunition.c', 'ammunition_libc.c', 'arithm.c', 'bits.c' ],
		'sequential/anagram': [ 'anagram.c', 'anagram_input.c', 'anagram_stdlib.c' ],
		'sequential/audiobeam': [ 'audiobeam.c', 'audiobeaminput.c', 'audiobeamlibmalloc.c', 'audiobeamlibm.c' ],
		'sequential/cjpeg_transupp': [ 'cjpeg_transupp.c' ],
		'sequential/cjpeg_wrbmp': [ 'cjpeg_wrbmp.c', 'input.c' ],
		'sequential/dijkstra': [ 'dijkstra.c', 'input.c' ],
		'sequential/epic': [ 'epic.c' ],
		'sequential/fmref': [ 'fmref.c', 'wcclibm.c' ],
		'sequential/g723_enc': [ 'g723_enc.c' ],
		'sequential/gsm_dec': [ 'gsm_dec.c' ],
		'sequential/gsm_enc': [ 'gsm_enc.c' ],
		'sequential/h264_dec': [ 'h264_dec.c', 'h264_decinput.c' ],
		'sequential/huff_dec': [ 'huff_dec.c' ],
		'sequential/huff_enc': [ 'huff_enc.c' ],
		'sequential/mpeg2': [ 'mpeg2.c' ],
		'sequential/ndes': [ 'ndes.c' ],
		'sequential/petrinet': [ 'petrinet.c' ],
		'sequential/rijndael_dec': [ 'aes.c', 'input_small_enc.c', 'rijndael_dec.c', 'rijndael_dec_libc.c' ],
		'sequential/rijndael_enc': [ 'aes.c', 'input_small.c', 'rijndael_enc.c', 'rijndael_enc_libc.c' ],
		'sequential/statemate': [ 'statemate.c' ],
		'sequential/susan': [ 'input.c', 'susan.c', 'wccfile.c', 'wcclibm.c', 'wccmalloc.c' ],
		'test/cover': [ 'cover.c' ],
		'test/duff': [ 'duff.c' ],
		'test/test3': [ 'test3.c' ],
	}
	this_dir = include_directories('.')
	foreach dir, srcs : tacle_benchmarks
		sources = []
		foreach src : srcs
			sources += 'tacle-bench/bench' / dir / src
		endforeach
		executable('tacle_bench_' + dir.underscorify(), sources,
			   include_directories : [ include_directories('tacle-bench/bench' / dir), this_dir ],
			   link_with : libtbwrap,
			   c_args : [ '-Werror=implicit-function-declaration' ],
			  )
	endforeach
endif
