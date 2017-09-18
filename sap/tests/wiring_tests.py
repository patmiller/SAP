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


    def test_three(self):
        @self.module
        def three():
            return 3
        self.assertEqual((3,),
                         self.module.module.interpret(self.interpreter,three))
        return

    class capture(object):
        def __enter__(self):
            from cStringIO import StringIO
            import sys
            global stdout
            self.stdout = sys.stdout
            self.out = None
            sys.stdout = StringIO()
            return self

        def __exit__(self,a,b,c):
            import sys
            io = sys.stdout
            sys.stdout = self.stdout
            self.out = io.getvalue()
            return
            

    def test_printer(self):
        @self.module(int)
        def printer(x):
            print 'hello',x
            return x+4
        with self.capture() as io:
            x = self.module.module.interpret(self.interpreter,printer,22)
        self.assertEqual((26,),x)
        self.assertEqual(io.out,'hello 22\n')
        return

    def test_plus(self):
        @self.module(int)
        def f(x):
            return x+1
        self.assertEqual((3,),
                         self.module.module.interpret(self.interpreter,f,2))
        return

    def test_mixed(self):
        @self.module(int,float)
        def g(x,y):
            return x+y
        self.assertEquals((30,),self.module.module.interpret(self.interpreter,g,10,20))
        return

    def test_(self):
        @self.module(int,float)
        def h(x,y):
            a,b = x+y,3
            return a-b-1
        self.assertEquals((26.5,),self.module.module.interpret(self.interpreter,h,10,20.5))
        return

    def test_and(self):
        @self.module(int,int,int)
        def andtest(x,y,z):
            return x and y and z

        self.assertEquals((False,),self.module.module.interpret(self.interpreter,andtest,0,0,0))
        self.assertEquals((False,),self.module.module.interpret(self.interpreter,andtest,0,0,1))
        self.assertEquals((False,),self.module.module.interpret(self.interpreter,andtest,0,1,0))
        self.assertEquals((False,),self.module.module.interpret(self.interpreter,andtest,0,1,1))
        self.assertEquals((False,),self.module.module.interpret(self.interpreter,andtest,1,0,0))
        self.assertEquals((False,),self.module.module.interpret(self.interpreter,andtest,1,0,1))
        self.assertEquals((False,),self.module.module.interpret(self.interpreter,andtest,1,1,0))
        self.assertEquals((True,),self.module.module.interpret(self.interpreter,andtest,1,1,1))
        return

    def test_or(self):
        @self.module(int,int,int)
        def ortest(x,y,z):
            return x or y or z
        self.assertEquals((False,),self.module.module.interpret(self.interpreter,ortest,0,0,0))
        self.assertEquals((True,),self.module.module.interpret(self.interpreter,ortest,0,0,1))
        self.assertEquals((True,),self.module.module.interpret(self.interpreter,ortest,0,1,0))
        self.assertEquals((True,),self.module.module.interpret(self.interpreter,ortest,0,1,1))
        self.assertEquals((True,),self.module.module.interpret(self.interpreter,ortest,1,0,0))
        self.assertEquals((True,),self.module.module.interpret(self.interpreter,ortest,1,0,1))
        self.assertEquals((True,),self.module.module.interpret(self.interpreter,ortest,1,1,0))
        self.assertEquals((True,),self.module.module.interpret(self.interpreter,ortest,1,1,1))
        return

    def test_compares(self):
        @self.module(int,int)
        def comparetest(a,b):
            return (
                a < b < 100,
                a <= b <= 20,
                a == b == 20,
                a > b > 100,
                a >= b >= 100,
                )
        self.assertEquals((True,True,False,False,False,),
                          self.module.module.interpret(self.interpreter,comparetest,10,20))
        self.assertEquals((False,False,False,False,False,),
                          self.module.module.interpret(self.interpreter,comparetest,20,10))
        self.assertEquals((False,True,True,False,False,),
                          self.module.module.interpret(self.interpreter,comparetest,20,20))
        self.assertEquals((False,False,False,False,False,),
                          self.module.module.interpret(self.interpreter,comparetest,100,200))
        return

    def test_if_elif(self):
        @self.module(int,int)
        def iftest(x,y):
            if x < y:
                z = x + 1000
            elif x > y:
                z = x + 100000
            else:
                z = x + 100000000
            return z
        self.assertEquals((1010,),self.module.module.interpret(self.interpreter,iftest,10,20))
        self.assertEquals((100100,),self.module.module.interpret(self.interpreter,iftest,100,20))
        self.assertEquals((100000010,),self.module.module.interpret(self.interpreter,iftest,10,10))
        return

    def test_simple_call(self):
        global f,g

        @self.module(int)
        def f(x):
            return x + 1
        self.assertEquals((11,),self.module.module.interpret(self.interpreter,f,10))

        @self.module(int)
        def g(x):
            return f(x*10)
        self.assertEquals((101,),self.module.module.interpret(self.interpreter,g,10))
        return

    def test_fib(self):
        global fib
        fib = self.module.forward(int,returns=int)
        @self.module(int)
        def fib(n):
            if n < 2:
                x = 1
            else:
                x = fib(n-1)+fib(n-2)
            return x
        self.assertEquals((89,),self.module.module.interpret(self.interpreter,fib,10))
        return
