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

foreach b : benchmarks
	executable(b, ['main.c'],
		   c_args : '-DBENCH_H="@0@.h"'.format(b),
		   dependencies : [ threads_dep, rt_dep ])
endforeach
