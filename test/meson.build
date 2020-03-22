configure_file(copy : true, input : 'testlib', output : 'testlib')

foreach t: '''
	test
'''.split()
	test(t, find_program(t), protocol : 'tap', workdir : meson.current_build_dir())
endforeach
