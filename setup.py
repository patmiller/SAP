from distutils.core import setup, Extension
setup(name='SAP',
      version='0.1',
      description='Single Assignment Python with IF1',
      author='Pat Miller',
      author_email='patrick.miller@gmail.com',
      url='https://github.com/patmiller/SAP',
      packages=['sap','sap.tests'],
      license='BSD 3-clause',
      ext_modules=[Extension('sap.if1', ['if1.cpp'])],
      )