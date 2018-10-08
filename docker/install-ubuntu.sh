#!/usr/bin/env bash

set -e

sudo apt-get install -y \
	build-essential pkg-config cmake curl \
	libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev \
	libsdl2-dev \
	libx11-dev libwayland-dev wayland-protocols libdbus-1-dev libudev-dev libpulse-dev libasound2-dev \
	libjack-jackd2-dev \
	yasm \
	libglm-dev

./build.sh all
