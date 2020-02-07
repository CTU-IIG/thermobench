all: build/build.ninja
	ninja -C $(<D) 1>&2	# Redirect everything to stderr so that QtCreator sees the error messages

clean: build/build.ninja
	ninja -C $(<D) clean

build/build.ninja:
	meson $(@D)
