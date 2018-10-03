#!/usr/bin/env bash

set -e

. ./env.sh

root=$(pwd)

SRC="$root/deps"
export SHELL=/bin/bash
export PREFIX="$root/prefix"
$CROSSWIN && export PREFIX="$root/prefixwin"
export PATH="$PREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$PREFIX/lib:$LD_LIBRARY_PATH"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"
export CC="gcc"
export CXX="g++"

# $LINUX && [ "$CROSSWIN" == "false" ] && CC="$CC -U_FORTIFY_SOURCE -fno-stack-protector -include $PREFIX/libcwrap.h"
# $LINUX && [ "$CROSSWIN" == "false" ] && CXX="$CXX -U_FORTIFY_SOURCE -fno-stack-protector -D_GLIBCXX_USE_CXX11_ABI=0 -include $PREFIX/libcwrap.h"

$CROSSWIN && MINGW="x86_64-w64-mingw32"
$CROSSWIN && export CXXFLAGS=""
$CROSSWIN && export HOST="$MINGW"
$CROSSWIN && export CC="$MINGW-gcc"
$CROSSWIN && export CXX="$MINGW-g++"
$CROSSWIN && export TOOLCHAIN="$root/cmake/toolchain-mingw64.cmake"

# multicore compilation
$MACOS && makearg="-j$(sysctl -n hw.ncpu)" || makearg="-j$(nproc)"

$LINUX && [ "$CROSSWIN" == "false" ] && cp -f "$SRC/libcwrap.h" "$PREFIX/libcwrap.h"

clean_prefix() {
	rm -rf "$PREFIX"
	mkdir -pv $PREFIX/{bin,include,lib,share}
}

build_glew() {
	echo "Building GLEW"
	cd "$SRC/glew"
	$CROSSWIN && SYSTEM="SYSTEM=linux-mingw64"
	$CROSSWIN && LDFLAGSEXTRA="-nostdlib -L/usr/$MINGW/lib"
	make CC="$CC" HOST="$MINGW" LIBDIR="$PREFIX/lib" LDFLAGS.EXTRA="$LDFLAGSEXTRA" GLEW_DEST="$PREFIX" GLEW_PREFIX="$PREFIX" $SYSTEM glew.lib $makearg
	make HOST="$MINGW" LIBDIR="$PREFIX/lib" GLEW_DEST="$PREFIX" GLEW_PREFIX="$PREFIX" $SYSTEM install
	make HOST="$MINGW" LIBDIR="$PREFIX/lib" GLEW_DEST="$PREFIX" GLEW_PREFIX="$PREFIX" $SYSTEM distclean
}

build_soundio() {
	echo "Building SoundIO"
	cd "$SRC/soundio"
	rm -rf build
	mkdir -p build
	cd build
	rm -f CMakeCache.txt
	cmake .. -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"
	make $makearg
	make install
	make clean
}

build_aubio() {
	echo "Building Aubio"
	cd "$SRC/aubio"
	./scripts/get_waf.sh
	$CROSSWIN && export TARGET=win64
	PKG_CONFIG_PATH="$PKG_CONFIG_PATH" ./waf configure --prefix="$PREFIX" --with-target-platform="$TARGET" \
		--disable-fftw3f --disable-fftw3 --disable-avcodec --disable-jack \
		--disable-sndfile --disable-apple-audio --disable-samplerate \
		--disable-wavread --disable-wavwrite \
		--disable-docs --disable-examples \
		build install distclean --testcmd='echo %s'
}

build_fgl() {
	echo "Building Freetype GL"
	cd "$SRC/ftgl"
	mkdir -p build
	cd build
	rm -f CMakeCache.txt
	cmake .. -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
		-Dfreetype-gl_BUILD_APIDOC=OFF \
		-Dfreetype-gl_BUILD_DEMOS=OFF \
		-Dfreetype-gl_BUILD_TESTS=OFF
	make $makearg
	make install
	install -v makefont "$PREFIX/bin/makefont"
	make clean
}

# yasm shouldn't be cross-compiled
build_yasm() {
	echo "Building Yasm"
	cd "$SRC/yasm"
	CC="" CXX="" ./configure --prefix="$PREFIX"
	make $makearg
	make install
	make distclean
}

build_ffmpeg() {
	echo "Building FFmpeg"
	cd "$SRC/ffmpeg"
	$CROSSWIN && export CROSS="--arch=x86 --target-os=mingw32 --cross-prefix=$MINGW- --enable-cross-compile"
	./configure --prefix="$PREFIX" \
		--cc="$CC" \
		--cxx="$CXX" \
		$CROSS \
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
	$CROSSWIN && export DEST_OS="win32"
	$CROSSWIN && export TARGET="$MINGW"
	PKG_CONFIG_PATH="$PKG_CONFIG_PATH" DEST_OS="$DEST_OS" TARGET="$TARGET" ./waf configure --prefix="$PREFIX" --disable-cplayer --enable-libmpv-shared \
		--disable-manpage-build --disable-android --disable-javascript \
		--disable-libass --disable-libass-osd --disable-libbluray \
		--disable-vapoursynth --disable-vapoursynth-lazy --disable-libarchive \
		--disable-oss-audio --disable-rsound --disable-wasapi --disable-alsa --disable-jack --disable-opensles \
		--disable-tv-v4l2 --disable-libv4l2 --disable-audio-input \
		--disable-apple-remote --disable-macos-touchbar --disable-macos-cocoa-cb \
		--disable-caca --disable-jpeg --disable-vulkan --disable-xv \
		--disable-lua \
		build install distclean
}

build_projectm() {
	echo "Building projectM"
	cd "$SRC/projectm"
	./configure --prefix="$PREFIX" --host="$MINGW" PKG_CONFIG_PATH="$PKG_CONFIG_PATH" CC="$CC" CXX="$CXX" \
		--disable-rpath --disable-qt --disable-sdl --disable-emscripten --disable-gles
	make $makearg
	make install
	make clean
}

# START OF BUILD PROCESS

if [ "$1" == "all" ]; then
	echo "Building all dependencies"
	clean_prefix

	build_glew

	build_soundio
	build_aubio

	# build_fgl

	build_yasm
	build_ffmpeg

	build_mpv


elif [ ! -z "$1" ]; then
	echo "Building dependency $1"
	build_$1
fi

touch "$PREFIX/built_libs"
