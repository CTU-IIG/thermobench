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

if get_option('tacle-bench').enabled()
	executable('tacle-bench_app_lift', [ 'tacle-bench/bench/app/lift/lift.c', 'tacle-bench/bench/app/lift/liftlibcontrol.c', 'tacle-bench/bench/app/lift/liftlibio.c' ], include_directories : [ include_directories('tacle-bench/bench/app/lift') ])
	executable('tacle-bench_app_powerwindow', [ 'tacle-bench/bench/app/powerwindow/powerwindow.c', 'tacle-bench/bench/app/powerwindow/powerwindow_const_params.c', 'tacle-bench/bench/app/powerwindow/powerwindow_controlexclusion.c', 'tacle-bench/bench/app/powerwindow/powerwindow_debounce.c', 'tacle-bench/bench/app/powerwindow/powerwindow_inputs.c', 'tacle-bench/bench/app/powerwindow/powerwindow_powerwindow_control.c', 'tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_DRV.c', 'tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_PSG_BackL.c', 'tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_PSG_BackR.c', 'tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_PSG_Front.c', 'tacle-bench/bench/app/powerwindow/wcclib.c' ], include_directories : [ include_directories('tacle-bench/bench/app/powerwindow') ])
	executable('tacle-bench_kernel_binarysearch', [ 'tacle-bench/bench/kernel/binarysearch/binarysearch.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/binarysearch') ])
	executable('tacle-bench_kernel_bitcount', [ 'tacle-bench/bench/kernel/bitcount/bitcnt_1.c', 'tacle-bench/bench/kernel/bitcount/bitcnt_2.c', 'tacle-bench/bench/kernel/bitcount/bitcnt_3.c', 'tacle-bench/bench/kernel/bitcount/bitcnt_4.c', 'tacle-bench/bench/kernel/bitcount/bitcount.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/bitcount') ])
	executable('tacle-bench_kernel_bitonic', [ 'tacle-bench/bench/kernel/bitonic/bitonic.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/bitonic') ])
	executable('tacle-bench_kernel_bsort', [ 'tacle-bench/bench/kernel/bsort/bsort.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/bsort') ])
	executable('tacle-bench_kernel_complex_updates', [ 'tacle-bench/bench/kernel/complex_updates/complex_updates.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/complex_updates') ])
	executable('tacle-bench_kernel_cosf', [ 'tacle-bench/bench/kernel/cosf/cosf.c', 'tacle-bench/bench/kernel/cosf/wcclibm.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/cosf') ])
	executable('tacle-bench_kernel_countnegative', [ 'tacle-bench/bench/kernel/countnegative/countnegative.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/countnegative') ])
	executable('tacle-bench_kernel_cubic', [ 'tacle-bench/bench/kernel/cubic/cubic.c', 'tacle-bench/bench/kernel/cubic/wcclibm.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/cubic') ])
	executable('tacle-bench_kernel_deg2rad', [ 'tacle-bench/bench/kernel/deg2rad/deg2rad.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/deg2rad') ])
	executable('tacle-bench_kernel_fac', [ 'tacle-bench/bench/kernel/fac/fac.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/fac') ])
	executable('tacle-bench_kernel_fft', [ 'tacle-bench/bench/kernel/fft/fft.c', 'tacle-bench/bench/kernel/fft/fft_input.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/fft') ])
	executable('tacle-bench_kernel_filterbank', [ 'tacle-bench/bench/kernel/filterbank/filterbank.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/filterbank') ])
	executable('tacle-bench_kernel_fir2dim', [ 'tacle-bench/bench/kernel/fir2dim/fir2dim.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/fir2dim') ])
	executable('tacle-bench_kernel_iir', [ 'tacle-bench/bench/kernel/iir/iir.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/iir') ])
	executable('tacle-bench_kernel_insertsort', [ 'tacle-bench/bench/kernel/insertsort/insertsort.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/insertsort') ])
	executable('tacle-bench_kernel_isqrt', [ 'tacle-bench/bench/kernel/isqrt/basicmath_libc.c', 'tacle-bench/bench/kernel/isqrt/isqrt.c', 'tacle-bench/bench/kernel/isqrt/wcclibm.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/isqrt') ])
	executable('tacle-bench_kernel_jfdctint', [ 'tacle-bench/bench/kernel/jfdctint/jfdctint.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/jfdctint') ])
	executable('tacle-bench_kernel_lms', [ 'tacle-bench/bench/kernel/lms/lms.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/lms') ])
	executable('tacle-bench_kernel_ludcmp', [ 'tacle-bench/bench/kernel/ludcmp/ludcmp.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/ludcmp') ])
	executable('tacle-bench_kernel_matrix1', [ 'tacle-bench/bench/kernel/matrix1/matrix1.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/matrix1') ])
	executable('tacle-bench_kernel_md5', [ 'tacle-bench/bench/kernel/md5/md5.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/md5') ])
	executable('tacle-bench_kernel_minver', [ 'tacle-bench/bench/kernel/minver/minver.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/minver') ])
	executable('tacle-bench_kernel_pm', [ 'tacle-bench/bench/kernel/pm/pm.c', 'tacle-bench/bench/kernel/pm/pm_input.c', 'tacle-bench/bench/kernel/pm/pm_libm.c', 'tacle-bench/bench/kernel/pm/pm_stdlib.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/pm') ])
	executable('tacle-bench_kernel_prime', [ 'tacle-bench/bench/kernel/prime/prime.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/prime') ])
	executable('tacle-bench_kernel_quicksort', [ 'tacle-bench/bench/kernel/quicksort/input.c', 'tacle-bench/bench/kernel/quicksort/quicksort.c', 'tacle-bench/bench/kernel/quicksort/quicksortlibm.c', 'tacle-bench/bench/kernel/quicksort/quicksortstdlib.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/quicksort') ])
	executable('tacle-bench_kernel_rad2deg', [ 'tacle-bench/bench/kernel/rad2deg/rad2deg.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/rad2deg') ])
	executable('tacle-bench_kernel_recursion', [ 'tacle-bench/bench/kernel/recursion/recursion.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/recursion') ])
	executable('tacle-bench_kernel_sha', [ 'tacle-bench/bench/kernel/sha/input_small.c', 'tacle-bench/bench/kernel/sha/memcpy.c', 'tacle-bench/bench/kernel/sha/memhelper.c', 'tacle-bench/bench/kernel/sha/memset.c', 'tacle-bench/bench/kernel/sha/sha.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/sha') ])
	executable('tacle-bench_kernel_st', [ 'tacle-bench/bench/kernel/st/st.c' ], include_directories : [ include_directories('tacle-bench/bench/kernel/st') ])
	# executable('tacle-bench_parallel_DEBIE_code', [ 'tacle-bench/bench/parallel/DEBIE/code/class.c', 'tacle-bench/bench/parallel/DEBIE/code/classtab.c', 'tacle-bench/bench/parallel/DEBIE/code/debie.c', 'tacle-bench/bench/parallel/DEBIE/code/health.c', 'tacle-bench/bench/parallel/DEBIE/code/hw_if.c', 'tacle-bench/bench/parallel/DEBIE/code/measure.c', 'tacle-bench/bench/parallel/DEBIE/code/tc_hand.c', 'tacle-bench/bench/parallel/DEBIE/code/telem.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code') ])
	# executable('tacle-bench_parallel_DEBIE_code_arch_arm7-lpc2138-mam', [ 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-lpc2138-mam/cpulib.c', 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-lpc2138-mam/crt_asyst.c', 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-lpc2138-mam/target.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/arch/arm7-lpc2138-mam') ])
	# executable('tacle-bench_parallel_DEBIE_code_arch_arm7-x1', [ 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x1/target.c', 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x1/wcc_memcpy.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x1') ])
	# executable('tacle-bench_parallel_DEBIE_code_arch_arm7-x2', [ 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x2/target.c', 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x2/wcc_memcpy.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x2') ])
	# executable('tacle-bench_parallel_DEBIE_code_arch_arm7-x4', [ 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x4/target.c', 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x4/wcc_memcpy.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x4') ])
	# executable('tacle-bench_parallel_DEBIE_code_arch_arm7-x8', [ 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x8/target.c', 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x8/wcc_memcpy.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/arch/arm7-x8') ])
	# executable('tacle-bench_parallel_DEBIE_code_arch_arm7', [ 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7/target.c', 'tacle-bench/bench/parallel/DEBIE/code/arch/arm7/wcc_memcpy.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/arch/arm7') ])
	# executable('tacle-bench_parallel_DEBIE_code_arch_mpc5554', [ 'tacle-bench/bench/parallel/DEBIE/code/arch/mpc5554/target.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/arch/mpc5554') ])
	# executable('tacle-bench_parallel_DEBIE_code_arch_tc1796', [ 'tacle-bench/bench/parallel/DEBIE/code/arch/tc1796/target.c', 'tacle-bench/bench/parallel/DEBIE/code/arch/tc1796/wcc_memcpy.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/arch/tc1796') ])
	# executable('tacle-bench_parallel_DEBIE_code_arch_tc1797', [ 'tacle-bench/bench/parallel/DEBIE/code/arch/tc1797/target.c', 'tacle-bench/bench/parallel/DEBIE/code/arch/tc1797/wcc_memcpy.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/arch/tc1797') ])
	# executable('tacle-bench_parallel_DEBIE_code_arch_x86', [ 'tacle-bench/bench/parallel/DEBIE/code/arch/x86/target.c', 'tacle-bench/bench/parallel/DEBIE/code/arch/x86/wcc_memcpy.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/arch/x86') ])
	# executable('tacle-bench_parallel_DEBIE_code_harness', [ 'tacle-bench/bench/parallel/DEBIE/code/harness/harness.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/DEBIE/code/harness') ])
	# executable('tacle-bench_parallel_PapaBench_sw_airborne_autopilot', [ 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/ad7714.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/adc.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/estimator.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/gps_sirf.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/gps_ubx.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/if_calib.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/infrared.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/link_fbw.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/main.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/mainloop.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/modem.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/nav.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/pid.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/spi.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot/uart.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/PapaBench/sw/airborne/autopilot') ])
	# executable('tacle-bench_parallel_PapaBench_sw_airborne_fly_by_wire', [ 'tacle-bench/bench/parallel/PapaBench/sw/airborne/fly_by_wire/adc_fbw.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/fly_by_wire/main.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/fly_by_wire/ppm.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/fly_by_wire/servo.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/fly_by_wire/spi.c', 'tacle-bench/bench/parallel/PapaBench/sw/airborne/fly_by_wire/uart.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/PapaBench/sw/airborne/fly_by_wire') ])
	# executable('tacle-bench_parallel_PapaBench_sw_lib_c', [ 'tacle-bench/bench/parallel/PapaBench/sw/lib/c/downlink.c', 'tacle-bench/bench/parallel/PapaBench/sw/lib/c/geometry.c', 'tacle-bench/bench/parallel/PapaBench/sw/lib/c/logger.c', 'tacle-bench/bench/parallel/PapaBench/sw/lib/c/math.c', 'tacle-bench/bench/parallel/PapaBench/sw/lib/c/network.c', 'tacle-bench/bench/parallel/PapaBench/sw/lib/c/protocol.c', 'tacle-bench/bench/parallel/PapaBench/sw/lib/c/traces.c', 'tacle-bench/bench/parallel/PapaBench/sw/lib/c/transport.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/PapaBench/sw/lib/c') ])
	# executable('tacle-bench_parallel_PapaBench_sw_lib_crt0', [ 'tacle-bench/bench/parallel/PapaBench/sw/lib/crt0/powerpc-elf-crt0.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/PapaBench/sw/lib/crt0') ])
	# executable('tacle-bench_parallel_rosace_thread1', [ 'tacle-bench/bench/parallel/rosace/thread1/assemblage.c', 'tacle-bench/bench/parallel/rosace/thread1/assemblage_includes.c', 'tacle-bench/bench/parallel/rosace/thread1/common.c', 'tacle-bench/bench/parallel/rosace/thread1/io.c', 'tacle-bench/bench/parallel/rosace/thread1/math_all.c', 'tacle-bench/bench/parallel/rosace/thread1/ros_th1.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/rosace/thread1') ])
	# executable('tacle-bench_parallel_rosace_thread2', [ 'tacle-bench/bench/parallel/rosace/thread2/assemblage.c', 'tacle-bench/bench/parallel/rosace/thread2/assemblage_includes.c', 'tacle-bench/bench/parallel/rosace/thread2/common.c', 'tacle-bench/bench/parallel/rosace/thread2/io.c', 'tacle-bench/bench/parallel/rosace/thread2/math_all.c', 'tacle-bench/bench/parallel/rosace/thread2/ros_th2.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/rosace/thread2') ])
	# executable('tacle-bench_parallel_rosace_thread3', [ 'tacle-bench/bench/parallel/rosace/thread3/assemblage.c', 'tacle-bench/bench/parallel/rosace/thread3/assemblage_includes.c', 'tacle-bench/bench/parallel/rosace/thread3/common.c', 'tacle-bench/bench/parallel/rosace/thread3/io.c', 'tacle-bench/bench/parallel/rosace/thread3/math_all.c', 'tacle-bench/bench/parallel/rosace/thread3/ros_th3.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/rosace/thread3') ])
	# executable('tacle-bench_parallel_rosace_thread4', [ 'tacle-bench/bench/parallel/rosace/thread4/assemblage.c', 'tacle-bench/bench/parallel/rosace/thread4/assemblage_includes.c', 'tacle-bench/bench/parallel/rosace/thread4/common.c', 'tacle-bench/bench/parallel/rosace/thread4/io.c', 'tacle-bench/bench/parallel/rosace/thread4/math_all.c', 'tacle-bench/bench/parallel/rosace/thread4/ros_th4.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/rosace/thread4') ])
	# executable('tacle-bench_parallel_rosace_thread5', [ 'tacle-bench/bench/parallel/rosace/thread5/assemblage.c', 'tacle-bench/bench/parallel/rosace/thread5/assemblage_includes.c', 'tacle-bench/bench/parallel/rosace/thread5/common.c', 'tacle-bench/bench/parallel/rosace/thread5/io.c', 'tacle-bench/bench/parallel/rosace/thread5/math_all.c', 'tacle-bench/bench/parallel/rosace/thread5/ros_th5.c' ], include_directories : [ include_directories('tacle-bench/bench/parallel/rosace/thread5') ])
	executable('tacle-bench_sequential_adpcm_dec', [ 'tacle-bench/bench/sequential/adpcm_dec/adpcm_dec.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/adpcm_dec') ])
	executable('tacle-bench_sequential_adpcm_enc', [ 'tacle-bench/bench/sequential/adpcm_enc/adpcm_enc.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/adpcm_enc') ])
	executable('tacle-bench_sequential_ammunition', [ 'tacle-bench/bench/sequential/ammunition/ammunition.c', 'tacle-bench/bench/sequential/ammunition/ammunition_libc.c', 'tacle-bench/bench/sequential/ammunition/arithm.c', 'tacle-bench/bench/sequential/ammunition/bits.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/ammunition') ])
	executable('tacle-bench_sequential_anagram', [ 'tacle-bench/bench/sequential/anagram/anagram.c', 'tacle-bench/bench/sequential/anagram/anagram_input.c', 'tacle-bench/bench/sequential/anagram/anagram_stdlib.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/anagram') ])
	executable('tacle-bench_sequential_audiobeam', [ 'tacle-bench/bench/sequential/audiobeam/audiobeam.c', 'tacle-bench/bench/sequential/audiobeam/audiobeaminput.c', 'tacle-bench/bench/sequential/audiobeam/audiobeamlibmalloc.c', 'tacle-bench/bench/sequential/audiobeam/audiobeamlibm.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/audiobeam') ])
	executable('tacle-bench_sequential_cjpeg_transupp', [ 'tacle-bench/bench/sequential/cjpeg_transupp/cjpeg_transupp.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/cjpeg_transupp') ])
	executable('tacle-bench_sequential_cjpeg_wrbmp', [ 'tacle-bench/bench/sequential/cjpeg_wrbmp/cjpeg_wrbmp.c', 'tacle-bench/bench/sequential/cjpeg_wrbmp/input.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/cjpeg_wrbmp') ])
	executable('tacle-bench_sequential_dijkstra', [ 'tacle-bench/bench/sequential/dijkstra/dijkstra.c', 'tacle-bench/bench/sequential/dijkstra/input.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/dijkstra') ])
	executable('tacle-bench_sequential_epic', [ 'tacle-bench/bench/sequential/epic/epic.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/epic') ])
	executable('tacle-bench_sequential_fmref', [ 'tacle-bench/bench/sequential/fmref/fmref.c', 'tacle-bench/bench/sequential/fmref/wcclibm.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/fmref') ])
	executable('tacle-bench_sequential_g723_enc', [ 'tacle-bench/bench/sequential/g723_enc/g723_enc.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/g723_enc') ])
	executable('tacle-bench_sequential_gsm_dec', [ 'tacle-bench/bench/sequential/gsm_dec/gsm_dec.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/gsm_dec') ])
	executable('tacle-bench_sequential_gsm_enc', [ 'tacle-bench/bench/sequential/gsm_enc/gsm_enc.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/gsm_enc') ])
	executable('tacle-bench_sequential_h264_dec', [ 'tacle-bench/bench/sequential/h264_dec/h264_dec.c', 'tacle-bench/bench/sequential/h264_dec/h264_decinput.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/h264_dec') ])
	executable('tacle-bench_sequential_huff_dec', [ 'tacle-bench/bench/sequential/huff_dec/huff_dec.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/huff_dec') ])
	executable('tacle-bench_sequential_huff_enc', [ 'tacle-bench/bench/sequential/huff_enc/huff_enc.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/huff_enc') ])
	executable('tacle-bench_sequential_mpeg2', [ 'tacle-bench/bench/sequential/mpeg2/mpeg2.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/mpeg2') ])
	executable('tacle-bench_sequential_ndes', [ 'tacle-bench/bench/sequential/ndes/ndes.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/ndes') ])
	executable('tacle-bench_sequential_petrinet', [ 'tacle-bench/bench/sequential/petrinet/petrinet.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/petrinet') ])
	executable('tacle-bench_sequential_rijndael_dec', [ 'tacle-bench/bench/sequential/rijndael_dec/aes.c', 'tacle-bench/bench/sequential/rijndael_dec/input_small_enc.c', 'tacle-bench/bench/sequential/rijndael_dec/rijndael_dec.c', 'tacle-bench/bench/sequential/rijndael_dec/rijndael_dec_libc.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/rijndael_dec') ])
	executable('tacle-bench_sequential_rijndael_enc', [ 'tacle-bench/bench/sequential/rijndael_enc/aes.c', 'tacle-bench/bench/sequential/rijndael_enc/input_small.c', 'tacle-bench/bench/sequential/rijndael_enc/rijndael_enc.c', 'tacle-bench/bench/sequential/rijndael_enc/rijndael_enc_libc.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/rijndael_enc') ])
	executable('tacle-bench_sequential_statemate', [ 'tacle-bench/bench/sequential/statemate/statemate.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/statemate') ])
	executable('tacle-bench_sequential_susan', [ 'tacle-bench/bench/sequential/susan/input.c', 'tacle-bench/bench/sequential/susan/susan.c', 'tacle-bench/bench/sequential/susan/wccfile.c', 'tacle-bench/bench/sequential/susan/wcclibm.c', 'tacle-bench/bench/sequential/susan/wccmalloc.c' ], include_directories : [ include_directories('tacle-bench/bench/sequential/susan') ])
	executable('tacle-bench_test_cover', [ 'tacle-bench/bench/test/cover/cover.c' ], include_directories : [ include_directories('tacle-bench/bench/test/cover') ])
	executable('tacle-bench_test_duff', [ 'tacle-bench/bench/test/duff/duff.c' ], include_directories : [ include_directories('tacle-bench/bench/test/duff') ])
	executable('tacle-bench_test_test3', [ 'tacle-bench/bench/test/test3/test3.c' ], include_directories : [ include_directories('tacle-bench/bench/test/test3') ])
endif
