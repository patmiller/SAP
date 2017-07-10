"""
>>> from sap.if1 import Module,IF_Basic,IF_Array,IF_Stream,IF_Multiple,IF_Tuple,IF_Function
>>> m = Module()
>>> m
<module>
>>> m.types
[boolean, character, doublereal, integer, null, real, wildbasic, wild]
>>> map(int,m.types)
[1, 2, 3, 4, 5, 6, 7, 8]
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
[boolean, character, doublereal, integer, null, real, wildbasic, wild]
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
>>> for x in m.if1: print x
T 1 1 0 %na=boolean
T 2 1 1 %na=character
T 3 1 2 %na=doublereal
T 4 1 3 %na=integer
T 5 1 4 %na=null
T 6 1 5 %na=real
T 7 1 6 %na=wildbasic
T 8 10 %na=wild
T 9 0 4
T 10 6 6
T 11 4 1
T 12 8 4 0
T 13 8 4 12
T 14 8 6 0
T 15 3 13 14
T 16 8 1 14
T 17 8 6 14
T 18 3 16 17
T 19 3 12 14
T 20 3 0 14
C$  C IF1 Check
C$  D Dataflow ordered
C$  F Python Frontend
"""

import sap.if1

if __name__ == '__main__':
    print sap.if1.opcodes

    m = sap.if1.Module()
    g = m.addfunction("main")
    
    print g(1)

    import doctest
    doctest.testmod()

if 0:
    print 'function',m.addtype(IF_Function,ins,outs)
    print 'function',m.addtype(IF_Function,None,outs)
    print 'function',m.addtype(IF_Function,(m.integer,m.integer),(m.boolean,))
    print 'function',m.addtype(IF_Function,(),(m.boolean,))
    print 'function',m.addtype(IF_Function,m.integer,m.boolean)
    
