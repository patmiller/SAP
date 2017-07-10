from distutils.core import setup, Extension
setup(name='SAP',
      version='0.1',
      packages=['sap','sap.tests'],
      license='BSD 3-clause',
      ext_modules=[Extension('sap.if1', ['if1.cpp'])],
      test_suite=['sap.tests.Tests']
      )
