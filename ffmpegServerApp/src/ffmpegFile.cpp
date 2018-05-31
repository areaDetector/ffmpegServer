/* video output */
#include "ffmpegCommon.h"
#include "ffmpegFile.h"

#include <epicsExport.h>
#include <iocsh.h>

static const char *driverName2 = "ffmpegFile";

/** Opens an FFMPEG file.
  * \param[in] filename The name of the file to open.
  * \param[in] openMode Mask defining how the file should be opened; bits are 
  *            NDFileModeRead, NDFileModeWrite, NDFileModeAppend, NDFileModeMultiple
  * \param[in] pArray A pointer to an NDArray; this is used to determine the array and attribute properties.
  */
  
/**************************************************************/

asynStatus ffmpegFile::openFile(const char *filename, NDFileOpenMode_t openMode, NDArray *pArray)
{
    int ret;
	static const char *functionName = "openFile";
	char errbuf[AV_ERROR_MAX_STRING_SIZE];
    epicsFloat64 f64Val;
    this->sheight = 0;
	this->swidth = 0;

    /* We don't support reading yet */
    if (openMode & NDFileModeRead) return(asynError);

    /* We don't support opening an existing file for appending yet */
    if (openMode & NDFileModeAppend) return(asynError);

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s Could not deduce output format from file extension: using MPEG.\n",
            driverName2, functionName);
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s Memory error: Cannot allocate output context\n",
            driverName2, functionName);
        return(asynError);
    }

    /* If we are using image2 then we only support one frame per image */
    fmt = oc->oformat;
    if (strcmp("image2", fmt->name)==0) {
    	this->supportsMultipleArrays = 0;
    } else {
    	this->supportsMultipleArrays = 1;
        /* We want to use msmpeg4v2 instead of mpeg4 for avi files*/
        if (av_match_ext(filename, "avi") && fmt->video_codec == AV_CODEC_ID_MPEG4) {
        	fmt->video_codec = AV_CODEC_ID_MSMPEG4V2;
        }
    }

    codec_id = av_guess_codec(fmt, NULL, filename, NULL, AVMEDIA_TYPE_VIDEO);
    if (codec_id == AV_CODEC_ID_NONE) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s Codec has no video stream component\n",
            driverName2, functionName);
        return(asynError);
    }

    /* find the encoder */
    video_st = NULL;
    codec = avcodec_find_encoder(codec_id);
	if (!codec) {
		asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
					"%s:%s Could not find encoder for '%s'\n",
					driverName2, functionName, avcodec_get_name(codec_id));
		return(asynError);
	}

	/* Create the video stream */
	video_st = avformat_new_stream(oc, codec);
	if (!video_st) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s Could not allocate stream\n",
            driverName2, functionName);
        return(asynError);
	}
	video_st->id = oc->nb_streams-1;
	c = video_st->codec;

	if (codec->type != AVMEDIA_TYPE_VIDEO) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s Codec context is not of type AVMEDIA_TYPE_VIDEO\n",
            driverName2, functionName);
        return(asynError);
	}

	avcodec_get_context_defaults3(c, codec);
	c->codec_id = codec_id;

    /* put sample parameters */
    getDoubleParam(0, ffmpegFileBitrate, &f64Val);
    c->bit_rate = (int64_t)f64Val;

    /* frames per second */
    AVRational avr;
    avr.num = 1;
    getIntegerParam(0, ffmpegFileFPS, &(avr.den));

    /* resolution must be a multiple of two */
    getIntegerParam(0, ffmpegFileWidth, &(c->width));
    getIntegerParam(0, ffmpegFileHeight, &(c->height));
    /* time base: this is the fundamental unit of time (in seconds) in terms
       of which frame timestamps are represented. for fixed-fps content,
       timebase should be 1/framerate and timestamp increments should be
       identically 1. */
    c->time_base = avr;

	c->gop_size      = 12; /* emit one intra frame every twelve frames at most */

    c->pix_fmt = AV_PIX_FMT_YUV420P;
    if(codec && codec->pix_fmts){
        const enum AVPixelFormat *p= codec->pix_fmts;
        for(; *p!=-1; p++){
            if(*p == c->pix_fmt)
                break;
        }
        if(*p == -1)
            c->pix_fmt = codec->pix_fmts[0];
    }

	if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
		/* just for testing, we also add B frames */
		c->max_b_frames = 2;
	}
	if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
		/* Needed to avoid using macroblocks in which some coeffs overflow.
		 * This does not happen with normal video, it just happens here as
		 * the motion of the chroma plane does not match the luma plane. */
		c->mb_decision = 2;
	}

	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */

	c = video_st->codec;

	/* open the codec */
	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) {
		asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
		            "%s:%s Could not open video codec: %s\n",
		            driverName2, functionName, av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret));
		return(asynError);
	}

	/* dump the format so we can see it on the console... */
    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
	ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
	if (ret < 0) {
		asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
							"%s:%s Could not open '%s': %s\n",
							driverName2, functionName, filename, av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret));
		return(asynError);
	}

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, NULL);
    if (ret < 0) {
		asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
				"%s:%s Error occurred when opening output file %s: %s\n",
							driverName2, functionName, filename, av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret));
		return(asynError);
    }

	outSize = c->width * c->height * 6;   

    /* alloc array for output and compression */
    if (scArray) {
    	scArray->release();
    	scArray = NULL;
    }
    if (outArray) {
    	outArray->release();
    	outArray = NULL;
    }

    scArray = this->pNDArrayPool->alloc(1, &outSize, NDInt8, 0, NULL);
    outArray = this->pNDArrayPool->alloc(1, &outSize, NDInt8, 0, NULL);
    if (scArray == NULL || outArray == NULL) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s error allocating arrays\n",
            driverName2, functionName);
        if (scArray) {
        	scArray->release();
        	scArray = NULL;
        }
        if (outArray) {
        	outArray->release();
        	outArray = NULL;
        }
        return(asynError);
    }

    /* alloc in and scaled pictures */
    inPicture = av_frame_alloc();
    scPicture = av_frame_alloc();
	avpicture_fill((AVPicture *)scPicture,(uint8_t *)scArray->pData,c->pix_fmt,c->width,c->height);       
    scPicture->pts = 0;
	needStop = 1;
    return(asynSuccess);

}


/** Writes single NDArray to the ffmpeg file.
  * \param[in] pArray Pointer to the NDArray to be written
  */
asynStatus ffmpegFile::writeFile(NDArray *pArray)
{

    static const char *functionName = "writeFile";
	int ret;
	char errbuf[AV_ERROR_MAX_STRING_SIZE];

    if (!needStop) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: video stream not setup properly\n",
            driverName2, functionName);
        return(asynError);
	}
	
	if (formatArray(pArray, this->pasynUserSelf, this->inPicture,
		&(this->ctx), this->c, this->scPicture) != asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: Could not format array for correct pix_fmt for codec\n",
            driverName2, functionName);		
        return(asynError);
    }	
	

    /* encode the image */
    AVPacket pkt;
    int got_output;
    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    ret = avcodec_encode_video2(c, &pkt, this->scPicture, &got_output);
    if (ret < 0) {
        fprintf(stderr, "Error encoding video frame: %s\n", av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret));
        exit(1);
    }

    /* If size is zero, it means the image was buffered. */
    if (got_output) {
        if (c->coded_frame->key_frame)
            pkt.flags |= AV_PKT_FLAG_KEY;

        pkt.stream_index = video_st->index;

        /* Write the compressed frame to the media file. */
        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
    	printf("got_output = 0, shouldn't see this\n");
        ret = 0;
    }
    scPicture->pts += av_rescale_q(1, video_st->codec->time_base, video_st->time_base);

    if (ret != 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s Error while writing video frame\n",
            driverName2, functionName);
    }

    return(asynSuccess);
}

/** Reads single NDArray from a ffmpeg file; NOT CURRENTLY IMPLEMENTED.
  * \param[in] pArray Pointer to the NDArray to be read
  */
asynStatus ffmpegFile::readFile(NDArray **pArray)
{
    //static const char *functionName = "readFile";

    return asynError;
}


/** Closes the ffmpeg file. */
asynStatus ffmpegFile::closeFile()
{
	if (needStop == 0) {
	    return asynError;		
	}
	needStop = 0;

    /* write the trailer, if any.  the trailer must be written
     * before you close the CodecContexts open when you wrote the
     * header; otherwise write_trailer may try to use memory that
     * was freed on av_codec_close() */
    av_write_trailer(oc);

    /* close each codec */
    if (video_st) {
	    avcodec_close(video_st->codec);
	}
	
    /* free the streams */
    for(unsigned int i = 0; i < oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    if (!(fmt->flags & AVFMT_NOFILE)) {
        /* close the output file */
        avio_close(oc->pb);
    }

    /* free the stream */
    av_free(oc);
    if (scArray) {
    	scArray->release();
    	scArray = NULL;
    }
    if (outArray) {
    	outArray->release();
    	outArray = NULL;
    }

    av_free(inPicture);
    av_free(scPicture);  
    return asynSuccess;
}

/** Called when asyn clients call pasynFloat64->write().
  * This function ensures the ffmpegFileBitrate parameter value is within the
  * allowed range of -2^53 to 2^53.  If it is not, the value is not set in
  * the parameter library, an error is logged, and the asynError status is
  * returned.  Otherwise this function calls asynPortDriver::writeFloat64 with
  * the same arguments this function was called with and returns its status.
  * \param[in] pasynUser pasynUser structure that encodes the reason and
  *            address.
  * \param[in] value Value to write. */
asynStatus ffmpegFile::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
        const char *paramName;
        getParamName(function, &paramName);
    asynStatus status = asynSuccess;
    int addr=0;
    static const char *functionName = "writeFloat64";

    if (function == ffmpegFileBitrate &&
            (value < EXACT_INT_DBL_MIN || value > EXACT_INT_DBL_MAX)) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
            "%s:%s: value out of range, function=%d, name=%s, value=%.0f, "
                "min=%.0f, max=%.0f\n",
            driverName2, functionName, function, paramName, value,
            EXACT_INT_DBL_MIN, EXACT_INT_DBL_MAX);
        return asynError;
    }

    return asynPortDriver::writeFloat64(pasynUser, value);
}

/** Constructor for ffmpegFile; all parameters are simply passed to NDPluginFile::NDPluginFile.
ffmpegFileConfigure() should be used to create an instance in the iocsh.
See ffmpegStream.template for more details of usage.

  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
ffmpegFile::ffmpegFile(const char *portName, int queueSize, int blockingCallbacks,
                       const char *NDArrayPort, int NDArrayAddr,
                       int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArrays of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority can be 0, which will use defaults. 
     * We require a minimum stacksize of 128k for windows-x64 */
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_FFMPEG_FILE_PARAMS,
                   2, -1, asynGenericPointerMask, asynGenericPointerMask, 
                   ASYN_CANBLOCK, 1, priority, stackSize < 128000 ? 128000 : stackSize)
{
    //const char *functionName = "ffmpegFile";

    createParam(ffmpegFileBitrateString, asynParamFloat64, &ffmpegFileBitrate);
    createParam(ffmpegFileFPSString,     asynParamInt32,   &ffmpegFileFPS);
    createParam(ffmpegFileHeightString,  asynParamInt32,   &ffmpegFileHeight);
    createParam(ffmpegFileWidthString,   asynParamInt32,   &ffmpegFileWidth);

    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "ffmpegFile");
    this->supportsMultipleArrays = 0;
    
    /* Initialise the ffmpeg library */
    ffmpegInitialise();

    this->ctx = NULL;
    this->outFile = NULL;
    this->codec = NULL;
    this->c = NULL;
    this->inPicture = NULL;
    this->scPicture = NULL;    
    this->scArray = NULL;
    this->outArray = NULL;
    this->ctx = NULL;      
    this->fmt = NULL;
    this->oc = NULL;
    this->video_st = NULL;    
    
}

/** Configuration routine.  Called directly, or from the iocsh function, calls ffmpegFile constructor:

\copydoc ffmpegFile::ffmpegFile
*/

extern "C" int ffmpegFileConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int priority, int stackSize)
{
    new ffmpegFile(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                   priority, stackSize);                   
    return(asynSuccess);
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "priority",iocshArgInt};
static const iocshArg initArg6 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6};
static const iocshFuncDef initFuncDef = {"ffmpegFileConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    ffmpegFileConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void ffmpegFileRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(ffmpegFileRegister);
}
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
