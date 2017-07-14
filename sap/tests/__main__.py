"""
>>> from sap.if1 import Module,IF_Basic,IF_Array,IF_Stream,IF_Multiple,IF_Tuple,IF_Function
>>> m = Module()
>>> m
<module>
>>> m.types
[boolean, character, doublereal, integer, null, real, wildbasic, wild, string]
>>> map(int,m.types)
[1, 2, 3, 4, 5, 6, 7, 8, 9]
>>> m.boolean
boolean
>>> m.integer
integer
>>> r = m.addtype(IF_Basic,3) # doesn't make a new type!
>>> r is m.integer
True
>>> m.pragmas['C'] = 'IF1 Check'
>>> m.pragmas['D'] = 'Dataflow ordered'
>>> m.pragmas['F'] = 'Python Frontend'
>>> m.types
[boolean, character, doublereal, integer, null, real, wildbasic, wild, string]
>>> m.addtype(IF_Array,m.integer)
array[integer]
>>> m.addtype(IF_Stream,m.real)
stream[real]
>>> m.addtype(IF_Multiple,m.boolean)
multiple[boolean]
>>> ins = m.addtype(IF_Tuple,m.integer,m.integer)
>>> outs = m.addtype(IF_Tuple,m.real)
>>> m.addtype(IF_Function,ins,outs)
(integer,integer)->(real)
>>> m.addtype(IF_Function,(m.boolean,m.real),(m.real,m.real))
(boolean,real)->(real,real)
>>> m.addtype(IF_Function,m.integer,m.real)
(integer)->(real)
>>> m.addtype(IF_Function,None,m.real)
->(real)
>>> m.addtype(IF_Function,(),m.real)
->(real)
>>> print m.if1
T 1 1 0 %na=boolean
T 2 1 1 %na=character
T 3 1 2 %na=doublereal
T 4 1 3 %na=integer
T 5 1 4 %na=null
T 6 1 5 %na=real
T 7 1 6 %na=wildbasic
T 8 10 %na=wild
T 9 0 2 %na=string
T 10 0 4
T 11 6 6
T 12 4 1
T 13 8 4 0
T 14 8 4 13
T 15 8 6 0
T 16 3 14 15
T 17 8 1 15
T 18 8 6 15
T 19 3 17 18
T 20 3 13 15
T 21 3 0 15
C$  C IF1 Check
C$  D Dataflow ordered
C$  F Python Frontend
<BLANKLINE>
"""

import sap.if1
import unittest

class TestIF1(unittest.TestCase):
    def test_fail(self):
        m = sap.if1.Module()
        g = m.addfunction("main")
        print g,+g

        LiteralTests = {
            'error': [
                't','Trues',            # Bad booleans
                "'aaa'","'","''","'''","'a'x", # Bad char
                '34D+-6',               # Bad doublereal
                '1d+',
                '1d-',
                '123.4d+',
                '123.4d-',
                '123.4D+',
                '123.4D-',
                '345x','++3',           # Bad integer
                'nilly','ni','n',       # Bad null
                '34e+-6','3.0x',        # Bad real
                '1e+',
                '1e-',
                '123.4e+',
                '123.4e-',
                '123.4E+',
                '123.4E-',
                '"hello"x',             # Bad string
                ],
            m.boolean: ['true','false','TRUE','FALSE'],
            m.character: ["'a'",r"'\b'",r"'\\'"],
            m.doublereal: ['1d+33','123.4d6','3d','123D45'],
            m.integer: ["3","12345","+41231","-32123",'12222222222222222'],
            m.null: ['nil','NIL','NiL'],
            m.real: ['123.45','+4.5','3e','123E45','1.5e+33'],
            m.string: ['"hello"','""',r'"hi\b"',r'"I said, \"hi\""'],
        }
        for expected,group in LiteralTests.iteritems():
            for L in group:
                try: T = (g(1) << L)
                except ValueError as e:
                    if expected != 'error': raise
                else:
                    self.assertIs(T,expected)
        return

if __name__ == '__main__':
    m = sap.if1.Module()
    g = m.addfunction("main")
    
    print g
    print g.name
    print g.opcode
    print g.type
    g(1) << "3"
    print g.type
    g(2) << "7.5"
    print g.type
    g(2) << None
    print g.if1
    
    m.pragmas['C'] = 'IF1 Check'
    m.pragmas['D'] = 'Dataflow ordered'
    m.pragmas['F'] = 'Python Frontend'

    print
    print m.if1
    import os; print os.getcwd()
    open('../../three.if1','w').write(m.if1)

    import doctest
    doctest.testmod()
    unittest.main()
    
