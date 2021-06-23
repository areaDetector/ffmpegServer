#!/bin/bash

# DLS specific http proxy
#export http_proxy=wwwcache.rl.ac.uk:8080

# Variables telling us where to get things
HERE="$(dirname "$0")"
VERSION="ffmpeg-4.3"
SOURCE="https://ffmpeg.org/releases/${VERSION}.tar.xz"
YASM="http://www.tortall.net/projects/yasm/releases/yasm-1.2.0.tar.gz"

# First check whether the user has 7za, needed for the windows build
which 7za > /dev/null
if [ $? -ne 0 ]; then
	echo "*** Error: You need to download p7zip from http://sourceforge.net/projects/p7zip/files/p7zip and make sure '7za' is on your PATH"
	exit 1
fi

# fail if we can't do anything
set -e

# remove ffmpeg things in vendor dir
rm -rf ${HERE}/vendor/ffmpeg*
rm -rf ${HERE}/vendor/yasm*

# Now get the the zip files
for z in $SOURCE $YASM; do
	wget -P "${HERE}/vendor" $z
done

# untar the source
echo "Untarring source..."
tar xf "${HERE}/vendor/$(basename $SOURCE)" -C "${HERE}/vendor"
mv "${HERE}/vendor/${VERSION}" "${HERE}/vendor/ffmpeg"
rm "${HERE}/vendor/$(basename $SOURCE)"

# untar yasm
echo "Untarring yasm..."
tar xzf "${HERE}/vendor/$(basename $YASM)" -C "${HERE}/vendor"
rm "${HERE}/vendor/$(basename $YASM)"

# move the untarred archives to the correct names
# mv "${HERE}/vendor/${VERSION}" "${HERE}/vendor/ffmpeg"
mv ${HERE}/vendor/yasm* "${HERE}/vendor/yasm"

# TODO - IMPORTANT really need to get this fixed in the ffmpeg repo
sed "s/--disable-ffserver/--disable-bsfs --disable-filters --disable-decoders --disable-encoders --disable-parsers --disable-demuxers --disable-muxers --enable-encoder=mjpeg --enable-decoder=mjpeg --enable-parser=mjpeg --enable-demuxer=mjpeg --enable-muxer=mjpeg/" -i "${HERE}/vendor/Makefile"
sed 's/make install/\\/' -i "${HERE}/vendor/Makefile"

echo "You can now type make to build this module"