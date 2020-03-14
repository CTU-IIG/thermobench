all debug: build/build.ninja
	ninja -C $(<D) 1>&2	# Redirect everything to stderr so that QtCreator sees the error messages

clean test: build/build.ninja
	ninja -C $(<D) $@

%/build.ninja:
	meson $(@D) $(MESON_OPTS)

aarch64: MESON_OPTS=--cross-file aarch64.txt --default-library=static
aarch64: build-aarch64/build.ninja
	ninja -C $(<D)

debug: MESON_OPTS=--buildtype=debug -Db_sanitize=address,undefined --optimization=g
