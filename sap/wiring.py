import ast
import inspect
import sys
import types
import sap.if1

class IF1Wiring(ast.NodeVisitor):
    def __init__(self,compiler):
        self.compiler = compiler
        self.symtab = compiler.symtab
        self.reversepolish = []

        self.module = module = compiler.module
        self.boolean = module.boolean
        self.character = module.character
        self.doublereal = module.doublereal
        self.integer = module.integer
        self.null = module.null
        self.real = module.real
        self.string = module.string
        return

    def visit_Compare(self,node):
        a = self.visit(node.left)
        if len(a) != 1:
            raise ArityError(self.compiler,node,'Left operand does not have arity 1')

        allcheck = []
        for port,(op,right) in enumerate(zip(node.ops,node.comparators)):
            b = self.visit(right)
            if len(b) != 1:
                raise ArityError(self.compiler,node,'Right operand does not have arity 1')
            cmp = self.symtab.context.addnode(self.visit(op))
            cmp[1] = self.module.boolean

            allcheck.append(cmp[1])

            left = a[0]
            left_type = self.module.type_of_value(left)
            right = b[0]
            right_type = self.module.type_of_value(right)
            if left_type is right_type:
                cmp(1) << left
                cmp(2) << right
            else:
                _,aa,bb = self.coerce_arithmetic(op,left,right)
                cmp(1) << aa
                cmp(2) << bb

            a = b

        # We need *all* the comparisons to be true
        all = self.symtab.context.addnode(self.module.IFAnd)
        all[1] = self.module.boolean
        for port,v in enumerate(allcheck):
            all(port+1) << v

        return (all[1],)

    def visit_Lt(self,node): return self.module.IFLess
    def visit_LtE(self,node): return self.module.IFLessEqual
    def visit_Eq(self,node): return self.module.IFEqual
    def visit_Gt(self,node): return self.module.IFGreat
    def visit_GtE(self,node): return self.module.IFGreatEqual

    def visit_Str(self,node):
        r = repr(node.s)[1:-1].replace('"','\\%03o'%ord('"'))
        return ('"'+r+'"',)
        '''
        abuild = self.symtab.context.addnode(self.module.IFABuild)
        abuild(1) << 0
        for i,c in enumerate(node.s):
            if c == '\\':
                lit = "'\\'"
            elif ' ' <= c <= '~':
                lit = "'%s'"%c
            else:
                raise TODO(self.compiler,node,'unprintable char')
            abuild(i+2) << lit
        abuild[1] = self.string
        print repr(node.s)
        return (abuild[1],)'''

    def visit_Assign(self,node):
        # Assign(targets=[Tuple(Name(),Name(),...)]) ==> simple assignment
        if (len(node.targets) == 1 and
            isinstance(node.targets[0],ast.Tuple) and
            all(isinstance(x,ast.Name) for x in node.targets[0].elts)):
            names = node.targets[0].elts
            values = self.visit(node.value)
            if len(names) != len(values):
                raise ArityError(self.compiler,node,'{} names in lhs, but rhs has arity {}',len(names),len(values))
            for name,value in zip(names,values):
                print name.id,'=',value
                try:
                    self.symtab[name.id] = value
                except KeyError:
                    raise SingleAssignment(self.compiler,name,'Single assignment violation for {}',name.id)
            return

        raise TODO(self.compiler,node,'record and array update')

    def visit_Tuple(self,node):
        # For a tuple, just merge all the outputs
        if isinstance(node.ctx,ast.Load):
            return sum([self.visit(x) for x in node.elts],())
        raise TODO(self.compiler,node,'non-load tuple')

    def visit_Name(self,node):
        if isinstance(node.ctx,ast.Load):
            v = self.symtab.get(node.id)
            if v is None:
                raise Unknown(self.compiler,node,'Unknown name {}',node.id)
            return (v,)

    def visit_Num(self,node):
        return (node.n,)

    def is_arithmetic(self,T):
        # TODO: Add complex here
        return T in (self.doublereal,self.integer,self.real)

    def coerce_arithmetic(self,n,*args):
        context = self.symtab.context
        try:
            types = [self.module.type_of_value(a) for a in args]
        except RuntimeError:
            raise Arithmetic(self.compiler,n,'non-arithmetic operand')

        if any(not self.is_arithmetic(x) for x in types):
            raise Arithmetic(self.compiler,n,'non-arithmetic operand')
        if any(x is self.doublereal for x in types):
            common = self.doublereal
            cvt = self.module.IFDouble
        elif any(x is self.real for x in types):
            common = self.real
            cvt = self.module.IFSingle
        else:
            common = self.integer
            cvt = None
        T_and_coerced = [common]
        for x,T in zip(args,types):
            if T is not common:
                n = context.addnode(cvt)
                n[1] = common
                n(1) << x
                T_and_coerced.append(n[1])
            else:
                T_and_coerced.append(x)
        return T_and_coerced

    def coerce_boolean(self,n,*args):
        context = self.symtab.context
        try:
            types = [self.module.type_of_value(a) for a in args]
        except RuntimeError:
            raise Arithmetic(self.compiler,n,'non-arithmetic operand')

        results = []
        for x,T in zip(args,types):
            if T is not self.module.boolean:
                n = context.addnode(self.module.IFBool)
                n[1] = self.module.boolean
                n(1) << x
                results.append(n[1])
            else:
                results.append(x)
        return results

    def simple_arithmetic_binop(self,opcode):
        # Pull stuff of the operator stack
        n = self.reversepolish.pop()
        b = self.reversepolish.pop()
        a = self.reversepolish.pop()

        # Coerce to a common type
        common,a,b = self.coerce_arithmetic(n,a,b)

        # Add the appropriate node
        operation = self.symtab.context.addnode(opcode)
        operation[1] = common
        operation(1) << a
        operation(2) << b
        return (operation[1],)

    def visit_Add(self,node):
        return self.simple_arithmetic_binop(self.module.IFPlus)

    def visit_Sub(self,node):
        return self.simple_arithmetic_binop(self.module.IFMinus)

    def visit_Mult(self,node):
        return self.simple_arithmetic_binop(self.module.IFTimes)

    def visit_Div(self,node):
        return self.simple_arithmetic_binop(self.module.IFDiv)

    def visit_And(self,node):
        return self.module.IFAnd

    def visit_Or(self,node):
        return self.module.IFOr

    def visit_BinOp(self,node):
        a = self.visit(node.left)
        if len(a) != 1:
            raise ArityError(self.compiler,node,'Left operand does not have arity 1')
        b = self.visit(node.right)
        if len(a) != 1:
            raise ArityError(self.compiler,node,'Right operand does not have arity 1')

        # Push a and b onto the stack and run the operator
        self.reversepolish.append(a[0])
        self.reversepolish.append(b[0])
        self.reversepolish.append(node)
        return self.visit(node.op)

    def visit_BoolOp(self,node):
        # Implements as a short circuit.  We can likely optimize certain variants to use IFAnd and IFOr directly
        printAst(node)

        # Start with an ifthenelse graph.  Each expression clause in the values becomes a test graph
        # For AND, if we see a false value, the value of the whole shebang is false. Only true if we pass
        # all tests.  We do this by booleanizing each value and then applying a NOT
        if isinstance(node.op,ast.And):
            fix = self.module.IFNot
            rvalue = False
        elif isinstance(node.op,ast.Or):
            fix = None
            rvalue = True
        else:
            raise Unexpected(self.compiler,node,'Unexpected ast.BoolOp structure -- weird op value')

        ifelse = self.symtab.context.addnode(self.module.IFIfThenElse)
        ifelse[1] = self.module.boolean
        def callback(compound,symbol,value,level):
            # Here, we are importing something to a subgraph.  We need a new port into the compound
            # for it and then we have to export the value on each subgraph.  Only then can we add it to this level                
            port = len(compound.inputs)+1
            print 'port',port,symbol,value
            compound(port) << value
            for child in compound.children:
                child[port] = value.type

            return (port,value.type)

        with self.symtab.addlevel(ifelse,callback):
            # Each value gets a test:body pair (we must booleanize the value)
            for operand in node.values:
                # Add a test graph
                T = ifelse.addgraph()

                # If we have any inputs to the ifthen, they are visible to this graph
                for inedge in ifelse.inputs:
                    raise NotImplemented("fix")

                # We add a new context with this graph
                def graphcallback(graph,symbol,port_type,level):
                    port,type = port_type
                    graph[port] = type
                    return graph[port]
                with self.symtab.addlevel(T,graphcallback):
                    v = self.visit(operand)
                    if len(v) > 1:
                        raise ArityError(self.compiler,operand,'test value has arity > 1')
                    b, = self.coerce_boolean(operand,v[0])
                    T(1) << b
                B = ifelse.addgraph()
                B(1) << rvalue

        # If we didn't match, then we use !rvalue as the result (true for and, false for or)
        E = ifelse.addgraph()
        E(1) << (not rvalue)
        return (ifelse[1],)

    def visit_If(self,node):
        # Python converts if T0: ... elif T1: ... else ... into a nested if... de-nest if possible
        #printAst(node)
        testbodypairs = [(node.test, node.body)]
        elses = node.orelse
        while len(elses) == 1 and isinstance(elses[0],ast.If):
            testbodypairs.append((elses[0].test,elses[0].body))
            elses = elses[0].orelse
        for t,b in testbodypairs:
            printAst(t)
            print '--'
            printAst(b)
            print
        print '----'
        printAst(elses)
        xx

    def visit_Print(self,node):
        peek = self.symtab.context.addnode(self.module.IFPeek)
        elements = sum((self.visit(child) for child in node.values),())
        spaced = sum(((v,"' '") for v in elements[:-1]),())
        if elements:
            spaced += (elements[-1],)
        if node.nl:
            spaced += (r"'\n'",)
        port = 0
        for v in spaced:
            port += 1
            peek(port) << v
        return ()

    def generic_visit(self,node):
        raise NotSupported(self.compiler,node,'Python ast node {} not supported yet',type(node).__name__)

class SemanticError(Exception):
    def __init__(self,compiler,node,format,*args):
        lineno = getattr(node,'lineno',None)
        if lineno is None:
            msg = format.format(*args)
        else:
            msg = '\n%s:%d: %s\n%s%s'%(compiler.sourcefile,
                                      node.lineno,
                                      format.format(*args),
                                      compiler.source[node.lineno-1],
                                      node.col_offset*' '+'^')
        super(SemanticError,self).__init__(msg)
        return

class SingleAssignment(SemanticError): pass
class NotSupported(SemanticError): pass
class ArityError(SemanticError): pass
class TODO(SemanticError): pass
class Arithmetic(SemanticError): pass
class Unknown(SemanticError): pass


def printAst(n, level=0, stream=sys.stdout,label=''):
    "simple ast pretty printer"
    if isinstance(n,ast.AST):
        s = ast.dump(n)
        if len(s) < 30:
            stream.write(level*'  '+label+s+'\n')
            return
        stream.write(level*'  '+type(n).__name__+'\n')
        for attr in n._fields:
            printAst(getattr(n,attr),level+1,stream,attr+'=')
    elif isinstance(n,list):
        stream.write(level*'  '+label+'[\n')
        for x in n:
            printAst(x,level+1,stream)
        stream.write(level*'  '+']\n')
    elif isinstance(n,tuple):
        stream.write(level*'  '+label+'(\n')
        for x in n:
            printAst(x,level+1,stream)
        stream.write(level*'  '+')\n')
    else:
        stream.write(level*'  '+label+repr(n)+'\n')
    return

def getAst(o):
    "Get ast for an object (even if indented) and make sure line numbers are right)"
    _,lineno = inspect.findsource(o)
    src = inspect.getsource(o)
    if src[:1].isspace():
        T = ast.parse('if 0:\n'+src).body[0].body[0]
        adjust = -1
    else:
        T = ast.parse(src).body[0]
        adjust = 0
    ast.increment_lineno(T,n=lineno+adjust)
    T.lineno += 1  # Best effort to skip the @decorator line
    T.sourcefile = inspect.getsourcefile(o)
    return T

class Meta(type):
    def __new__(cls,name,bases,dct):
        T = super(Meta, cls).__new__(cls, name, bases, dct)
        if '__metaclass__' not in dct: return T.create(name)
        return T

class Symtab(object):
    def __init__(self):
        self.stack = []
        self.addlevel(None,(lambda key,value,level: value))
        self.marks = []
        return

    def __exit__(self,*args):
        savepoint = self.marks.pop()
        del self.stack[savepoint-1:]
        return

    def __enter__(self):
        self.marks.append(len(self.stack))
        return self

    def addlevel(self,n,callback):
        self.stack.append((n,callback,{}))
        return self

    def get(self,key):
        callbacks = []
        for n,cb,level in reversed(self.stack):
            v = level.get(key)
            if v is not None: break
            callbacks.append((n,cb,level))
        else:
            return None
        # We may have to do work to bring the value up to this level
        if callbacks: print 'CB',callbacks
        for n,cb,level in reversed(callbacks):
            v = cb(n,key,v,level)
            level[key] = v
        return v

    def __repr__(self):
        return repr(self.stack)

    def __setitem__(self,key,v):
        if key in self.stack[-1][2]:
            raise KeyError("Trying to override key {}".format(key))
        self.stack[-1][2][key] = v
        return

    @property
    def context(self):
        return self.stack[-1][0]

class DataflowGraph(object):
    def __init__(self,*args):
        m = self.module = sap.if1.Module()
        self.shortcuts = {
            bool : m.boolean,
            chr : m.character,
            float : m.doublereal,
            int : m.integer,
            str : m.string,
            }
        self.integer = m.integer
        self.boolean = m.boolean
        self.character = m.character
        self.real = m.real
        self.doublereal = m.doublereal
        self.string = m.string

        self.forwards = {}
        self.symtab = Symtab()

        class struct(object):
            __metaclass__ = Meta
            LINK = self.module.IF_Field
            FLAVOR = self.module.IF_Record

            @classmethod
            def create(cls,name):
                # Grab the ast and we'll use that to rebuild the type
                tree = getAst(cls)

                # Get the field names IN ORDER OF DEFINITION
                fieldnames = [n.targets[0].id for n in tree.body if isinstance(n,ast.Assign)]
                assert fieldnames

                # We build up a chain type for either a record or union
                types = [(fieldname,self.typeof(getattr(cls,fieldname))) for fieldname in reversed(fieldnames)]
                fields = None
                for fieldname,fieldtype in types:
                    fields = m.addtype(cls.LINK,fieldtype,fields,name=fieldname)
                R = m.addtype(cls.FLAVOR,fields,name=name)

                if tree.sourcefile is not None: R.sf = tree.sourcefile
                R.li = tree.lineno

                # We build c'tor function (or functions) as well
                cls.ctor(R)
                return R

            @classmethod
            def ctor(cls,R):
                ctor = m.addfunction(R.name)
                ctor.sf = R.sf
                ctor.li = R.li
                chain = R.parameter1
                N = ctor.addnode(m.IFRBuild)
                N[1] = R
                port = 1
                while chain:
                    ctor[port] = chain.parameter1
                    N(port) << ctor[port]
                    ctor[port].na = chain.name

                    chain = chain.parameter2
                    port += 1
                ctor(1) << N[1]
                self.symtab[R.name] = ctor
                return

        self.struct = struct

        class union(struct):
            __metaclass__ = Meta
            LINK = self.module.IF_Tag
            FLAVOR = self.module.IF_Union

            @classmethod
            def ctor(cls,R):
                chain = R.parameter1
                port = 1
                while chain:
                    fname = R.name+'#'+chain.name
                    ctor = m.addfunction(fname)
                    self.symtab[fname] = ctor

                    ctor[1] = chain.parameter1
                    N = ctor.addnode(m.IFRBuild)
                    N(port) << ctor[1]
                    N[1] = R
                    ctor(1) << N[1]
                    chain = chain.parameter2
                    port += 1
                return

        self.union = union

        return

    def typeof(self,x):
        if isinstance(x,sap.if1.Type):
            return x
        # Maybe we have a direct shortcut for this
        try:
            T = self.shortcuts.get(x)
        except TypeError:
            T = None
        if T is not None: return T

        if isinstance(x,list) and len(x) == 1:
            return self.array(x[0])

        raise NotImplementedError("Cannot type-ify {}".format(x))


    def __call__(self,*inputs,**state):
        def decorator(f):
            return self.compile(f,inputs,state)
        if len(inputs) == 1 and not state and isinstance(inputs[0],types.FunctionType):
            f = inputs[0]
            inputs = ()
            return decorator(f)
        return decorator

    def compile(self,f,inputs,state):
        if state: return self.reduction(f,inputs,state)

        # Get the Python ast and some sourcefile info
        tree = getAst(f)
        self.sourcefile = inspect.getsourcefile(f)
        self.source,_ = inspect.findsource(f)

        # See if this name exists already
        v = self.symtab.get(f.func_name)
        if v is not None: raise SingleAssignment(self,tree,'Single assignment violated for name {}',f.func_name)
        if tree.args.vararg is not None:
            raise SemanticError(self,tree,'varargs are not meaningful here')
        if tree.args.kwarg is not None:
            raise SemanticError(self,tree,'keywords are not meaningful here')
        if tree.args.defaults:
            raise NotSupported(self,tree,'default arguments are not supported')
        if len(tree.args.args) != len(inputs):
            raise SemanticError(self,tree,'function has {} parameters, but {} were described in decorator',
                                len(tree.args.args),
                                len(inputs))

        # Add a function graph to the if1 module
        graph = self.module.addfunction(f.func_name)
        self.symtab[f.func_name] = graph

        # Open up a new local symbol table level and fill it with argument names
        # These names may mask function names... that's ok
        def using_function(n,symbol,value,level):
            "just promoting global symbol literal values up the chain"
            level[symbol] = value
            return value

        # If we fail, we must restore the symbol table back to the original state
        with self.symtab.addlevel(graph,using_function):

            for port,name,type in zip(range(1,len(inputs)+1),tree.args.args,inputs):
                graph[port] = self.typeof(type)
                self.symtab[name.id] = graph[port]

            # Make sure the body is well formed
            if not tree.body or not isinstance(tree.body[-1],ast.Return):
                raise SemanticError(self,tree.body[-1] if tree.body else tree,'Function body must end with a return')

            # Generate the tree to make some values
            visitor = IF1Wiring(self)
            for node in tree.body[:-1]:
                visitor.visit(node)
            results = visitor.visit(tree.body[-1].value)

            for port,result in enumerate(results):
                graph(port+1) << result

        return graph

    def reduction(self,f,inputs,state):
        raise NotImplementedError("Cannot do reductions yet")

    def array(self,base):
        base = self.typeof(base)
        return self.module.addtype(self.module.IF_Array,base)

    def stream(self,base):
        base = self.typeof(base)
        return self.module.addtype(self.module.IF_Stream,base)

    def multiple(self,base):
        base = self.typeof(base)
        return self.module.addtype(self.module.IF_Multiple,base)

    def forward(self,name,*args,**kwargs):
        # name cannot already be set by another forward nor function
        if name in self.forwards: raise KeyError("duplicate forward {}",name)
        if self.symtab.get(name) is not None: raise KeyError("forward {} would shadow existing name",name)

        inputs = [self.typeof(x) for x in args]
        ichain = None
        for T in reversed(inputs):
            ichain = self.module.addtype(self.module.IF_Tuple,T,ichain)

        returns = kwargs.pop('returns')
        if isinstance(returns,tuple):
            outputs = [self.typeof(x) for x in returns]
        else:
            outputs = [self.typeof(returns)]
        ochain = None
        for T in reversed(outputs):
            ochain = self.module.addtype(self.module.IF_Tuple,T,ochain)

        result = self.forwards[name] = self.module.addtype(self.module.IF_Function,ichain,ochain)
        return result


module = DataflowGraph()

if 1:
    class XYZ(module.struct):
        x = int
        y = float
        z = bool

#fib = module.forward('fib',int,returns=int)

if 0:
    class U(module.union):
        a = float
        c = int
        d = chr

#@module(int)
def fib(n):
    if n < 2:
        x = 1
    else:
        x = fib(n-1)+fib(n-2)
    return x

import sap.interpreter
I = sap.interpreter.Interpreter()
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
    print module.module.interpret(I,g)
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

if 1:
    @module(int,int)
    def iftest(x,y):
        if x < y:
            z = 100
        else:
            if x > y:
                z = 200
            else:
                z = 0
        return z

print module.forwards
print module.symtab

