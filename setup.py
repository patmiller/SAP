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
                    ['type.cpp','module.cpp','graph.cpp','if1.cpp','node.cpp','inport.cpp','outport.cpp'],
                    extra_compile_args=['-std=c++11'],
                    #depends=['IFX.h','module.h','type.h','node.h','graph.h','inport.h','outport.h'],
          ),
      ],
      )
