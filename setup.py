from distutils.core import setup, Extension

setup(name='SAP',
      version='0.1',
      description='Single Assignment Python with IF1',
      author='Pat Miller',
      author_email='patrick.miller@gmail.com',
      url='https://github.com/patmiller/SAP',
      packages=['sap','sap.tests'],
      license='BSD 3-clause',
      ext_modules=[
          Extension('sap.if1',
                    [
                        'nodebase.cpp',
                        'parser.cpp',
                        'inport.cpp',
                        'graph.cpp',
                        'node.cpp',
                        'module.cpp',
                        'type.cpp',
                        'if1.cpp',
                        'outport.cpp',
                    ],
                    extra_compile_args=['-std=c++11'],
                    depends=['IFX.h','module.h','type.h','node.h','graph.h','inport.h','outport.h','parser.h','nodebase.h'],
          ),
      ],
      )
