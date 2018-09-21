#!/bin/bash

# DLS specific http proxy
#export http_proxy=wwwcache.rl.ac.uk:8080

# Variables telling us where to get things
HERE="$(dirname "$0")"
VERSION="ffmpeg-4.0.2"
SOURCE="https://ffmpeg.org/releases/${VERSION}.tar.xz"
WIN32SHARED="http://ffmpeg.zeranoe.com/builds/win32/shared/${VERSION}-win32-shared.zip"
WIN32SHAREDDEV="http://ffmpeg.zeranoe.com/builds/win32/dev/${VERSION}-win32-dev.zip"
WIN64SHARED="http://ffmpeg.zeranoe.com/builds/win64/shared/${VERSION}-win64-shared.zip"
WIN64SHAREDDEV="http://ffmpeg.zeranoe.com/builds/win64/dev/${VERSION}-win64-dev.zip"
YASM="http://www.tortall.net/projects/yasm/releases/yasm-1.2.0.tar.gz"

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
tar Jxvf "${HERE}/vendor/$(basename $SOURCE)" -C "${HERE}/vendor"

# unzip the zip archives
unzip   "${HERE}/vendor/$(basename $WIN32SHARED)"    "-d${HERE}/vendor"
unzip   "${HERE}/vendor/$(basename $WIN32SHAREDDEV)" "-d${HERE}/vendor"
unzip   "${HERE}/vendor/$(basename $WIN64SHARED)"    "-d${HERE}/vendor" 
unzip   "${HERE}/vendor/$(basename $WIN64SHAREDDEV)" "-d${HERE}/vendor"

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

# patch win32 #defines for VC2003 compiler
patch -d "${HERE}" -p0 < "${HERE}/vendor/vc2003.patch"

echo "You can now type make to build this module"
