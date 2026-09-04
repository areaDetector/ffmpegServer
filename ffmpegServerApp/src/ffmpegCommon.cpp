#include "ffmpegCommon.h"

static int ffmpegInitialised = 0;
static unsigned char *neutral = NULL;

/** Initialise the ffmpeg library */
void ffmpegInitialise() {
	/* check if we're already intialised */
	if (ffmpegInitialised) return;
	
    /* Make a big neutral array */
    neutral = (unsigned char *)malloc(NEUTRAL_FRAME_SIZE *  sizeof(unsigned char));
    memset(neutral, 128, NEUTRAL_FRAME_SIZE);           
         
    /* say that we're initialise */
	ffmpegInitialised = 1;
}    

/** Format an NDArray as an AVFrame using the codec context c.
Use inPicture to wrap the data in pArray, use swscale to convert it to scPicture
using the output parameters stored in c. Special case for gray8 -> YUVx, can
use the data as is and add a neutral array for the colour info.
*/
int formatArray(NDArray *pArray, asynUser *pasynUser, AVFrame *inPicture,
		struct SwsContext **pCtx, AVCodecContext *c, AVFrame *scPicture) {
    static const char *functionName = "formatArray";
    int colorMode = NDColorModeMono;
	int width, height;
	enum AVPixelFormat pix_fmt;
	NDAttribute *pAttribute = NULL;
	int Int16;
	
	/* only support 8 and 16 bit data for now */
    switch (pArray->dataType) {
        case NDInt8:
        case NDUInt8:
            Int16 = 0;
            break;
        case NDInt16:
        case NDUInt16:       
            Int16 = 1; 
            break;
        default:
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                "%s: only 8 or 16-bit data is supported\n", functionName);
        	return(asynError);
    }	

	/* Get the colormode of the array */	
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);

    /* We do some special treatment based on colorMode */
    if ((pArray->ndims == 2) && (colorMode == NDColorModeMono)) {
        width  = (int) pArray->dims[0].size;
        height = (int) pArray->dims[1].size;
        pix_fmt = Int16 ? AV_PIX_FMT_GRAY16 : AV_PIX_FMT_GRAY8;

        /* setup the input picture */
        inPicture->data[0] = (uint8_t*) pArray->pData;
        inPicture->linesize[0] = width * (Int16 + 1);
    } else if ((pArray->ndims == 3) && (pArray->dims[0].size == 3) && (colorMode == NDColorModeRGB1)) {
        width  = (int) pArray->dims[1].size;
        height = (int) pArray->dims[2].size;
        if (Int16) {
            pix_fmt = AV_PIX_FMT_RGB48;
        } else {
            pix_fmt = AV_PIX_FMT_RGB24;
        }    
        /* setup the input picture */
        inPicture->data[0] = (uint8_t*) pArray->pData;
        inPicture->linesize[0] = width * (Int16 + 1) * 3;          
    } else {
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
            "%s: unsupported array structure\n", functionName);
        return(asynError);
    }

	/* setup the swscale ctx */   
	*pCtx = sws_getCachedContext(*pCtx, width, height, pix_fmt,
                                  c->width, c->height, c->pix_fmt,
                                  SWS_BICUBIC, NULL, NULL, NULL);   
                                  
    if (*pCtx == NULL) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
            "%s: sws_getCachedContext failed\n", functionName);
        return(asynError);
    }
    /* scale the picture so we can pass it to the encoder */
    sws_scale(*pCtx, inPicture->data, inPicture->linesize, 0,
		      height, scPicture->data, scPicture->linesize);

    scPicture->format = c->pix_fmt;
    scPicture->width = c->width;
    scPicture->height = c->height;

	return(asynSuccess);
}
	
/**
 * \file
 * section License
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
