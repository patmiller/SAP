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
    def test_three(self):
        # Paul Dubois told me to always test the first program you write in a new language
        # with the "three" program.  In IF1, this is a simple graph with a single literal
        # in-edge.
        m = sap.if1.Module()
        three = m.addfunction("main")
        three(1) << "3"
        self.assertEqual(m.if1,'''T 1 1 0 %na=boolean
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
X 11 "main"
L     0 1 4 "3"''')
        return

    def xtest_simple(self):
        m = sap.if1.Module()

        g = m.addfunction("main")
        g[1] = m.integer
        g[2] = m.integer

        plus = g.addnode(sap.if1.IFPlus)
        plus(1) << g[1]
        plus(2) << g[2]
        plus[1] = m.integer

        g(1) << plus[1]
        self.assertEqual(m.if1,'''T 1 1 0 %na=boolean
T 2 1 1 %na=character
T 3 1 2 %na=doublereal
T 4 1 3 %na=integer
T 5 1 4 %na=null
T 6 1 5 %na=real
T 7 1 6 %na=wildbasic
T 8 10 %na=wild
T 9 0 2 %na=string
T 10 8 4 0
T 11 8 4 10
T 12 3 11 10
X 12 "main"
E 1 1 0 1 4
N 1 141
E 0 1 1 1 4
E 0 2 1 2 4
''')
        return

        
    def test_fail(self):
        m = sap.if1.Module()
        g = m.addfunction("main")

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

    def test_typenames(self):
        m = sap.if1.Module()

        # Get a type name
        self.assertEqual(m.integer.name,'integer')

        # Set a type name
        m.integer.name = 'foobar'
        self.assertEqual(m.integer.name,'foobar')

        # A couple ways to delete a type name
        m.real.name = None
        del m.boolean.name

        self.assertEqual(m.if1,'''T 1 1 0
T 2 1 1 %na=character
T 3 1 2 %na=doublereal
T 4 1 3 %na=foobar
T 5 1 4 %na=null
T 6 1 5
T 7 1 6 %na=wildbasic
T 8 10 %na=wild
T 9 0 2 %na=string
''')
        return

    def test_typepragmas(self):
        m = sap.if1.Module()
        
        # The na and name attributes are linked
        self.assertIs(m.integer.na,m.integer.name)

        # We can add arbitrary 2 character pragmas
        m.integer.xx = 20
        self.assertEqual(m.integer.xx,20)
        m.integer.aa = 1
        m.integer.bb = 2
        m.integer.cc = 3

        # We can delete pragmas (if they exist)
        del m.integer.bb
        with self.assertRaises(AttributeError):
            del m.integer.qq

        # We cannot set attributes with different lengths
        with self.assertRaises(AttributeError):
            m.integer.xxx = 10
        with self.assertRaises(AttributeError):
            m.integer.yyy

        # You can get the whole set of pragmas as a dictionary
        pragmas = m.integer.pragmas
        self.assertIn('aa',pragmas)
        self.assertIn('cc',pragmas)
        self.assertIn('na',pragmas)
        self.assertIn('xx',pragmas)

        # Setting a pragma in this dictionary works (2 characters only)
        pragmas['mm'] = 10
        self.assertEqual(m.integer.mm,10)
        pragmas['aaa'] = 20
        with self.assertRaises(AttributeError):
            m.integer.aaa

        # Pragmas are emitted in lexical order
        self.assertEqual(m.integer.if1,'T 4 1 3 %aa=1 %cc=3 %mm=10 %na=integer %xx=20')
        return

    def xtest_readif1(self):
        sample = '''T 1 1 0 %fo=bar %na=bool %sl=10
T 2 1 3 %na=int
T 3 0 2
T 4 10
T 6 1 4
T 7 8 2 0
T 8 8 2 7
T 9 3 8 7
T 10 3 0 7
'''
        # Order should not be important.  We try a random shuffle of the lines to see if we get the same info
        import random
        scramble = sample.split('\n')
        random.shuffle(scramble)
        scramble = '\n'.join(scramble)
        for example in (sample,scramble):
            m = sap.if1.Module(example)
            # Normal types will be missing
            with self.assertRaises(AttributeError):
                m.integer

            # The Make sure the types look right
            self.assertIs(m.bool,m.types[0])
            self.assertEqual(m.bool.code,sap.if1.IF_Basic)
            self.assertEqual(m.bool.aux,sap.if1.IF_Boolean)

            self.assertIs(m.int,m.types[1])
            self.assertEqual(m.int.code,sap.if1.IF_Basic)
            self.assertEqual(m.int.aux,sap.if1.IF_Integer)

            self.assertEqual(m.types[2].code,sap.if1.IF_Array)
            self.assertIs(m.types[2].parameter1,m.int)

            self.assertEqual(m.types[3].code,sap.if1.IF_Wild)
            self.assertIs(m.types[3].parameter1,None)

            # A skipped type (type 5 is missing above) shows up as a pure wild
            self.assertEqual(m.types[4].code,sap.if1.IF_Wild)
            self.assertIs(m.types[4].parameter1,None)

            self.assertEqual(m.types[5].code, sap.if1.IF_Basic)

            self.assertEqual(m.types[6].code, sap.if1.IF_Tuple)
            self.assertIs(m.types[6].parameter1, m.int)
            self.assertIs(m.types[6].parameter2, None)

            self.assertEqual(m.types[7].code, sap.if1.IF_Tuple)
            self.assertIs(m.types[7].parameter1, m.int)
            self.assertIs(m.types[7].parameter2, m.types[6])

            self.assertEqual(m.types[8].code, sap.if1.IF_Function)
            self.assertIs(m.types[8].parameter1, m.types[7])
            self.assertIs(m.types[8].parameter2, m.types[6])

            self.assertEqual(m.types[9].code, sap.if1.IF_Function)
            self.assertIs(m.types[9].parameter1, None)
            self.assertIs(m.types[9].parameter2, m.types[6])

            # and check some pragmas
            self.assertIsInstance(m.bool.na,str)
            self.assertIs(m.bool.na,m.bool.name)
            self.assertIsInstance(m.bool.fo,str)
            self.assertIsInstance(m.bool.sl,int)

            # If we remove the "extra" wild, the if1 should look like the sample
            self.assertEqual(m.if1.replace('T 5 10\n',''),sample)

        return

    def xtest_nodepragmas(self):
        m = sap.if1.Module()
        g = m.addfunction("main")
        g.sl = 100
        g[1] = m.integer
        g[2] = m.integer
        self.assertIn('sl',g.pragmas)
        self.assertEqual(g.sl,100)

        plus = g.addnode(sap.if1.IFPlus)
        plus.sl = 111
        plus.fn = 'foo.py'
        plus(1) << g[1]
        plus(2) << 100
        plus[1] = m.integer
        self.assertIn('fn',plus.pragmas)
        self.assertEqual(plus.fn,'foo.py')
        self.assertIn('sl',plus.pragmas)
        self.assertEqual(plus.sl,111)

        minus = g.addnode(sap.if1.IFMinus)
        minus[1] = m.integer
        minus(1) << plus[1]
        minus(2) << g[2]

        g(1) << plus[1]
        g(2) << g[1]
        g(3) << g[2]
        g(4) << minus[1]

        self.assertIn('X 14 "main" %sl=100',m.if1)
        self.assertIn('N 1 141 %fn=foo.py %sl=111',m.if1)
        return

    def xtest_edgepragmas(self):
        m = sap.if1.Module()
        g = m.addfunction("main")

        # Decorate some outports
        g[1] = m.integer
        g[1].mk = 'V'
        self.assertEqual(g[1].pragmas,{'mk':'V'})

        g[2] = m.integer
        g[2].na = 'y'
        g[2].mk = 'Q'
        g[2].zz = 'shared'
        self.assertEqual(g[2].pragmas,{'mk':'Q','na':'y','zz':'shared'})

        g[3] = m.integer
        self.assertEqual(g[3].pragmas,{})

        # Decorate some inports
        g(1) << 3
        g(1).na = 'x'
        self.assertEqual(g(1).pragmas,{'na':'x'})

        g(2) << g[1]
        g(2).mk = 'K'
        self.assertEqual(g(2).pragmas,{'mk':'K'})

        g(3) << g[2]
        g(3).mk = 'Z'
        self.assertEqual(g(3).pragmas,{'mk':'Z'})

        g(4) << g[2]
        self.assertEqual(g(4).pragmas,{})

        # When we generate if1, we merge the pragmas from both src and destination (destination is primary)
        self.assertEqual('''X 14 "main"
L     0 1 4 "3" %na=x
E 0 1 0 2 4 %mk=K
E 0 2 0 3 4 %mk=Z %na=y %zz=shared
E 0 2 0 4 4 %mk=Q %na=y %zz=shared
''',g.if1)

        return

    def xtest_outports(self):
        m = sap.if1.Module()
        g = m.addfunction("main")
        g[1] = m.integer
        self.assertIs(g[1].node,g)
        self.assertEqual(g[1].port,1)
        self.assertIs(g[1].type,m.integer)
        self.assertEqual(g[1].pragmas,{})
        self.assertEqual(int(g[1]),1)

        g[2] = m.integer
        self.assertIs(g[2].node,g)
        self.assertEqual(g[2].port,2)
        self.assertIs(g[2].type,m.integer)
        self.assertEqual(g[2].pragmas,{})
        self.assertEqual(int(g[2]),2)

        plus = g.addnode(sap.if1.IFPlus)
        plus[1] = m.integer
        self.assertEqual(int(plus[1]),1)

        plus(1) << g[2]
        self.assertIs(plus(1).node,plus)
        self.assertIs(+plus(1).node,g)
        self.assertEqual(plus(1).port,1)
        self.assertIs(plus(1).type,m.integer)
        self.assertIs(plus(1).literal,None)
        self.assertIs(plus(1).src,g)
        self.assertIs(plus(1).oport,2)
        self.assertEqual(int(plus(1)),1)

        plus(2) << 5
        self.assertIs(plus(2).node,plus)
        self.assertIs(+plus(2).node,g)
        self.assertEqual(plus(2).port,2)
        self.assertIs(plus(2).type,m.integer)
        self.assertEqual(plus(2).literal,'5')
        self.assertIs(plus(2).src,None)
        self.assertEqual(plus(2).oport,0)
        self.assertEqual(int(plus(2)),2)

        g(1) << 3
        self.assertIs(g(1).node,g)
        self.assertIs(+g(1).node,g)
        self.assertEqual(g(1).port,1)
        self.assertIs(g(1).type,m.integer)
        self.assertEqual(g(1).literal,'3')
        self.assertIs(g(1).src,None)
        self.assertEqual(g(1).oport,0)

        g(2) << g[1]
        self.assertIs(g(2).node,g)
        self.assertIs(+g(2).node,g)
        self.assertEqual(g(2).port,2)
        self.assertIs(g(2).type,m.integer)
        self.assertIs(g(2).literal,None)
        self.assertIs(g(2).src,g)
        self.assertEqual(g(2).oport,1)

        g(3) << plus[1]
        self.assertIs(g(3).node,g)
        self.assertIs(+g(3).node,g)
        self.assertEqual(g(3).port,3)
        self.assertIs(g(3).type,m.integer)
        self.assertIs(g(3).literal,None)
        self.assertIs(g(3).src,plus)
        self.assertEqual(g(3).oport,1)

        g(4) << 10
        self.assertIs(g(4).node,g)
        self.assertIs(+g(4).node,g)
        self.assertEqual(g(4).port,4)
        self.assertIs(g(4).type,m.integer)
        self.assertEqual(g(4).literal,"10")
        self.assertIs(g(4).src,None)
        self.assertEqual(g(4).oport,0)

        g(5) << g[1]
        g(6) << g[2]

        self.assertEqual(g[1].edges,[g(2),g(5)])
        self.assertEqual(g[2].edges,[g(6),plus(1)])

        return

    def xtest_outputs(self):
        m = sap.if1.Module()
        g = m.addfunction("main")
        g[1] = m.integer
        g[2] = m.real
        self.assertEqual(g[1],g[1])
        self.assertNotEqual(g[1],g[2])

        self.assertEqual(g.outputs,[g[1],g[2]])
        return
        
    def xtest_inputs(self):
        m = sap.if1.Module()
        g = m.addfunction("main")
        g(1) << 1
        g(2) << 2
        self.assertEqual(g(1),g(1))
        self.assertNotEqual(g(1),g(2))

        self.assertEqual(g.inputs,[g(1),g(2)])
        return

    def test_empty_module(self):
        m = sap.if1.Module()

        self.assertTrue(isinstance(m.types,list))
        self.assertEqual(m.pragmas,{})
        self.assertEqual(m.functions,[])

        self.assertEqual(str(m.boolean),'boolean')
        self.assertEqual(str(m.character),'character')
        self.assertEqual(str(m.doublereal),'doublereal')
        self.assertEqual(str(m.integer),'integer')
        self.assertEqual(str(m.null),'null')
        self.assertEqual(str(m.real),'real')
        self.assertEqual(str(m.wildbasic),'wildbasic')
        self.assertEqual(str(m.wild),'wild')
        self.assertEqual(str(m.string),'string')

        # Type labels
        self.assertEqual(m.boolean.label,1)
        self.assertEqual(map(int,m.types),[1,2,3,4,5,6,7,8,9])
        # Makes some if1 (just for types)
        self.assertEqual(m.if1,'''T 1 1 0 %na=boolean
T 2 1 1 %na=character
T 3 1 2 %na=doublereal
T 4 1 3 %na=integer
T 5 1 4 %na=null
T 6 1 5 %na=real
T 7 1 6 %na=wildbasic
T 8 10 %na=wild
T 9 0 2 %na=string
''')
        return

    def test_module_pragmas(self):
        m = sap.if1.Module()
        m.pragmas['C'] = 'IF1 Check'
        m.pragmas['D'] = 'Dataflow ordered'
        m.pragmas['F'] = 'Python frontend'
        self.assertEqual(m.if1,'''T 1 1 0 %na=boolean
T 2 1 1 %na=character
T 3 1 2 %na=doublereal
T 4 1 3 %na=integer
T 5 1 4 %na=null
T 6 1 5 %na=real
T 7 1 6 %na=wildbasic
T 8 10 %na=wild
T 9 0 2 %na=string
C$  C IF1 Check
C$  D Dataflow ordered
C$  F Python frontend
''')
        return

    def test_noctor_type(self):
        with self.assertRaises(TypeError):
            T = sap.if1.Type()
        return

    def test_type(self):
        m = sap.if1.Module()

        T = m.boolean
        self.assertEqual(T.code,sap.if1.IF_Basic)
        self.assertEqual(T.aux,sap.if1.IF_Boolean)
        self.assertIs(T.parameter1,None)
        self.assertIs(T.parameter2,None)

        self.assertEqual(T.name,'boolean')
        self.assertEqual(T.if1,'T 1 1 0 %na=boolean')

        del T.name
        self.assertIs(T.name,None)
        self.assertEqual(T.if1,'T 1 1 0')

        T.name = 'foo'
        self.assertEqual(T.name,'foo')
        self.assertEqual(T.if1,'T 1 1 0 %na=foo')

        T.name = None
        self.assertIs(T.name,None)
        self.assertEqual(T.if1,'T 1 1 0')

        T.name = 'baz'
        self.assertEqual(T.name,'baz')

        self.assertEqual(T.label,1)
        self.assertEqual(int(T),1)
 
        # Check a parameter1
        self.assertIs(m.string.parameter1,m.character)
        return

    def test_addtype(self):
        m = sap.if1.Module()

        # If we re-add an existing type, we get the same type back
        T = m.addtype(sap.if1.IF_Basic,sap.if1.IF_Integer)
        self.assertIs(T,m.integer)

        # We can delete a type (auto renumbered)
        self.assertIs(m.types[3],m.integer)
        self.assertIn(m.integer,m.types)
        del m.types[3]
        self.assertNotIn(m.integer,m.types)

        # Now, we create one like it and it is inserted
        T = m.addtype(sap.if1.IF_Basic,sap.if1.IF_Integer)
        T.name = 'int'
        self.assertIn(T,m.types)

        # and the IF1 is sensible
        self.assertEqual(m.if1,'''T 1 1 0 %na=boolean
T 2 1 1 %na=character
T 3 1 2 %na=doublereal
T 4 1 4 %na=null
T 5 1 5 %na=real
T 6 1 6 %na=wildbasic
T 7 10 %na=wild
T 8 0 2 %na=string
T 9 1 3 %na=int
''')
        return

    def test_emptymodule(self):
        'Build our own type table from scratch?'
        m = sap.if1.Module('')

        W = m.addtype(sap.if1.IF_Wild,name="crazy")
        self.assertEqual(W.label,1)

        I = m.addtype(sap.if1.IF_Basic,sap.if1.IF_Integer)
        self.assertEqual(I.label,2)
        R = m.addtype(sap.if1.IF_Basic,sap.if1.IF_Real)
        self.assertEqual(R.label,3)

        m.addtype(sap.if1.IF_Array,I)
        m.addtype(sap.if1.IF_Multiple,I)
        m.addtype(sap.if1.IF_Stream,I)

        t0 = m.addtype(sap.if1.IF_Tuple,I)
        t1 = m.addtype(sap.if1.IF_Tuple,I,t0)

        self.assertEqual(m.if1,'''T 1 10 %na=crazy
T 2 1 3
T 3 1 5
T 4 0 2
T 5 4 2
T 6 6 2
T 7 8 2 0
T 8 8 2 7
''')
        return

    def test_typechain(self):
        m = sap.if1.Module('')
        from sap.if1 import IF_Basic,IF_Integer,IF_Real
        from sap.if1 import IF_Tuple,IF_Field,IF_Tag
        I = m.addtype(IF_Basic,IF_Integer,name="int")
        R = m.addtype(IF_Basic,IF_Real,name="real")
        empty = m.addtypechain(code=IF_Tuple)
        self.assertIs(empty,None)

        c1 = m.addtypechain(I,I,R,I,code=IF_Tuple)
        self.assertEqual(str(c1),"int->int->real->int")

        s1 = m.addtypechain(I,I,code=IF_Field)
        self.assertEqual(str(s1),"int->int")

        u1 = m.addtypechain(I,R,code=IF_Tag)
        self.assertEqual(str(u1),"int->real")

        self.assertEqual(m.if1,'''T 1 1 3 %na=int
T 2 1 5 %na=real
T 3 8 1 0
T 4 8 2 3
T 5 8 1 4
T 6 8 1 5
T 7 2 1 0
T 8 2 1 7
T 9 7 2 0
T 10 7 1 9
''')

        self.assertEqual(c1.chain(),(I,I,R,I))
        self.assertEqual(s1.chain(),(I,I))
        self.assertEqual(u1.chain(),(I,R))

        self.assertIs(c1.parameter1,I)
        self.assertEqual(c1.parameter2.chain(),(I,R,I))
        return

    def test_struct_union(self):
        m = sap.if1.Module()
        s = m.addtype(sap.if1.IF_Record,
                      m.addtypechain(m.integer,m.real,code=sap.if1.IF_Field))
        self.assertEqual(str(s),'record[integer->real]')
        self.assertEqual(s.chain(),(m.integer,m.real))
        u = m.addtype(sap.if1.IF_Union,
                      m.addtypechain(m.integer,m.real,code=sap.if1.IF_Tag))
        self.assertEqual(str(u),'union[integer->real]')
        self.assertEqual(u.chain(),(m.integer,m.real))
        return

    def test_chainname(self):
        m = sap.if1.Module()
        c = m.addtypechain(m.integer,m.integer,m.real,code=sap.if1.IF_Tuple,names=('aa','bb','cc'))
        self.assertEqual(str(c),'aa:integer->bb:integer->cc:real')
        return

    def test_functiontype(self):
        m = sap.if1.Module()
        ins = m.addtypechain(m.integer,m.real)
        outs = m.addtypechain(m.real)
        f0 = m.addtype(sap.if1.IF_Function,ins,outs)
        self.assertEqual(str(f0),'function[integer->real returns real]')

        f1 = m.addtype(sap.if1.IF_Function,None,outs)
        self.assertEqual(str(f1),'function[ returns real]')

        return

    def test_opnames_opcodes(self):
        self.assertEqual(sap.if1.opcodes['Plus'],141)
        self.assertEqual(sap.if1.opnames[141],'Plus')
        return

    def test_barefunction(self):
        m = sap.if1.Module()
        f = m.addfunction("main")
        self.assertEqual(m.functions,[f])
        self.assertEqual(str(f),'XGraph')
        self.assertEqual(f.if1,'X 0 "main"')
        return

    def test_simplein(self):
        m = sap.if1.Module()
        f = m.addfunction("main")
        p = f(4)
        self.assertEqual(str(p),'4:XGraph')
        self.assertEqual(p.port,4)
        self.assertEqual(int(p),4)
        self.assertIs(p.literal,None)
        self.assertIs(p.type,None)
        return
        self.assertIs(p.dst,f)
        self.assertIs(p.src,None)
        self.assertIs(p.oport,None)
        self.assertEqual(p.pragmas,{})

        p << 3
        self.assertEqual(p.literal,"3")
        self.assertIs(p.type,m.integer)
        self.assertIs(p.dst,f)
        self.assertIs(p.src,None)
        self.assertIs(p.oport,None)
        self.assertEqual(p.pragmas,{})

        # Same, even on a new selection
        self.assertEqual(f(4).literal,"3")
        self.assertIs(f(4).type,m.integer)
        self.assertIs(f(4).dst,f)
        self.assertIs(f(4).src,None)
        self.assertIs(f(4).oport,None)
        self.assertEqual(f(4).pragmas,{})

        p << "3.5"
        self.assertEqual(p.literal,"3.5")
        self.assertIs(p.type,m.real)
        self.assertIs(p.dst,f)
        self.assertIs(p.src,None)
        self.assertIs(p.oport,None)
        self.assertEqual(p.pragmas,{})

        p << True
        self.assertEqual(p.literal,"true")
        self.assertIs(p.type,m.boolean)

        p << False
        self.assertEqual(p.literal,"false")
        self.assertIs(p.type,m.boolean)

        p << 3.5
        self.assertEqual(p.literal,"3.5d")
        self.assertIs(p.type,m.doublereal)

        p << 3.5e20
        self.assertEqual(p.literal,"3.5d20")
        self.assertIs(p.type,m.doublereal)

        p << 3.5e-20
        self.assertEqual(p.literal,"3.5d-20")
        self.assertIs(p.type,m.doublereal)

        f(4).zz = 'hi'
        self.assertEqual(p.pragmas,{'zz':'hi'})

        return

    def test_outports(self):
        m = sap.if1.Module()
        f = m.addfunction("main")
        f[1] = m.integer
        self.assertIs(f[1].type,m.integer)
        self.assertIs(f[1].node,f)
        self.assertEqual(f[1].port,1)
        self.assertEqual(int(f[1]),1)
        self.assertEqual(str(f[1]),'XGraph:1')
        return

if __name__ == '__main__':
    unittest.main()

    if 0:
        import doctest
        doctest.testmod()
        unittest.main()
    
