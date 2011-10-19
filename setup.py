from distutils.core import setup, Extension
 
httpmodule = Extension('httpget', sources = ['http.c'])
 
setup (name = 'HTTP Head',
        version = '1.0',
        description = 'Dum dum',
        ext_modules = [httpmodule])