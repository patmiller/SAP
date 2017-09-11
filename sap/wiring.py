import ast
import inspect
import sys
import types
import os
import sap.if1

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
class TypeMismatch(SemanticError): pass
class NoEffect(SemanticError): pass
class HigherOrderFunction(SemanticError): pass
class UnknownName(SemanticError): pass
class NotAFunction(SemanticError): pass

class IF1Wiring(ast.NodeVisitor):
    def __init__(self,function,compiler):
        self.function = function
        self.path = function.func_globals.get('__file__')
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

    def newnode(self,opcode,N=None):
        node = self.symtab.context.addnode(opcode)
        if N is not None:
            sl = getattr(N,'lineno',None)
            if sl is not None:
                node.pragmas['sl'] = sl
        if self.path is not None:
            node.pragmas['sf'] = os.path.split(self.path)[-1]
        return node

    def visit_Expr(self,node):
        raise NoEffect(self.compiler,node,'Expression has no effect')

    def visit_Call(self,node):
        if not isinstance(node.func,ast.Name) or node.keywords or node.starargs or node.kwargs:
            raise HigherOrderFunction(self.compiler,node,'Cannot use higher order function, named args, or a method here')

        # We build the parameters we pass in first so that we can do some type checking
        values = sum((self.visit(n) for n in node.args),())

        # A node to make the call
        call = self.newnode(self.module.IFCall,node)

        # Look up the function implementation or forward
        fname = node.func.id
        f = self.function.func_globals.get(fname)
        if f is None:
            raise UnknownName(self.compiler,node.func,'Cannot find name {}',fname)

        # OK, we can deal with one of two things now (and eventually a python thing)
        if isinstance(f,sap.if1.Graph):
            # If this is a function graph, we can directly get input and output chains
            call(1) << f
            ftype = f.type
            ichain = ftype.parameter1
            inputs =  ichain.chain() if ichain is not None else ()
            outputs = ftype.parameter2.chain()
        elif isinstance(f,sap.if1.Type):
            if f.code != self.module.IF_Function:
                raise Invalid(self.compiler,self.func,'This does not name a forwarded type')
            call(1).set(fname,f)
            ichain = f.parameter1
            inputs =  ichain.chain() if ichain is not None else ()
            outputs = f.parameter2.chain()
        elif callable(f):
            raise NotImplementedError('Have to do a lookup for things like abs, max, etc.. that are Python functions with well defined meanings')
        else:
            raise NotAFunction(self.compiler,node.func,'{} does not name a callable or a forward',fname)

        # Check the input arity to make sure we are passing the correct number of inputs
        if len(values) != len(inputs):
            raise ArityError(self.compiler,node,'{} expected {} arguments, but got {}',fname,len(inputs),len(values))

        # Check to see that the types match up
        for i,(v,expected) in enumerate(zip(values,inputs)):
            actual = v.type
            if expected is not actual:
                raise TypeMismatch(self.compiler,node.args[i],
                                   'Argument {} expected {}, but actual was {}',
                                   i+1,expected,actual)

        # Now, at last, we can wire the inputs to the call
        for port,arg in enumerate(values):
            call(port+2) << arg

        # Now we just use the output chain to set outports and return those values
        results = []
        for port,out in enumerate(outputs):
            call[port+1] = out
            results.append(call[port+1])
        
        return tuple(results)

    def visit_Compare(self,node):
        a = self.visit(node.left)
        if len(a) != 1:
            raise ArityError(self.compiler,node,'Left operand does not have arity 1')

        allcheck = []
        for port,(op,right) in enumerate(zip(node.ops,node.comparators)):
            b = self.visit(right)
            if len(b) != 1:
                raise ArityError(self.compiler,node,'Right operand does not have arity 1')
            cmp = self.newnode(self.visit(op),op)
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
        all = self.newnode(self.module.IFAnd,node)
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
                if isinstance(value,sap.if1.OutPort):
                    value.pragmas['na'] = name.id
                if name.id in self.symtab:
                    raise SingleAssignment(self.compiler,name,'Single assignment violation for {}',name.id)
                self.symtab[name.id] = value
            return
        
        if len(node.targets) == 1 and isinstance(node.targets[0],ast.Name):
            name = node.targets[0].id
            value = self.visit(node.value)
            if len(value) != 1:
                raise ArityError(self.compiler,node.value,'one value on the LHS, but {} values on the right',len(value))
            if name in self.symtab:
                raise SingleAssignment(self.compiler,node.targets[0],'Single assignment violation for {}',name)
            v = value[0]
            if isinstance(v,sap.if1.OutPort):
                v.pragmas['na'] = name
            self.symtab[name] = value[0]
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
        operation = self.newnode(opcode,n)
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

    @staticmethod
    def compound_callback(compound,symbol,value,level):
        # Here, we are importing something to a subgraph.  We need a new port into the compound
        # for it and then we have to export the value on each subgraph.  Only then can we add it to this level                
        port = len(compound.inputs)+1
        compound(port) << value
        for child in compound.children:
            child[port] = value.type

        return (port,value.type)

    @staticmethod
    def graph_callback(graph,symbol,port_type,level):
        port,type = port_type
        graph[port] = type
        return graph[port]

    def visit_BoolOp(self,node):
        # Implements as a short circuit.  We can likely optimize certain variants to use IFAnd and IFOr directly
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

        ifthen = self.newnode(self.module.IFIfThenElse,node)
        ifthen[1] = self.module.boolean
        with self.symtab.addlevel(ifthen,self.compound_callback):
            # Each value gets a test:body pair (we must booleanize the value)
            for operand in node.values:
                # Add a test graph
                T = ifthen.addgraph()

                # If we have any inputs to the ifthen, they are visible to this graph
                for inedge in ifthen.inputs:
                    T[inedge.port] = inedge.type

                # We add a new context with this graph
                with self.symtab.addlevel(T,graph_callback):
                    v = self.visit(operand)
                    if len(v) > 1:
                        raise ArityError(self.compiler,operand,'test value has arity > 1')
                    b, = self.coerce_boolean(operand,v[0])
                    T(1) << b
                B = ifthen.addgraph()
                B(1) << rvalue

        # If we didn't match, then we use !rvalue as the result (true for and, false for or)
        E = ifthen.addgraph()
        E(1) << (not rvalue)
        return (ifthen[1],)

    def visit_If(self,node):
        # We generate the test in the current context (must have arity 1) and wire it to the ifthen
        v = self.visit(node.test)
        if len(v) != 1:
            raise ArityError(self.compiler,node,'test expression does not have arity-1')

        ifthen = self.newnode(self.module.IFIfThenElse,node)
        ifthen(1) << v[0]
        
        # Visit each set of assignments, but don't wire any final values to the graph yet
        with self.symtab.addlevel(ifthen,self.compound_callback) as compound:
            symtabs = []
            for stmts in (node.body,node.orelse):
                g = ifthen.addgraph()
                for inedge in ifthen.inputs:
                    g[inedge.port] = inedge.type
                with self.symtab.addlevel(g,self.graph_callback) as local:
                    symtabs.append(local)
                    for stmt in stmts:
                        self.visit(stmt)

        truesyms,falsesyms = symtabs

        allsyms = set(self.symtab)
        trueside = set(truesyms).difference(allsyms)
        falseside = set(falsesyms).difference(allsyms)
        names = sorted(trueside.intersection(falseside))

        # Wire values into the bodies of the graphs
        T,F = ifthen.children
        for port,name in enumerate(names):
            true_value = truesyms[name]
            false_value = falsesyms[name]
            T(port+1) << true_value
            F(port+1) << false_value
            if T(port+1).type is not F(port+1).type:
                raise TypeMismatch(self.compiler,node,
                                   '{} does not have consistent type on true ({}) and false ({}) sides of conditional',
                                   name,T(port+1).type,F(port+1).type)
            ifthen[port+1] = T(port+1).type
            self.symtab[name] = ifthen[port+1]
        return

    def visit_Print(self,node):
        peek = self.newnode(self.module.IFPeek,node)
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

    def __iter__(self):
        for n,callback,syms in reversed(self.stack):
            for sym in syms:
                yield sym
        return

    def __exit__(self,*args):
        savepoint = self.marks.pop()
        del self.stack[savepoint-1:]
        return

    def __enter__(self):
        self.marks.append(len(self.stack))
        return self.stack[-1][2]

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

                if tree.sourcefile is not None: R.sf = os.path.split(tree.sourcefile)[-1]
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

    @property
    def if1(self):
        return self.module.if1

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
            visitor = IF1Wiring(f,self)
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

    def forward(self,*args,**kwargs):
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

        return self.module.addtype(self.module.IF_Function,ichain,ochain)

