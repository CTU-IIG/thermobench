
cl_wrap = generator(find_program('bash'),
                    output  : 'kernel_@BASENAME@.h',
                    arguments : [ '-c', '''printf 'const char* ocl_code = R"_delim_(%s)_delim_";' "$(cat '@INPUT@')" > @OUTPUT@'''],
		   )

dep_boost_program_options = dependency('boost', modules : [ 'program_options' ],
				       required : get_option('cl-bench'), disabler : true)
dep_opencl = dependency('OpenCL', required : false, disabler : true)
if not dep_opencl.found()
	dep_opencl = cxx.find_library('OpenCL', required : false, disabler : true)
endif

if get_option('cl-bench').enabled() and not dep_opencl.found()
	error('OpenCL library needed for cl-bench')
endif

cxx.has_header('CL/cl.h', required : get_option('cl-bench'))

executable('cl-mem',
	   [ 'memory.cc', 'common.cc', cl_wrap.process('memory.cl') ],
	   dependencies : [ dep_boost_program_options, dep_opencl ],
	  )

executable('cl-mandelbrot',
	   [ 'mandelbrot.cc', 'common.cc', cl_wrap.process('mandelbrot.cl') ],
	   dependencies : [ dep_boost_program_options, dep_opencl ],
	  )
