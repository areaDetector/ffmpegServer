#ifndef ffmpegCommon_H
#define ffmpegCommon_H

#define __STDC_CONSTANT_MACROS
#include <stdint.h>

/* ffmpeg includes */
extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
}

/* areaDetector includes */
#include "NDArray.h"

/* asyn includes */
#include "asynDriver.h"

/* This wraps the initialisation of the ffmpeg library */
void ffmpegInitialise();

/* This is needed to give a neutral colour to Mono8 jpegs. Needs to be at
   least maxw * maxh / 2 in size */
#define NEUTRAL_FRAME_SIZE 4000 * 3000 / 2

/* This formats an NDArray so that it matches the input for a particular
   codec context */
int formatArray(NDArray *pArray, asynUser *pasynUser, AVFrame *inPicture,
		struct SwsContext **pCtx, AVCodecContext *c, AVFrame *scPicture);

#endif
