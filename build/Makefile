install: ../bin/linux-x86/ffmpeg ffmpegViewer
	$(MAKE) -C ffmpegViewer
	$(MAKE) -C ffmpegViewer install
	
clean:
	rm -rf ffmpeg_linux ffmpegViewer

ffmpeg_linux:
	mkdir $@
	cd $@ && ../../ffmpeg/configure --prefix=../.. \
		--bindir=../../bin/linux-x86 \
		--datadir=../../data --libdir=../../lib/linux-x86 \
		--shlibdir=../../lib/linux-x86 --incdir=../../include \
		--mandir=../../documentation/man --disable-indev=v4l \
		--disable-bzlib --disable-zlib
	$(MAKE) -C $@

ffmpegViewer:
	mkdir $@
	cd $@ && /dls_sw/prod/tools/RHEL5/bin/qmake -unix -o Makefile ../../ffmpegViewer/ffmpegViewer.pro

../bin/linux-x86/ffmpeg: ffmpeg_linux
	$(MAKE) -C ffmpeg_linux install


