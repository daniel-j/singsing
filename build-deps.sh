#!/usr/bin/env bash

set -e

. ./env.sh

root=$(pwd)

SRC="$root/deps"
export SHELL=/bin/bash
export PREFIX="$root/prefix"
export PATH="$PREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$PREFIX/lib:$LD_LIBRARY_PATH"
PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"
export CC="gcc"
export CXX="g++"

$LINUX && [ "$CROSSWIN" == "false" ] && CC="$CC -U_FORTIFY_SOURCE -include $PREFIX/libcwrap.h"
$LINUX && [ "$CROSSWIN" == "false" ] && CXX="$CXX -U_FORTIFY_SOURCE -D_GLIBCXX_USE_CXX11_ABI=0 -include $PREFIX/libcwrap.h"

# multicore compilation
$MACOS && makearg="-j$(sysctl -n hw.ncpu)" || makearg="-j$(nproc)"

clean_prefix() {
	rm -rf "$PREFIX"
	mkdir -pv $PREFIX/{bin,include,lib}
	cp -f "$SRC/libcwrap.h" "$PREFIX/libcwrap.h"
}

build_soundio() {
	echo "Building SoundIO"
	cd "$SRC/soundio"
	rm -rf build
	mkdir -p build
	cd build
	rm -f CMakeCache.txt
	export LDFLAGS=-lrt
	cmake .. -DCMAKE_INSTALL_PREFIX="$PREFIX"
	make $makearg
	make install
	make clean
}

build_ftgl() {
	echo "Building Freetype GL"
	cd "$SRC/ftgl"
	mkdir -p build
	cd build
	rm -f CMakeCache.txt
	cmake .. -DCMAKE_INSTALL_PREFIX="$PREFIX" \
		-Dfreetype-gl_BUILD_APIDOC=OFF \
		-Dfreetype-gl_BUILD_DEMOS=OFF \
		-Dfreetype-gl_BUILD_TESTS=OFF
	make $makearg
	make install
	install -v makefont "$PREFIX/bin/makefont"
	make clean
}

build_yasm() {
	echo "Building Yasm"
	cd "$SRC/yasm"
	./configure --prefix="$PREFIX" PKG_CONFIG_PATH="$PKG_CONFIG_PATH" CC="$CC" CXX="$CXX" \
		--disable-static
	make $makearg
	make install
	make distclean
}

build_ffmpeg() {
	echo "Building FFmpeg"
	cd "$SRC/ffmpeg"
	./configure --prefix="$PREFIX" \
		--cc="$CC" \
		--cxx="$CXX" \
		--enable-gpl \
		--disable-static \
		--enable-shared \
		--disable-programs \
		--disable-doc \
		--disable-encoders \
		--disable-xlib \
		--disable-libxcb \
		--disable-libxcb-shm \
		--disable-libx264 \
		--disable-libx265 \
		--disable-network \
		--disable-debug \
		--disable-indevs \
		--disable-outdevs \
		--disable-postproc \
		--disable-muxers \
		--disable-bsfs \
		--disable-filters \
		--disable-protocols \
		--disable-lzma \
		--disable-bzlib
	make $makearg
	make install
	make distclean
}

build_mpv() {
	echo "Building MPV"
	cd "$SRC/mpv"
	./bootstrap.py
	PKG_CONFIG_PATH="$PKG_CONFIG_PATH" ./waf configure --prefix="$PREFIX" --disable-cplayer --enable-libmpv-shared \
		--disable-manpage-build --disable-android --disable-javascript \
		--disable-libass --disable-libass-osd --disable-libbluray \
		--disable-vapoursynth --disable-vapoursynth-lazy --disable-libarchive \
		--disable-oss-audio --disable-rsound --disable-wasapi --disable-alsa --disable-jack --disable-opensles \
		--disable-tv-v4l2 --disable-libv4l2 --disable-audio-input \
		--disable-apple-remote --disable-macos-touchbar --disable-macos-cocoa-cb \
		--disable-caca --disable-jpeg --disable-vulkan --disable-xv \
		--disable-lua # ythook
	./waf build $makearg
	./waf install
	./waf distclean
}

build_aubio() {
	echo "Building Aubio"
	cd "$SRC/aubio"
	./scripts/get_waf.sh
	PKG_CONFIG_PATH="$PKG_CONFIG_PATH" ./waf configure --prefix="$PREFIX" \
		--disable-fftw3f --disable-fftw3 --disable-avcodec --disable-jack \
		--disable-sndfile --disable-apple-audio --disable-samplerate \
		--disable-wavread --disable-wavwrite \
		--disable-docs --disable-examples
	./waf build $makearg
	./waf install
	./waf distclean
}


# START OF BUILD PROCESS

if [ "$1" == "all" ]; then
	echo "Building all dependencies"
	clean_prefix

	build_ftgl

	build_yasm
	build_ffmpeg

	build_mpv

	build_aubio


elif [ ! -z "$1" ]; then
	echo "Building dependencies $1"
	$1
fi

touch "$PREFIX/built_libs"
