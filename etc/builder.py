from iocbuilder import Device, AutoSubstitution, Architecture, Xml
from iocbuilder.arginfo import *

from iocbuilder.modules.areaDetector import AreaDetector, _NDPluginBase, _NDFile, _NDFileBase, _ADBase

class FFmpegServer(Device):
    '''Library dependencies for ffmpeg'''
    Dependencies = (AreaDetector,)
    # Device attributes
    LibFileList = []
    libraries = ['swscale', 'avutil', 'avcodec', 'avformat', 'avdevice']
    if Architecture() == "win32-x86" or Architecture() == "windows-x64":
        LibFileList = libraries
#    else:
#        SysLibFileList = libraries
    LibFileList += ['ffmpegServer']
    DbdFileList = ['ffmpegServer']  
    AutoInstantiate = True    

class _ffmpegStream(AutoSubstitution):
    TemplateFile = 'ffmpegStream.template'

class ffmpegStream(_NDPluginBase):
    '''This plugin provides an http server that produces an mjpeg stream'''
    Dependencies = (FFmpegServer,)    
    _SpecificTemplate = _ffmpegStream
    
    def __init__(self, QUEUE = 2, HTTP_PORT = 8080, MEMORY = -1, Enabled = 1, **args):
        # Init the superclass (_NDPluginBase)
        args["Enabled"] = Enabled
        self.__super.__init__(**args)
        # Store the args
        self.__dict__.update(locals())

    # __init__ arguments
    # NOTE: _NDPluginBase comes 2nd so we overwrite NDARRAY_PORT argInfo
    ArgInfo = _SpecificTemplate.ArgInfo + _NDPluginBase.ArgInfo + makeArgInfo(__init__,        
        Enabled   = Simple('Plugin Enabled at startup?', int),
        QUEUE     = Simple('Input array queue size', int),          
        HTTP_PORT = Simple('HTTP Port number', int),      
        MEMORY  = Simple('Max memory to allocate, should be maxw*maxh*nbuffer '
            'for driver and all attached plugins', int))
            
    def InitialiseOnce(self):
        print "ffmpegServerConfigure(%(HTTP_PORT)d)" % self.__dict__                        
                                                                        
    def Initialise(self):
        print '# ffmpegStreamConfigure(portName, queueSize, blockingCallbacks, '\
            'NDArrayPort, NDArrayAddr, maxMemory)'    
        print 'ffmpegStreamConfigure(' \
            '"%(PORT)s", %(QUEUE)d, %(BLOCK)d, "%(NDARRAY_PORT)s", ' \
            '"%(NDARRAY_ADDR)s", %(MEMORY)d)' % self.__dict__


class _ffmpegFile(AutoSubstitution):
    '''Template containing the records for an NDColorConvert'''
    TemplateFile = 'ffmpegFile.template'
    SubstitutionOverwrites = [_NDFile]

class ffmpegFile(_NDFileBase):
    '''This plugin can compress NDArrays to video and write them to file'''
    Dependencies = (FFmpegServer,)    
    _SpecificTemplate = _ffmpegFile        
    # NOTE: _NDFileBase comes 2nd so we overwrite NDARRAY_PORT argInfo
    ArgInfo = _SpecificTemplate.ArgInfo + _NDFileBase.ArgInfo 
    
class diagnosticPlugins(Xml):
    """This plugin instantiates a standard set of plugins for diagnostic camera:
areaDetector.ROI $(PORTNAME).ROI for taking a region of interest
areaDetector.NDProcess $(PORTNAME).PROC for doing recursive average / clipping the image
areaDetector.NDStats $(PORTNAME).STAT for calculating centroid / profile of the image
areaDetector.NDStdArrays $(PORTNAME).ARR for publishing an EPIC array of video
areaDetector.NDFileHDF5 $(PORTNAME).HDF5 for writing static images
ffmpegServer.ffmpegFile $(PORTNAME).FIMG for writing compressed images and video
areaDetector.NDOverlay $(PORTNAME).OVER for drawing the centroid cross on the image
ffmpegServer.ffmpegStream $(PORTNAME).MJPG for streaming compressed video"""
    TemplateFile = 'diagnosticPlugins.xml'  
diagnosticPlugins.ArgInfo.descriptions["CAM"] = Ident("aravisCamera or firewireDCAM object to connect to", _ADBase)  
