#!/usr/bin/env bash

set -e

packages=(
	# base
	curl unzip
	# compiler and build systems
	build-essential pkg-config cmake
	# opengl
	libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev
	# sdl2
	libx11-dev libwayland-dev wayland-protocols libdbus-1-dev libudev-dev
	# soundio
	libpulse-dev libasound2-dev libjack-jackd2-dev
	# ffmpeg
	yasm
	# mpv
	libdrm-dev librubberband-dev libva-dev
	# projectm
	libglm-dev
)

sudo apt-get install -y "${packages[@]}"

./build.sh all
