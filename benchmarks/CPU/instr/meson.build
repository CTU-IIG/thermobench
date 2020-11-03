benchmarks = '''
        read
'''.split()

if host_machine.cpu_family() == 'aarch64'
	benchmarks += '''
	 	alu_fp32_add
	 	alu_fp32_div
	 	alu_fp32_madd
	 	alu_fp32_mul
	 	alu_fp32_mul_add
	 	alu_fp64_add
	 	alu_fp64_div
	 	alu_fp64_madd
	 	alu_fp64_mul
	 	alu_fp64_mul_add
	 	alu_int32_add
	 	alu_int32_changing_data_change
	 	alu_int32_changing_data_ref
	 	alu_int32_div
	 	alu_int32_madd
	 	alu_int32_mul
	 	alu_int64_add
	 	alu_int64_changing_data_change
	 	alu_int64_changing_data_ref
	 	alu_int64_div
	 	alu_int64_madd
	 	alu_int64_mul
	 	simd_fp32_add
	 	simd_fp32_div
	 	simd_fp32_madd
	 	simd_fp32_mul
	 	simd_fp32_mul_add
	 	simd_fp64_add
	 	simd_fp64_div
	 	simd_fp64_madd
	 	simd_fp64_mul
	 	simd_fp64_mul_add
	 	simd_int16_add
	 	simd_int16_madd
	 	simd_int16_mul
	 	simd_int32_add
	 	simd_int32_changing_data_change
	 	simd_int32_changing_data_ref
	 	simd_int32_madd
	 	simd_int32_mul
	 	simd_int64_add
	 	simd_int64_changing_data_change
	 	simd_int64_changing_data_ref
	 	simd_int8_add
	 	simd_int8_madd
	 	simd_int8_mul
	'''.split()
endif

rt_dep = declare_dependency(link_args : '-lrt')
threads_dep = dependency('threads')

# find demos userspace library
demos_dep = meson.get_compiler('c').find_library(
		'demos-sch',
		required: false,
		static: true,
		has_headers: ['demos-sch.h'])

if demos_dep.found()
	# copy DEmOS config file to build directory when DEmOS support is enabled
	configure_file(
			copy: true,
			input: 'read_demos_config.yaml',
			output: 'read_demos_config.yaml')
endif

foreach b : benchmarks
	executable(b, ['main.c'],
		   c_args : ['-DBENCH_H="@0@.h"'.format(b)] + (demos_dep.found() ? ['-DWITH_DEMOS'] : []),
		   dependencies : [threads_dep, rt_dep, demos_dep])
endforeach
