#!/bin/bash

# DLS specific http proxy
export http_proxy=wwwcache.rl.ac.uk:8080

# Variables telling us where to get things
HERE="$(dirname "$0")"
VERSION="ffmpeg-0.8"
SOURCE="http://ffmpeg.zeranoe.com/builds/source/ffmpeg/${VERSION}.tar.bz2"
WIN32SHARED="http://ffmpeg.zeranoe.com/builds/win32/shared/${VERSION}-win32-shared.7z"
WIN32SHAREDDEV="http://ffmpeg.zeranoe.com/builds/win32/dev/${VERSION}-win32-dev.7z"
WIN64SHARED="http://ffmpeg.zeranoe.com/builds/win64/shared/${VERSION}-win64-shared.7z"
WIN64SHAREDDEV="http://ffmpeg.zeranoe.com/builds/win64/dev/${VERSION}-win64-dev.7z"
YASM="http://www.tortall.net/projects/yasm/releases/yasm-1.1.0.tar.gz"

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
for z in $SOURCE $WIN32SHARED $WIN32SHAREDDEV $WIN64SHARED $WIN64SHAREDDEV $YASM; do
	wget -P "${HERE}/vendor" $z
done

# untar the source
echo "Untarring source..."
tar xjf "${HERE}/vendor/$(basename $SOURCE)" -C "${HERE}/vendor"

# unzip the 7z archives
7zr x "-o${HERE}/vendor" "${HERE}/vendor/$(basename $WIN32SHARED)"
7zr x "-o${HERE}/vendor" "${HERE}/vendor/$(basename $WIN32SHAREDDEV)"
7zr x "-o${HERE}/vendor" "${HERE}/vendor/$(basename $WIN64SHARED)"
7zr x "-o${HERE}/vendor" "${HERE}/vendor/$(basename $WIN64SHAREDDEV)"

# untar yasm
echo "Untarring yasm..."
tar xzf "${HERE}/vendor/$(basename $YASM)" -C "${HERE}/vendor"

# remove the archives
for z in $SOURCE $WIN32SHARED $WIN32SHAREDDEV $WIN64SHARED $WIN64SHAREDDEV $YASM; do
	rm "${HERE}/vendor/$(basename $z)"
done

# move the untarred archives to the correct names
mv "${HERE}/vendor/${VERSION}" "${HERE}/vendor/ffmpeg"
mv "${HERE}/vendor/${VERSION}-win32-dev" "${HERE}/vendor/ffmpeg-win32-dev"
mv "${HERE}/vendor/${VERSION}-win32-shared" "${HERE}/vendor/ffmpeg-win32-shared"
mv "${HERE}/vendor/${VERSION}-win64-dev" "${HERE}/vendor/ffmpeg-win64-dev"
mv "${HERE}/vendor/${VERSION}-win64-shared" "${HERE}/vendor/ffmpeg-win64-shared"
mv ${HERE}/vendor/yasm* "${HERE}/vendor/yasm"

# patch mjpg parser
patch -d "${HERE}" -p0 < "${HERE}/vendor/mjpg.patch"

echo "You can now type make to build this module"
