#!/usr/bin/env bash

set -e

. ./env.sh

declare -a deps
deps+=('glew|https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0.tgz/download')
deps+=('soundio|https://github.com/andrewrk/libsoundio/archive/7c53dabc6e21d08c68ce6764f3108f6349ced308.tar.gz')
deps+=('yasm|http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz')
deps+=('ffmpeg|https://www.ffmpeg.org/releases/ffmpeg-4.0.2.tar.xz')
deps+=('mpv|https://github.com/daniel-j/mpv/archive/ao_cb.tar.gz')
deps+=('aubio|https://aubio.org/pub/aubio-0.4.7.tar.bz2')
deps+=('ftgl|https://github.com/rougier/freetype-gl/archive/7a290ac372d2bd94137a9fed6b2f5a6ac4360ec2.tar.gz')
deps+=('projectm|https://github.com/projectM-visualizer/projectm/releases/download/v3.1.0/projectM-3.1.0.tar.gz')
$LINUX && deps+=('libcwrap.h|https://raw.githubusercontent.com/wheybags/glibc_version_header/master/version_headers/force_link_glibc_2.10.2.h')
deps+=('cold-video.mp4|https://djazz.se/lab/usdx-player/songs/Synthis,%204EverfreeBrony,%20FritzyBeat%20-%20Cold/video.mp4')
deps+=('cold-song.mp3|https://djazz.se/lab/usdx-player/songs/Synthis,%204EverfreeBrony,%20FritzyBeat%20-%20Cold/song.mp3')
deps+=('cold-notes.txt|https://djazz.se/lab/usdx-player/songs/Synthis,%204EverfreeBrony,%20FritzyBeat%20-%20Cold/notes.txt')

mkdir -p deps

echo "Downloading dependencies"
for i in "${deps[@]}"; do
	IFS='|' read -a dep <<< "$i"
	name="${dep[0]}"
	url="${dep[1]}"
	if [ -e "deps/$name" ] && [ "$1" != "$name" ] && [ "$1" != "all" ]; then
		continue
	fi
	rm -rf "deps/$name"
	echo "Downloading $url"
	filename="${url%/download}"
	case "$filename" in
	*.zip)
		mkdir -p deps
		curl --progress-bar -L "$url" -o deps/$name.zip
		mkdir -p "deps/$name.tmp"
		unzip -q deps/$name.zip -d "deps/$name.tmp"
		rm deps/$name.zip
		mv "deps/$name.tmp/"* "deps/$name"
		rmdir "deps/$name.tmp"
		;;

	*.tar|*.tar.gz|*.tar.xz|*.tar.bz2|*.tgz|*.tar.mlk)
		case "$filename" in
		*xz)	compressor=J ;;
		*bz2)	compressor=j ;;
		*gz)	compressor=z ;;
		*)	compressor= ;;
		esac
		mkdir -p "deps/$name"
		curl --progress-bar -L "$url" | tar -x$compressor -C "deps/$name" --strip-components=1
		;;

	*)
		mkdir -p deps
		curl --progress-bar -L "$url" -o deps/$name
		chmod +x deps/$name
		;;
	esac
done
