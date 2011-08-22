#!/bin/bash

# DLS specific http proxy
export http_proxy=wwwcache.rl.ac.uk:8080

# Variables telling us where to get things
HERE="$(dirname "$0")"
VERSION="ffmpeg-0.8"
SOURCE="http://ffmpeg.zeranoe.com/builds/source/ffmpeg/${VERSION}.tar.bz2"
WIN32SHARED="http://ffmpeg.zeranoe.com/builds/win32/shared/${VERSION}-win32-shared.7z"
WIN32SHAREDDEV="http://ffmpeg.zeranoe.com/builds/win32/dev/${VERSION}-win32-dev.7z"
YASM="http://www.tortall.net/projects/yasm/releases/yasm-1.0.1.tar.gz"

# First check whether the user has 7zr, needed for the windows build
which 7zr > /dev/null
if [ $? -ne 0 ]; then
	echo "*** Error: You need to download p7zip from http://sourceforge.net/projects/p7zip/files/p7zip and make sure '7zr' is on your PATH"
	exit 1
fi

# fail if we can't do anything
set -e

# remove ffmpeg things in vendor dir
rm -rf ${HERE}/vendor/ffmpeg*
rm -rf ${HERE}/vendor/yasm*

# Now get the the zip files
for z in $SOURCE $WIN32SHARED $WIN32SHAREDDEV $YASM; do
	wget -P "${HERE}/vendor" $z
done

# untar the source
echo "Untarring source..."
tar xjf "${HERE}/vendor/$(basename $SOURCE)" -C "${HERE}/vendor"

# unzip the 7z archives
7zr x "-o${HERE}/vendor/ffmpeg-mingw32-shared" "${HERE}/vendor/$(basename $WIN32SHARED)"
7zr x -y "-o${HERE}/vendor/ffmpeg-mingw32-shared" "${HERE}/vendor/$(basename $WIN32SHAREDDEV)"

# untar yasm
echo "Untarring yasm..."
tar xzf "${HERE}/vendor/$(basename $YASM)" -C "${HERE}/vendor"

# remove the archives
for z in $SOURCE $WIN32SHARED $WIN32SHAREDDEV $YASM; do
	rm "${HERE}/vendor/$(basename $z)"
done

# move the untarred archives to the correct names
mv "${HERE}/vendor/${VERSION}" "${HERE}/vendor/ffmpeg"
mv ${HERE}/vendor/yasm* "${HERE}/vendor/yasm"

echo "You can now type make to build this module"
