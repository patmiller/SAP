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
        " Returns the top-level symbol dictionary"
        v = self.stack[-1]
        del self.stack[-1]
        return v

    def __len__(self):
        return sum(map(len(self.stack)))

    def __iter__(self):
        for d in reversed(self.stack):
            for k in d:
                yield k
        return

    def top(self,symbol):
        "Look only in the top level"
        return self.stack[-1].get(symbol)

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

class Compiler(object):
    placeholder = object()
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
        self.filename = f.func_code.co_filename
        baseline = f.func_code.co_firstlineno
        self.graphs = []
        src = inspect.getsource(f)
        
        if src[0].isspace():
            body = ast.parse('if 0:\n'+src).body[0].body[0]
        else:
            body = ast.parse(src).body[0]
        ast.increment_lineno(body,baseline-body.lineno)
        ast.fix_missing_locations(body)
        return self.visit(body,f,types)

    def visit(self,node,*args):
        mname = type(node).__name__
        method = getattr(self,mname,self.default)
        return method(node,*args)

    def default(self,node):
        self.error(node,'not implemented %s'%type(node).__name__)
        return

    def error(self,node,msg):
        f = self.function 
        fname = f.func_name
        lineno = getattr(node,'lineno',None)
        col_offset = getattr(node,'col_offset',0)

        suffix = ''
        if lineno is not None:
            prefix = '\n%s:%d:%s: '%(self.filename,lineno,fname)
            try:
                suffix = '\n'+[x for x in open(self.filename)][lineno-1]
                suffix += ' '*col_offset+'^'
            except: pass
        else:
            prefix = '%s: '%fname
            
        raise SemanticError(prefix+msg+suffix)

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
        if len(node.args.args) != len(types):
            self.error(node,"type info does not match arity of actual args")
        for i,(arg,type) in enumerate(zip(node.args.args,types)):
            # Assign the type to the port and save the
            # name
            port = i+1
            f[port] = type
            f[port].na = arg.id
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

    def Assign(self,node):
        # The lhs is either a single Name or a Tuple of Name
        # Other things are legal python, but are not supported
        # right now
        if len(node.targets) > 1:
            self.error(node,'no chained assignments')
        lhs = []
        target = node.targets[0]
        if isinstance(target,ast.Name):
            targets = node.targets
        elif isinstance(target,ast.Tuple):
            targets = target.elts
        else:
            self.error(node,'invalid lhs')
        for target in targets:
            if not isinstance(target,ast.Name):
                self.error(target,'LHS must be a name')
            name = target.id
            if self.symtab.top(name) is not None:
                self.error(target,'single assignment violation')
            self.symtab[name] = self.placeholder
            lhs.append(name)
            
        rhs = self.visit(node.value)
        if len(lhs) != len(rhs):
            self.error(node,'lhs has arity %d, but rhs has arity %d'%(len(lhs),len(rhs)))
        # Replace the placeholder with the real value
        for sym,val in zip(lhs,rhs):
            self.symtab[sym] = val
            # Set the %na pragma (if possible)
            try: setattr(val,'na',sym)
            except AttributeError: pass
        return lhs

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
            if val is self.placeholder:
                self.error(node,'using a placeholder, not a true value')
            return (val,)
        raise NotImplementedError,node.ctx

    def Tuple(self,node):
        # catenate all the tuples for the rhs into one
        return sum((self.visit(x) for x in node.elts),())

    def Add(self,node):
        return self.graphs[-1].addnode(self.m.IFPlus)

    def Mult(self,node):
        return self.graphs[-1].addnode(self.m.IFTimes)

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
