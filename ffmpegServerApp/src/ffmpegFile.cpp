/*#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>

#include <epicsStdio.h>*/
#include <epicsExport.h>
#include <iocsh.h>

#include "ffmpegFile.h"


static const char *driverName2 = "ffmpegFile";
#define MAX_ATTRIBUTE_STRING_SIZE 256

/** The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static asynParamString_t ffmpegFileParamString[] = {
    {ffmpegFileBitrate,     "FFMPEG_BITRATE"},
    {ffmpegFileFPS,         "FFMPEG_FPS"}    
};
#define NUM_FFMPEG_FILE_PARAMS (sizeof(ffmpegFileParamString)/sizeof(ffmpegFileParamString[0]))

/** Opens a JPEG file.
  * \param[in] fileName The name of the file to open.
  * \param[in] openMode Mask defining how the file should be opened; bits are 
  *            NDFileModeRead, NDFileModeWrite, NDFileModeAppend, NDFileModeMultiple
  * \param[in] pArray A pointer to an NDArray; this is used to determine the array and attribute properties.
  */
asynStatus ffmpegFile::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
    static const char *functionName = "openFile";
    int colorMode = NDColorModeMono;
    int codi;
    NDAttribute *pAttribute;

    /* We don't support reading yet */
    if (openMode & NDFileModeRead) return(asynError);

    /* We don't support opening an existing file for appending yet */
    if (openMode & NDFileModeAppend) return(asynError);

   /* Create the file. */
    if ((this->outFile = fopen(fileName, "wb")) == NULL ) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s:%s error opening file %s\n",
        driverName2, functionName, fileName);
        return(asynError);
    }

    /* find the video encoder */
    getIntegerParam(0, ffmpegFileBitrate, &codi);
    switch (codi) {
    	case 0: 		    	
    	default:
		    codec = avcodec_find_encoder(CODEC_ID_MSMPEG4V2);    	
	}

    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

	/* Setup codec context */
    c = avcodec_alloc_context();

    /* We do some special treatment based on colorMode */
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);

    if ((pArray->ndims == 2) && (colorMode == NDColorModeMono)) {
        c->width  = pArray->dims[0].size;
        c->height = pArray->dims[1].size;
        outSize = c->width * c->height;
    } else if ((pArray->ndims == 3) && (pArray->dims[0].size == 3) && (colorMode == NDColorModeRGB1)) {
        c->width  = pArray->dims[1].size;
        c->height = pArray->dims[2].size;
        outSize = c->width * c->height * 3;        
    } else {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: unsupported array structure\n",
            driverName2, functionName);
        return(asynError);
    }    

    /* put sample parameters */
    getIntegerParam(0, ffmpegFileBitrate, &(c->bit_rate));
    
    /* frames per second */
    AVRational avr;
    avr.num = 1;    
    getIntegerParam(0, ffmpegFileFPS, &(avr.den));
    c->time_base= avr;
/*    c->gop_size = 10; /* emit one intra frame every ten frames */
/*    c->max_b_frames=1;	*/
    
	/* pick a suitable format */
    c->pix_fmt = codec->pix_fmts[0];
    
    /* open it */
    if (avcodec_open(c, codec) < 0) {
        fprintf(stderr, "could not open codec\n");
        return(asynError);
    }
    
    /* alloc array for output and compression */
    scArray = this->pNDArrayPool->alloc(1, &outSize, NDInt8, 0, NULL);
    outArray = this->pNDArrayPool->alloc(1, &outSize, NDInt8, 0, NULL);    

    /* alloc in and scaled pictures */
    inPicture = avcodec_alloc_frame();
    scPicture = avcodec_alloc_frame();
	avpicture_fill((AVPicture *)scPicture,(uint8_t *)scArray->pData,c->pix_fmt,c->width,c->height);
    
    return(asynSuccess);
}

/** Writes single NDArray to the JPEG file.
  * \param[in] pArray Pointer to the NDArray to be written
  */
asynStatus ffmpegFile::writeFile(NDArray *pArray)
{
    static const char *functionName = "writeFile";
    int colorMode = NDColorModeMono;
	int width, height, out_written;
	PixelFormat pix_fmt;
	NDAttribute *pAttribute = NULL;
	
	/* only support 8 bit data for now */
    switch (pArray->dataType) {
        case NDInt8:
        case NDUInt8:
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: only 8-bit data is supported\n",
            driverName2, functionName);
        return(asynError);
    }	
	
    /* We do some special treatment based on colorMode */
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);

    if ((pArray->ndims == 2) && (colorMode == NDColorModeMono)) {
        width  = pArray->dims[0].size;
        height = pArray->dims[1].size;
        pix_fmt = PIX_FMT_GRAY8;
    } else if ((pArray->ndims == 3) && (pArray->dims[0].size == 3) && (colorMode == NDColorModeRGB1)) {
        width  = pArray->dims[1].size;
        height = pArray->dims[2].size;
        pix_fmt = PIX_FMT_RGB24;
    } else {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: unsupported array structure\n",
            driverName2, functionName);
        return(asynError);
    }

	/* If things have changed then setup the swscale ctx */    
	if (height != this->sheight || width != this->swidth || pix_fmt != this->spix_fmt) {
		this->sheight = height;
		this->swidth = width;
		this->spix_fmt = pix_fmt;
		ctx = sws_getCachedContext(ctx, width, height, pix_fmt,
                                   c->width, c->height, c->pix_fmt,
                                   SWS_BICUBIC, NULL, NULL, NULL);   
    }
    
    /* setup the input picture */
    inPicture->data[0] = (uint8_t*) pArray->pData;
    inPicture->linesize[0] = c->width;
    
    /* scale the picture so we can pass it to the encoder */
    sws_scale(ctx, inPicture->data, inPicture->linesize, 0, height,
              scPicture->data, scPicture->linesize);    
   
    /* encode the frame */
    out_written = avcodec_encode_video(c, (uint8_t *)outArray->pData, outSize, scPicture);
    printf("encoding frame (size=%5d)\n", out_written);
    fwrite(outArray->pData, 1, out_written, this->outFile);    
    
    return(asynSuccess);
}

/** Reads single NDArray from a JPEG file; NOT CURRENTLY IMPLEMENTED.
  * \param[in] pArray Pointer to the NDArray to be read
  */
asynStatus ffmpegFile::readFile(NDArray **pArray)
{
    //static const char *functionName = "readFile";

    return asynError;
}


/** Closes the JPEG file. */
asynStatus ffmpegFile::closeFile()
{
    //static const char *functionName = "closeFile";

    /* add sequence end code to have a real mpeg file */
/*    ((uint8_t *)outArray->pData)[0] = 0x00;
    ((uint8_t *)outArray->pData)[1] = 0x00;
    ((uint8_t *)outArray->pData)[2] = 0x01;
    ((uint8_t *)outArray->pData)[3] = 0xb7;
    fwrite((uint8_t *)outArray->pData, 1, 4, this->outFile);*/
    fclose(this->outFile);
    scArray->release();
    outArray->release();

/*    avcodec_close(c);
    av_free(c);
    av_free(inPicture);
    av_free(scPicture);    */
    printf("\n");

    return asynSuccess;
}

/* asynDrvUser interface methods */
/** Sets pasynUser->reason to one of the enum values for the parameters defined for
  * this class if the drvInfo field matches one the strings defined for it.
  * If the parameter is not recognized by this class then calls NDPluginDriver::drvUserCreate.
  * Uses asynPortDriver::drvUserCreateParam.
  * \param[in] pasynUser pasynUser structure that driver modifies
  * \param[in] drvInfo String containing information about what driver function is being referenced
  * \param[out] pptypeName Location in which driver puts a copy of drvInfo.
  * \param[out] psize Location where driver puts size of param 
  * \return Returns asynSuccess if a matching string was found, asynError if not found. */
asynStatus ffmpegFile::drvUserCreate(asynUser *pasynUser,
                                       const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    asynStatus status;
    //const char *functionName = "drvUserCreate";
    
    status = this->drvUserCreateParam(pasynUser, drvInfo, pptypeName, psize, 
                                      ffmpegFileParamString,  NUM_FFMPEG_FILE_PARAMS);

    /* If not, then call the base class method, see if it is known there */
    if (status) status = NDPluginFile::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
    return(status);
}


/** Constructor for ffmpegFile; all parameters are simply passed to NDPluginFile::NDPluginFile.
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
                   NDArrayPort, NDArrayAddr, 1, ffmpegFileLastParam,
                   2, -1, asynGenericPointerMask, asynGenericPointerMask, 
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    //const char *functionName = "ffmpegFile";

    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "ffmpegFile");
    this->supportsMultipleArrays = 1;
}

/* Configuration routine.  Called directly, or from the iocsh  */

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
