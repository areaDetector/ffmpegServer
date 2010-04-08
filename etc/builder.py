from iocbuilder import Device, AutoSubstitution
from iocbuilder.arginfo import *

from iocbuilder.modules.areaDetector import NDPluginBase, NDFile

class _ffmpegServer(AutoSubstitution):
    TemplateFile = 'ffmpegServer.db'

class ffmpegServer(Device):
    def __init__(self, MEMORY, HTTP_PORT = 8080, QUEUE = 16, BLOCK = 0, **args):
        # Pass the arguments to the relevant templates
        self.base = NDPluginBase(**filter_dict(args, NDPluginBase.ArgInfo.Names()))    
        self.template = _ffmpegServer(**filter_dict(args, _ffmpegServer.ArgInfo.Names()))        
        # Init the superclass
        self.__super.__init__()
        # Store the args
        self.__dict__.update(self.base.args)    
        self.__dict__.update(self.template.args)
        self.__dict__.update(locals())
        
    # __init__ arguments        
    ArgInfo = _ffmpegServer.ArgInfo +  \
    NDPluginBase.ArgInfo.filtered(without = _ffmpegServer.ArgInfo.Names()) + \
              makeArgInfo(__init__,
        MEMORY    = Simple(
            'Max memory to allocate, should be maxw*maxh*nbuffer for driver'
            ' and all attached plugins', int),    
        HTTP_PORT = Simple('HTTP Port number', int),            
        QUEUE     = Simple('Input array queue size', int),
        BLOCK     = Simple('Blocking callbacks?', int))

    # Device attributes
    LibFileList = ['ffmpegServer']
    DbdFileList = ['ffmpegServer']   

    def InitialiseOnce(self):
        print "ffmpegServerConfigure(%(HTTP_PORT)d)" % self.__dict__                        
                                                                        
    def Initialise(self):
        print 'ffmpegStreamConfigure(' \
            '"%(PORT)s", %(QUEUE)d, %(BLOCK)d, "%(NDARRAY_PORT)s", ' \
            '"%(NDARRAY_ADDR)s", %(MEMORY)d)' % self.__dict__

class _ffmpegFile(AutoSubstitution):
    TemplateFile = 'ffmpegFile.db'

class ffmpegFile(Device):
    def __init__(self, QUEUE = 16, BLOCK = 0, **args):
        # Pass the arguments to the relevant templates
        self.base = NDPluginBase(**filter_dict(args, NDPluginBase.ArgInfo.Names()))   
        self.file = NDFile (**filter_dict(args, NDFile.ArgInfo.Names()))   
        self.template = _ffmpegFile(**filter_dict(args, _ffmpegFile.ArgInfo.Names()))        
        # Init the superclass
        self.__super.__init__()
        # Store the args
        self.__dict__.update(self.base.args)        
        self.__dict__.update(self.file.args)        
        self.__dict__.update(self.template.args)        
        self.__dict__.update(locals())
        
    # __init__ arguments        
    ArgInfo = _ffmpegFile.ArgInfo +  \
    NDFile.ArgInfo.filtered(without = _ffmpegFile.ArgInfo.Names()) + \
    NDPluginBase.ArgInfo.filtered(without = _ffmpegFile.ArgInfo.Names() + NDFile.ArgInfo.Names()) + \
              makeArgInfo(__init__,        
        QUEUE     = Simple('Input array queue size', int),
        BLOCK     = Simple('Blocking callbacks?', int))

    # Device attributes
    LibFileList = ['ffmpegServer']
    DbdFileList = ['ffmpegServer']   

    def Initialise(self):
        print 'ffmpegFileConfigure(' \
            '"%(PORT)s", %(QUEUE)d, %(BLOCK)d, "%(NDARRAY_PORT)s", ' \
            '"%(NDARRAY_ADDR)s")' % self.__dict__
