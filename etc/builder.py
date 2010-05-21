from iocbuilder import Device, AutoSubstitution
from iocbuilder.arginfo import *

from iocbuilder.modules.areaDetector import AreaDetector, NDPluginBase, _NDFile, _NDFileBase

class FFmpegServer(Device):
    '''Library dependencies for ffmpeg'''
    Dependencies = (AreaDetector,)
    # Device attributes
    LibFileList = ['ffmpegServer']
    DbdFileList = ['ffmpegServer']  
    AutoInstantiate = True	

class _ffmpegServer(AutoSubstitution):
    TemplateFile = 'ffmpegServer.db'

class ffmpegServer(NDPluginBase):
    '''This plugin provides an http server that produces an mjpeg stream'''
    Dependencies = (FFmpegServer,)    
    gda_template = "ffmpegServer"
    def __init__(self, HTTP_PORT = 8080, QUEUE = 16, BLOCK = 0, MEMORY = -1, **args):
        # Init the superclass (NDPluginBase)
        self.__super.__init__(**filter_dict(args, NDPluginBase.ArgInfo.Names()))
        # Init the template class
        self.template = _ffmpegServer(**filter_dict(args, _ffmpegServer.ArgInfo.Names()))
        # Store the args
        self.__dict__.update(self.template.args)
        self.__dict__.update(locals())

    # __init__ arguments
    # NOTE: NDPluginBase comes 2nd so we overwrite NDARRAY_PORT argInfo
    ArgInfo = _ffmpegServer.ArgInfo + NDPluginBase.ArgInfo + makeArgInfo(__init__,
        HTTP_PORT = Simple('HTTP Port number', int),      
        QUEUE     = Simple('Input array queue size', int),
        BLOCK     = Simple('Blocking callbacks?', int),
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
    TemplateFile = 'ffmpegFile.db'
    SubstitutionOverwrites = [_NDFile]

class ffmpegFile(_NDFileBase):
    '''This plugin can compress NDArrays to JPEG and write them to file'''
    Dependencies = (FFmpegServer,)    
    gda_template = "ffmpegFile"
    template_class = _ffmpegFile        
