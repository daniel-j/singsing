#!/usr/bin/env bash
#
# Usage:
#   ./build.sh all               # cleans and builds everything
#   ./build.sh <component>       # builds a single component
#   ./build.sh                   # builds just the main program
#   ./build.sh run               # runs the program
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
export BUILD_TYPE=Debug

# $LINUX && export CC="$CC"
# $LINUX && export CXX="$CXX -static-libgcc -static-libstdc++"

# $LINUX && [ "$CROSSWIN" == "false" ] && CC="$CC -U_FORTIFY_SOURCE -fno-stack-protector -include $PREFIX/libcwrap.h"
# $LINUX && [ "$CROSSWIN" == "false" ] && CXX="$CXX -U_FORTIFY_SOURCE -fno-stack-protector -D_GLIBCXX_USE_CXX11_ABI=0 -include $PREFIX/libcwrap.h"

$CROSSWIN && MINGW="x86_64-w64-mingw32"
$CROSSWIN && export CXXFLAGS=""
$CROSSWIN && export HOST="$MINGW"
$CROSSWIN && export CC="$MINGW-gcc -static-libgcc"
$CROSSWIN && export CXX="$MINGW-g++ -static-libgcc -static-libstdc++"
$CROSSWIN && export TOOLCHAIN="$root/cmake/toolchain-mingw64.cmake"

# multicore compilation
$MACOS && makearg="-j$(sysctl -n hw.ncpu)" || makearg="-j$(nproc)"

mkdir -pv $PREFIX/{bin,include,lib,share}

clean_prefix() {
	tput setaf 10 && tput bold
	echo "==> Cleaning prefix"
	tput sgr0
	rm -rf "$PREFIX"
	mkdir -pv $PREFIX/{bin,include,lib,share}
}

build_glew() {
	tput setaf 10 && tput bold
	echo "==> Building GLEW"
	tput sgr0
	cd "$SRC/glew"
	if $CROSSWIN; then
		make SYSTEM=mingw \
			 CC="$MINGW-gcc" \
			 AR="$MINGW-ar" ARFLAGS=crs \
			 RANLIB="$MINGW-ranlib" \
			 STRIP="$MINGW-strip" \
			 LD="$MINGW-gcc" \
			 HOST="$HOST" \
			 LIBDIR="$PREFIX/lib" \
			 LDFLAGS.EXTRA="-L/usr/$MINGW/lib -nostdlib" \
			 GLEW_DEST="$PREFIX" GLEW_PREFIX="$PREFIX" \
			 install $makearg
		make LIBDIR="$PREFIX/lib" GLEW_DEST="$PREFIX" GLEW_PREFIX="$PREFIX" distclean
	else
		make LIBDIR="$PREFIX/lib" \
			 GLEW_DEST="$PREFIX" GLEW_PREFIX="$PREFIX" \
			 install $makearg
		make LIBDIR="$PREFIX/lib" GLEW_DEST="$PREFIX" GLEW_PREFIX="$PREFIX" distclean
	fi
}

build_freetype() {
	tput setaf 10 && tput bold
	echo "==> Building FreeType"
	tput sgr0
	cd "$SRC/freetype"
	./configure --prefix="$PREFIX" PKG_CONFIG_PATH="$PKG_CONFIG_PATH" CC="$CC" CXX="$CXX" \
		--disable-static --with-harfbuzz=no --with-png=no --with-bzip2=no
	make $makearg
	make install
	make distclean
}

build_ftgl() {
	tput setaf 10 && tput bold
	echo "==> Building FreeType-GL"
	tput sgr0
	cd "$SRC/ftgl"
	mkdir -p build
	cd build
	rm -f CMakeCache.txt
	cmake .. -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
		-Dfreetype-gl_BUILD_APIDOC=OFF \
		-Dfreetype-gl_BUILD_DEMOS=OFF \
		-Dfreetype-gl_BUILD_TESTS=OFF \
		-Dfreetype-gl_USE_VAO=ON \
		-DOpenGL_GL_PREFERENCE=GLVND \
		-DCMAKE_BUILD_TYPE=MinSizeRel
	make $makearg
	make install
	install -v makefont "$PREFIX/bin/makefont"
	make clean
}

build_sdl2() {
	tput setaf 10 && tput bold
	echo "==> Building SDL2"
	tput sgr0
	cd "$SRC/sdl2"
	bash ./autogen.sh || true
	# rm -rf build
	mkdir -p build
	cd build
	../configure --prefix="$PREFIX" --host="$HOST" PKG_CONFIG_PATH="$PKG_CONFIG_PATH" CC="$CC" CXX="$CXX" LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" \
		--enable-sdl-dlopen \
		--enable-alsa-shared --enable-pulseaudio-shared \
		--enable-jack-shared --disable-sndio --enable-sndio-shared \
		--disable-nas --disable-esd --disable-arts --disable-diskaudio \
		--enable-video-wayland --enable-wayland-shared --enable-x11-shared \
		--disable-ibus --disable-fcitx --disable-ime \
		--disable-rpath --disable-video-vulkan --disable-render
	make $makearg
	make install
	make distclean
	# cmake ../sdl2 -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"
	# make clean
}

build_sdl2_ttf() {
	tput setaf 10 && tput bold
	echo "==> Building SDL2_ttf"
	tput sgr0
	cd "$SRC/sdl2_ttf"
	bash ./autogen.sh || true
	# rm -rf build
	mkdir -p build
	cd build
	../configure --prefix="$PREFIX" --host="$HOST" PKG_CONFIG_PATH="$PKG_CONFIG_PATH" CC="$CC" CXX="$CXX" LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" \
		--disable-static --with-sdl-prefix="$PREFIX"
	make $makearg
	make install
	make distclean
}

build_soundio() {
	tput setaf 10 && tput bold
	echo "==> Building SoundIO"
	tput sgr0
	cd "$SRC/soundio"
	rm -rf build
	mkdir -p build
	cd build
	rm -f CMakeCache.txt
	cmake .. -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
		-DBUILD_STATIC_LIBS=NO -DCMAKE_BUILD_TYPE=MinSizeRel \
		-DBUILD_TESTS=NO # tests require gcov
	make $makearg
	make install
	make clean
}

build_aubio() {
	tput setaf 10 && tput bold
	echo "==> Building Aubio"
	tput sgr0
	cd "$SRC/aubio"
	$CROSSWIN && export TARGET=win64
	PKG_CONFIG_PATH="$PKG_CONFIG_PATH" ./waf configure --prefix="$PREFIX" --with-target-platform="$TARGET" \
		--disable-fftw3f --disable-fftw3 --disable-avcodec --disable-jack \
		--disable-sndfile --disable-apple-audio --disable-samplerate \
		--disable-wavread --disable-wavwrite \
		--disable-docs --disable-examples \
		build install distclean --testcmd='echo %s'
}

build_ffmpeg() {
	tput setaf 10 && tput bold
	echo "==> Building FFmpeg"
	tput sgr0
	cd "$SRC/ffmpeg"
	echo "Configuring... (this takes a while)"
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
	tput setaf 10 && tput bold
	echo "==> Building MPV"
	tput sgr0
	cd "$SRC/mpv"
	./bootstrap.py
	$CROSSWIN && export DEST_OS="win32"
	$CROSSWIN && export TARGET="$MINGW"
	PKG_CONFIG_PATH="$PKG_CONFIG_PATH" DEST_OS="$DEST_OS" TARGET="$TARGET" CC="$CC" ./waf configure --prefix="$PREFIX" --enable-libmpv-shared \
		--disable-manpage-build --disable-android --disable-javascript \
		--disable-libass --disable-libass-osd --disable-libbluray \
		--disable-vapoursynth --disable-vapoursynth-lazy --disable-libarchive \
		--disable-oss-audio --disable-rsound --disable-wasapi --disable-alsa --disable-jack --disable-opensles \
		--disable-tv-v4l2 --disable-libv4l2 --disable-audio-input \
		--disable-apple-remote --disable-macos-touchbar --disable-macos-cocoa-cb \
		--disable-caca --disable-jpeg --disable-vulkan --disable-xv \
		--disable-lua --disable-lcms2 --enable-sdl2 \
		build install distclean
	# --disable-cplayer
}

build_singsing() {
	tput setaf 10 && tput bold
	echo "==> Building singsing"
	tput sgr0
	cd "$root"
	export buildpath=build
	$CROSSWIN && export buildpath=buildwin
	mkdir -p "$buildpath"
	cd "$buildpath"
	# rm -f CMakeCache.txt
	cmake .. -DCMAKE_PREFIX_PATH="$PREFIX" -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_INSTALL_RPATH="$PREFIX/lib" -DCMAKE_BUILD_TYPE=$BUILD_TYPE
	make $makearg
	make install
}

# START OF BUILD PROCESS

if [ "$1" == "all" ]; then
	tput setaf 10 && tput bold
	echo "==> Building everything"
	tput sgr0

	clean_prefix

	# build_glew

	build_freetype
	# build_ftgl

	build_sdl2
	build_sdl2_ttf

	build_soundio
	build_aubio

	build_ffmpeg # requires yasm

	build_mpv

	touch "$PREFIX/built_libs"

	# ensure a clean build
	$CROSSWIN && rm -rf buildwin || rm -rf build

	build_singsing

elif [ "$1" == "run" ]; then
	build_singsing
	cd "$root"
	tput setaf 10 && tput bold
	shift
	echo "==> Running singsing"
	tput sgr0
	$CROSSWIN && exec wine prefixwin/bin/singsing.exe "$@" || LD_LIBRARY_PATH=prefix/lib exec prefix/bin/singsing "$@"

elif [ "$1" == "release" ]; then
	export BUILD_TYPE=Release
	# ensure a clean build
	$CROSSWIN && rm -rf buildwin || rm -rf build
	build_singsing

elif [ ! -z "$1" ]; then
	build_$1
else
	# no arguments: rebuild
	build_singsing
fi
