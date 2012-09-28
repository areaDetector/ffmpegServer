TEMPLATE = subdirs
SUBDIRS = ffmpegWidget ffmpegViewer webcam4
ffmpegViewer.depends = ffmpegWidget
webcam4.depends = ffmpegWidget
