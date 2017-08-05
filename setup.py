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
                        'src/nodebase.cpp',
                        'src/parser.cpp',
                        'src/inport.cpp',
                        'src/graph.cpp',
                        'src/node.cpp',
                        'src/module.cpp',
                        'src/type.cpp',
                        'src/if1.cpp',
                        'src/outport.cpp',
                    ],
                    extra_compile_args=['-std=c++11'],
                    depends=['src/IFX.h','src/module.h','src/type.h','src/node.h','src/graph.h','src/inport.h','src/outport.h','src/parser.h','src/nodebase.h'],
          ),
      ],
      )
