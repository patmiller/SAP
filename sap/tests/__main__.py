from sap.tests.if1_tests import *
from sap.tests.compiler_tests import *
from sap.tests.error_tests import *
from sap.tests.symbol_table_tests import *
from sap.tests.interpreter_tests import *

if __name__ == '__main__':
    import doctest
    doctest.testmod()
    import unittest
    unittest.main()
    
