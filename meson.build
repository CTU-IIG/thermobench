cxx = meson.get_compiler('cpp')

dep_scanner = dependency('wayland-scanner', native: true, disabler: true, required: false)
prog_scanner = find_program(dep_scanner.get_pkgconfig_variable('wayland_scanner'))

dep_wp = dependency('wayland-protocols', version: '>= 1.18', disabler: true, required: false)
dir_wp_base = dep_wp.get_pkgconfig_variable('pkgdatadir')

generated_protocols = [
	[ 'fullscreen-shell', 'v1' ],
	[ 'xdg-shell', 'v6' ],
	[ 'xdg-shell', 'stable' ],
]

foreach proto: generated_protocols
	proto_name = proto[0]
	if proto[1] == 'internal'
		base_file = proto_name
		xml_path = '@0@.xml'.format(proto_name)
	elif proto[1] == 'stable'
		base_file = proto_name
		xml_path = '@0@/stable/@1@/@1@.xml'.format(dir_wp_base, base_file)
	else
		base_file = '@0@-unstable-@1@'.format(proto_name, proto[1])
		xml_path = '@0@/unstable/@1@/@2@.xml'.format(dir_wp_base, proto_name, base_file)
	endif

	foreach output_type: [
                'client-header',
                'private-code'
        ]
		if output_type == 'client-header'
			output_file = '@0@-client-protocol.h'.format(base_file)
		elif output_type == 'server-header'
			output_file = '@0@-server-protocol.h'.format(base_file)
		else
			output_file = '@0@-protocol.c'.format(base_file)
			if dep_scanner.version().version_compare('< 1.14.91')
				output_type = 'code'
			endif
		endif

		var_name = output_file.underscorify()
		target = custom_target(
			'@0@ @1@'.format(base_file, output_type),
			command: [ prog_scanner, output_type, '@INPUT@', '@OUTPUT@' ],
			input: xml_path,
			output: output_file,
		)

		set_variable(var_name, target)
	endforeach
endforeach

foreach wayland: [ false, true ]
        tinyrenderer = executable(
	        'tinyrenderer' + (wayland ? '-wayland' : ''),
	        [
		        'geometry.cpp',
		        'main.cpp',
		        'model.cpp',
		        'our_gl.cpp',
		        'tgaimage.cpp'
                ] + (wayland ?
                     [
                             'wayland/shm.c',
                             'wayland/os-compatibility.c',
		             xdg_shell_client_protocol_h,
		             xdg_shell_protocol_c,
		             fullscreen_shell_unstable_v1_client_protocol_h,
		             fullscreen_shell_unstable_v1_protocol_c,
	             ] : []),
	        dependencies : [
                        dependency('OpenMP', required : false),
                ] + (wayland ? [ dependency('wayland-client', version: '>= 1.17.0', disabler: true, required: false) ] : []),
	        link_with : libtbwrap,
	        cpp_args : cxx.get_supported_arguments(
		        [
			        '-std=c++14',
			        '-Wall',
			        '-Wextra',
			        '-Wno-non-virtual-dtor',
			        '-pedantic',
			        '-std=c++14',
			        '-O3',
			        '-fopenmp',
		        ] + (wayland ? ['-DWITH_WAYLAND'] : [])),
        )
endforeach
