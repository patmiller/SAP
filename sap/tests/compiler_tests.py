import unittest

from sap.compiler import Compiler
from sap.error import *

class TestCompiler(unittest.TestCase):
    def test_error(self):
        SAP = Compiler()
        with self.assertRaises(SemanticError) as e:
            @SAP()
            def three():
                return x
        self.assertIn('Name x not found in symbol table',e.exception.message)
        return


    def test_three(self):
        SAP = Compiler()
        @SAP()
        def three():
            return 3
        self.assertEqual(three.if1,'X 11 "three"\nL     0 1 4 "3"')

        self.assertEqual(SAP.m.if1,'''T 1 1 0 %na=boolean
T 2 1 1 %na=character
T 3 1 2 %na=doublereal
T 4 1 3 %na=integer
T 5 1 4 %na=null
T 6 1 5 %na=real
T 7 1 6 %na=wildbasic
T 8 10 %na=wild
T 9 0 2 %na=string
T 10 8 4 0
T 11 3 0 10
X 11 "three"
L     0 1 4 "3"''')
        return

    def test_plus1(self):
        SAP = Compiler()
        @SAP(SAP.integer)
        def plus1(x):
            return x+1
        self.assertEqual(plus1.if1,'''X 11 "plus1"
E 1 1 0 1 4
N 1 141
E 0 1 1 1 4 %na=x
L     1 2 4 "1"''')
        return

    
    def test_assign(self):
        SAP = Compiler()
        @SAP(SAP.integer)
        def f(x):
            y,z = x*10,20+2
            return y+z,z
        self.assertEqual(f.if1,'''X 12 "f"
E 3 1 0 1 4
E 2 1 0 2 4 %na=z
N 1 152
E 0 1 1 1 4 %na=x
L     1 2 4 "10"
N 2 141
L     2 1 4 "20"
L     2 2 4 "2"
N 3 141
E 1 1 3 1 4 %na=y
E 2 1 3 2 4 %na=z''')
        return
