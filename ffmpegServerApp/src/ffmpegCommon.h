#ifndef ffmpegCommon_H
#define ffmpegCommon_H

#define __STDC_CONSTANT_MACROS

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
/**
 * \file
 * \section License
 * Author: Diamond Light Source, Copyright 2010
 *
 * 'ffmpegServer' is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * 'ffmpegServer' is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with 'ffmpegServer'.  If not, see http://www.gnu.org/licenses/.
 */
