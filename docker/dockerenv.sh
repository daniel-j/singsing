#!/usr/bin/env bash

set -eo pipefail

cd "${0%/*}"

targetarch="${ARCH-$(uname -m)}"

if [ "$targetarch" == "x86_64" ]; then
    imagename="singsing/buildenv:centos7"
    from="centos:7"
    prefixcmd="linux64"
elif [ "$targetarch" == "i386" ] || [ "$targetarch" == "i686" ]; then
    imagename="singsing/buildenv:centos7-i386"
    from="i386/centos:7"
    prefixcmd="linux32"
else
    echo "Unsupported architecture: $targetarch"
    exit 1
fi

replacements="
    s!%%from%%!$from!g;
"

sed "$replacements" Dockerfile.in | docker build --force-rm=true --rm -t "$imagename" -

docker run --rm -it \
    -v "$(realpath ..):/src" \
    -v "/etc/passwd:/etc/passwd:ro" \
    -v "/etc/group:/etc/group:ro" \
    --user $(id -u):$(id -g) \
    -e "TERM=$TERM" \
    -e "PS1=[\u@$(tput setaf 6)$(tput bold)\h:$(uname -m)$(tput sgr0) \W]\$ " \
    -e "PATH=/opt/rh/devtoolset-7/root/usr/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
    -h "singsingbuilder" \
    -w /src \
    "$imagename" \
    "$prefixcmd" "$@"
