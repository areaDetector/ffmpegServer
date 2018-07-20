ffmpegServer
============

ffmpegServer is a windows and linux [areaDetector](http://cars9.uchicago.edu/software/epics/areaDetector.html) plugin wrapping the [ffmpeg](http://www.ffmpeg.org) libraries that provides 2 functions:

* ffmpegStream: Compression into an [mjpg](http://en.wikipedia.org/wiki/Motion_JPEG) stream which is made available over http.
* ffmpegFile: Compression to disk into any file format that ffmpeg supports.

External Modules
----------------

Please note that some releases may depend on dls tagged versions of external modules (areaDetector, asyn, busy, autosave). These modifications are only relevant for internal diamond uses, and standard versions of these modules can be used.

Stream view applications
------------------------

A regular browser can be used to view the mjpeg streams.

A stand-alone viewer application based on Qt4 has also been developed. See [ffmpegViewer](https://github.com/areaDetector/ffmpegViewer)

History
-------

ffmpegServer is developed by Tom Cobb, Diamond Light Source and was originally distributed from [the Diamond Controls Group website](http://controls.diamond.ac.uk/downloads/support/ffmpegServer) where older versions can still be found.


