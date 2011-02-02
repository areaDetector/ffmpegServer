/* video output */
#include "ffmpegCommon.h"
#include "ffmpegFile.h"

#include <epicsExport.h>
#include <iocsh.h>

static const char *driverName2 = "ffmpegFile";
#define MAX_ATTRIBUTE_STRING_SIZE 256

/** Opens an FFMPEG file.
  * \param[in] fileName The name of the file to open.
  * \param[in] openMode Mask defining how the file should be opened; bits are 
  *            NDFileModeRead, NDFileModeWrite, NDFileModeAppend, NDFileModeMultiple
  * \param[in] pArray A pointer to an NDArray; this is used to determine the array and attribute properties.
  */
  
/**************************************************************/
asynStatus ffmpegFile::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
    static const char *functionName = "openFile";
    int codi;
	this->sheight = 0;
	this->swidth = 0;

    /* We don't support reading yet */
    if (openMode & NDFileModeRead) return(asynError);

    /* We don't support opening an existing file for appending yet */
    if (openMode & NDFileModeAppend) return(asynError);

    /* auto detect the output format from the name. default is
       mpeg. */
    fmt = av_guess_format(NULL, fileName, NULL);
    if (!fmt) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        fmt = av_guess_format("mpeg", NULL, NULL);
    }
    if (!fmt) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s Could not find suitable output format for '%s'\n",
            driverName2, functionName, fileName);
        return(asynError);
    }

    /* allocate the output media context */
    oc = avformat_alloc_context();
    if (!oc) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s Memory error: Cannot allocate context\n",
            driverName2, functionName);
        return(asynError);
    }
    oc->oformat = fmt;
    snprintf(oc->filename, sizeof(oc->filename), "%s", fileName);

    /* add the audio and video streams using the default format codecs
       and initialize the codecs */
    if (fmt->video_codec == CODEC_ID_NONE) {    
        video_st = NULL;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s Selected codec cannot write video\n",
            driverName2, functionName);   
        return(asynError);
    } else {
        video_st = av_new_stream(oc, 0);
        if (!video_st) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s Could not alloc stream\n",
                driverName2, functionName);
            return(asynError);
        }
        avcodec_get_context_defaults2(video_st->codec, CODEC_TYPE_VIDEO);
          
        /* find the video encoder */
        getIntegerParam(0, NDFileFormat, &codi);
        switch (codi) {
        	case 0:
                codec = avcodec_find_encoder(fmt->video_codec);        	
        	    break;
        	case 1:
        	    codec = avcodec_find_encoder_by_name("msmpeg4v2");
        	    break;
        	case 2:
        	    codec = avcodec_find_encoder_by_name("mpeg4");	     
        	    break;  	        	    
        	default:
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                    "%s:%s Invalid codec selection\n",
                    driverName2, functionName);   
            return(asynError);                    	
	    }
	    if (!codec) {
	        printf("Codec cannot be found, trying msmpeg4v2\n");
	        codec = avcodec_find_encoder_by_name("msmpeg4v2"); 
	    }   
	    if (!codec) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s Could not open any codecs\n",
                driverName2, functionName);   
            return(asynError);      
	    }	    
	    c = video_st->codec;
        c->codec_id = codec->id;          
        c->codec_type = CODEC_TYPE_VIDEO;

        /* put sample parameters */
        getIntegerParam(0, ffmpegFileBitrate, &(c->bit_rate));
        
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
        c->gop_size = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt = PIX_FMT_YUV420P;
        if(codec && codec->pix_fmts){
            const enum PixelFormat *p= codec->pix_fmts;
            for(; *p!=-1; p++){
                if(*p == c->pix_fmt)
                    break;
            }
            if(*p == -1)
                c->pix_fmt = codec->pix_fmts[0];
        }        

        // some formats want stream headers to be separate
        if(oc->oformat->flags & AVFMT_GLOBALHEADER)
            c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    }

    /* set the output parameters (must be done even if no
       parameters). */
    if (av_set_parameters(oc, NULL) < 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s Invalid output format parameters\n",
            driverName2, functionName);            
        return(asynError);
    }

    dump_format(oc, 0, fileName, 1);

    /* now that all the parameters are set, we can open the audio and
       video codecs and allocate the necessary encode buffers */

    /* find the video encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s codec not found\n",
            driverName2, functionName);
        return(asynError);
    }

    /* open the codec */
    if (avcodec_open(c, codec) < 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s could not open codec\n",
            driverName2, functionName);
        return(asynError);
    }
	outSize = c->width * c->height * 6;   

    /* alloc array for output and compression */
    scArray = this->pNDArrayPool->alloc(1, &outSize, NDInt8, 0, NULL);
    outArray = this->pNDArrayPool->alloc(1, &outSize, NDInt8, 0, NULL);    

    /* alloc in and scaled pictures */
    inPicture = avcodec_alloc_frame();
    scPicture = avcodec_alloc_frame();
	avpicture_fill((AVPicture *)scPicture,(uint8_t *)scArray->pData,c->pix_fmt,c->width,c->height);       

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        if (url_fopen(&oc->pb, fileName, URL_WRONLY) < 0) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s error opening file %s\n",
                driverName2, functionName, fileName);
            scArray->release();
            outArray->release();
            return(asynError);
        }
    }

    /* write the stream header, if any */
    if (av_write_header(oc) < 0) 
        printf("Could not write header for output file (incorrect codec parameters ?)");    
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
	
    video_pts = (double)video_st->pts.val * video_st->time_base.num / video_st->time_base.den;
            
    if (oc->oformat->flags & AVFMT_RAWPICTURE) {
        /* raw video case. The API will change slightly in the near
            futur for that */
        AVPacket pkt;
        av_init_packet(&pkt);
        printf("How did you manage to get in this mode?\n");
        pkt.flags |= PKT_FLAG_KEY;
        pkt.stream_index= video_st->index;
        pkt.data= (uint8_t *)scPicture;
        pkt.size= sizeof(AVPicture);

        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
        /* encode the image */
        int out_size = avcodec_encode_video(c, (uint8_t *) outArray->pData, outSize, scPicture);
        /* if zero size, it means the image was buffered */
        if (out_size > 0) {
            AVPacket pkt;
            av_init_packet(&pkt);

            if (c->coded_frame->pts != (uint) AV_NOPTS_VALUE)
                pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, video_st->time_base);
            if(c->coded_frame->key_frame)
                pkt.flags |= PKT_FLAG_KEY;
            pkt.stream_index= video_st->index;
            pkt.data= (uint8_t *) outArray->pData;
            pkt.size= out_size;

            /* write the compressed frame in the media file */
            ret = av_interleaved_write_frame(oc, &pkt);
        } else {
            ret = 0;
        }
    }
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
	    return asynSuccess;		
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
    for(uint i = 0; i < oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    if (!(fmt->flags & AVFMT_NOFILE)) {
        /* close the output file */
        url_fclose(oc->pb);
    }

    /* free the stream */
    av_free(oc);
        
    scArray->release();
    outArray->release();

    av_free(inPicture);
    av_free(scPicture);  
    return asynSuccess;
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
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_FFMPEG_FILE_PARAMS,
                   2, -1, asynGenericPointerMask, asynGenericPointerMask, 
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    //const char *functionName = "ffmpegFile";

    createParam(ffmpegFileBitrateString,  asynParamInt32, &ffmpegFileBitrate);
    createParam(ffmpegFileFPSString,      asynParamInt32, &ffmpegFileFPS);
    createParam(ffmpegFileHeightString,   asynParamInt32, &ffmpegFileHeight);
    createParam(ffmpegFileWidthString,    asynParamInt32, &ffmpegFileWidth);

    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "ffmpegFile");
    this->supportsMultipleArrays = 1;
    
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
