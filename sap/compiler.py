import ast
import inspect
import collections

import sap.if1

class SemanticError(Exception):
    pass

class SymbolTable(collections.MutableMapping):
    def __init__(self):
        self.stack = []
        return

    def push(self):
        d = {}
        self.stack.append(d)
        return d

    def pop(self):
        del self.stack[-1]

    def __len__(self):
        return sum(map(len(self.stack)))

    def __iter__(self):
        for d in reversed(self.stack):
            for k in d:
                yield k
        return

    def __getitem__(self,symbol):
        for d in reversed(self.stack):
            v = d.get(symbol)
            if v is not None: return v
        return None

    def __setitem__(self,symbol,value):
        self.stack[-1][symbol] = value
        return
    
    def __delitem__(self,symbol):
        return

missing = object()
class Compiler(object):
    def __init__(self):
        self.m = sap.if1.Module()
        self.symtab = SymbolTable()
        for type in ('boolean','character','doublereal',
                     'integer','null','real','wild'):
            setattr(self,type,getattr(self.m,type))
        return

    def __call__(self,*types):
        for t in types:
            if not isinstance(t,sap.if1.Type):
                raise TypeError("argss must be types")
        def compile_function(f):
            return self.compile(f,types)
        return compile_function

    def compile(self,f,types):
        self.function = f
        self.graphs = []
        src = inspect.getsource(f)
        
        if src[0].isspace():
            body = ast.parse('if 0:\n'+src).body[0].body[0]
        else:
            body = ast.parse(src).body[0]
        return self.visit(body,f,types)

    def visit(self,node,*args):
        mname = type(node).__name__
        method = getattr(self,mname,missing)
        if method is missing:
            raise NotImplementedError(mname)
        return method(node,*args)

    def error(self,node,msg):
        f = self.function 
        fname = f.func_name
        filename = f.func_globals.get('__file__').replace('.pyc','.py')
        raise SemanticError('\n%s:%d:%s: %s'%(
            filename,
            node.lineno,
            fname,
            msg))

    def FunctionDef(self,node,f,types):
        self.symtab.push()

        # Create my top level function (and push on
        # the graph stack).  The top of the graph stack
        # is used to build new nodes
        f = self.m.addfunction(node.name)
        self.graphs.append(f)

        # If well formed, the last statement is a Return
        assert node.body and isinstance(node.body[-1],ast.Return)
        assert node.args.vararg is None
        assert node.args.kwarg is None
        assert node.args.defaults == []

        # Load the first level of the symbol table
        assert len(node.args.args) == len(types)
        for i,(arg,type) in enumerate(zip(node.args.args,types)):
            # Assign the type to the port and save the
            # name
            port = i+1
            f[port] = type
            self.symtab[arg.id] = f[port]
        
        # Visit the children
        for stmt in node.body[:-1]:
            self.visit(stmt)

        # Visit the expression side of the return
        ret = node.body[-1]
        values = self.visit(ret.value)
        for i,v in enumerate(values):
            f(i+1) << v
        return f

    def Num(self,node):
        "Num just returns the literal value"
        # This is done as a tuple because, in general,
        # an expression can return an arbitrary number
        # of values
        return (node.n,)

    def Name(self,node):
        # ctx is Param(), Load() [rhs], or Store() [lhs]
        if isinstance(node.ctx,ast.Load):
            val = self.symtab[node.id]
            if val is None:
                self.error(node,'missing name %s'%node.id)
            return (val,)
        raise NotImplementedError,node.ctx

    def Add(self,node):
        "Add just returns an IFPlus"
        return self.graphs[-1].addnode(self.m.IFPlus)

    def BinOp(self,node):
        "The op is just an ast node that will set the if1 node"
        
        # This still needs some work to deal with type
        # mismatch, etc...
        a = self.visit(node.left)
        b = self.visit(node.right)
        assert len(a) == 1,'left operand must have arity 1'
        assert len(b) == 1,'right operand must have arity 1'

        # Get a node and wire in the inputs
        N = self.visit(node.op)
        N(1) << a[0]
        N(2) << b[0]
        if N(1).type is not N(2).type:
            self.error(node,'binop types must match')

        # Set the output edge (which is the output value)
        N[1] = N(1).type
        return (N[1],)

# This makes a basic compiler.  We can have multiple compilers
# if we choose
SAP = Compiler()
