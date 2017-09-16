import sap.if1
import unittest
import sap.interpreter
from sap.wiring import DataflowGraph

class TestWiring(unittest.TestCase):
    def setUp(self):
        self.module = DataflowGraph(lineno=False)
        self.interpreter = sap.interpreter.Interpreter()
        return

    def test_struct(self):
        global XYZ
        class XYZ(self.module.struct):
            x = int
            y = float
            z = bool
        self.assertEquals(self.module.module.functions[0].if1,
'''X 18 "XYZ" %sf=wiring_tests.py
E 1 1 0 1 13
N 1 143
E 0 1 1 1 4 %na=x
E 0 2 1 2 3 %na=y
E 0 3 1 3 1 %na=z''')                          

        @self.module(int)
        def f(x):
            return XYZ(x,2.5,True)

        r = self.module.module.interpret(self.interpreter,f,100)[0]
        self.assertEquals((100,2.5,True),(r['x'],r['y'],r['z']))

        @self.module(XYZ)
        def x(r):
            return r.x
        @self.module(XYZ)
        def y(r):
            return r.y
        @self.module(XYZ)
        def z(r):
            return r.z

        self.assertEquals((100,),self.module.module.interpret(self.interpreter,x,r))
        self.assertEquals((2.5,),self.module.module.interpret(self.interpreter,y,r))
        self.assertEquals((True,),self.module.module.interpret(self.interpreter,z,r))
        return

    def test_union(self):
        global U
        class U(self.module.union):
            f = float
            i = int
            c = chr
        functions = {}
        for f in self.module.module.functions:
            functions[f.name] = f
        self.assertEquals(set(functions),set(['U#f','U#i','U#c']))
        self.assertEquals(functions['U#f'].if1,
'''X 16 "U#f"
E 1 1 0 1 13
N 1 143
E 0 1 1 1 3''')
        self.assertEquals(functions['U#i'].if1,
'''X 18 "U#i"
E 1 1 0 1 13
N 1 143
E 0 1 1 2 4''')
        self.assertEquals(functions['U#c'].if1,
'''X 20 "U#c"
E 1 1 0 1 13
N 1 143
E 0 1 1 3 2''')

        @self.module(int)
        def ubuild(x):
            return U(i=x)

        @self.module(U)
        def uget(u):
            return u.i

        u = self.module.module.interpret(self.interpreter,ubuild,100)
        self.assertEquals(((2,100),),u)
        self.assertEqual((100,),
                         self.module.module.interpret(self.interpreter,uget,u[0]))
        return


if 0:
    @module
    def three():
        return 3
    print three.if1
    print module.module.interpret(I,three)
    print

if 0:
    @module(int)
    def printer(x):
        print 'hello',x
        return x+4
    print printer.if1
    print module.module.interpret(I,printer,22)
    print

if 0:
    @module(int)
    def f(x):
        return x+1
    print f.if1
    print module.module.interpret(I,f,2)
    print

if 0:
    @module(int,float)
    def g(x,y):
        return x+y
    print g.if1
    print module.module.interpret(I,g,10,20)
    print

if 0:
    @module(int,float)
    def h(x,y):
        a,b = x+y,3
        return a-b-1
    print h.if1
    print module.module.interpret(I,h)
    print

if 0:
    @module(int)
    def andtest(x):
        return 1 and 2 and 4.5 and x
    print andtest.if1
    print module.module.interpret(I,andtest,35)
    print

if 0:
    @module(int)
    def ortest(x):
        return x or x
    print ortest.if1
    print module.module.interpret(I,ortest)
    print

if 0:
    @module(int,int)
    def comparetest(a,b):
        return (
            a < b < 100,
            a <= b <= 20,
            a == b == 20,
            a > b > 100,
            a >= b >= 100,
            )
    print module.module.interpret(I,comparetest,10,20)
    print module.module.interpret(I,comparetest,20,10)
    print module.module.interpret(I,comparetest,20,20)
    print module.module.interpret(I,comparetest,100,200)
    print

if 0:
    @module(int,int)
    def iftest(x,y):
        if x < y:
            z = x + 1000
        elif x > y:
            z = x + 100000
        else:
            z = x + 100000000
        return z
    print module.module.interpret(I,iftest,10,20)
    print module.module.interpret(I,iftest,100,20)
    print module.module.interpret(I,iftest,10,10)

if 0:
    @module(int)
    def f(x):
        return x + 1
    print module.module.interpret(I,f,10.0)

    @module(int)
    def g(x):
        return f(x*10)
    print module.module.interpret(I,g,10)



if 0:
    fib = module.forward(int,returns=int)
    @module(int)
    def fib(n):
        if n < 2:
            x = 1
        else:
            x = fib(n-1)+fib(n-2)
        return x
    print module.module.interpret(I,fib,10)
    print fib.if1
