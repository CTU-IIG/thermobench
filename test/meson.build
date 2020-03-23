configure_file(copy : true, input : 'testlib', output : 'testlib')

foreach t: '''
0010-help.t
0020-basic.t
0022-csv-escape.t
0030-column.t
0040-exec.t
0040-time.t
0050-sensors.t
'''.split()
	test(t, find_program(t), protocol : 'tap', workdir : meson.current_build_dir())
endforeach
