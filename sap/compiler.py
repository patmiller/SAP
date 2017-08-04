import ast
import inspect

import sap.if1
import sap.compiler_base

class Compiler(sap.compiler_base.CompilerBase):
    placeholder = object()
    def __init__(self):
        sap.compiler_base.CompilerBase.__init__(self)
        return

    def __call__(self,*types):
        for t in types:
            if not isinstance(t, sap.if1.Type):
                raise TypeError('argss must be types')
        def compile_function(f):
            return self.compile(f, types)
        return compile_function

    def compile(self,f,types):
        self.set_base_attr(f)
        baseline = f.func_code.co_firstlineno
        self.graphs = []
        src = inspect.getsource(f)

        if src[0].isspace():
            body = ast.parse('if 0:\n'+src).body[0].body[0]
        else:
            body = ast.parse(src).body[0]

        self.func_def = body
        ast.increment_lineno(body,baseline-body.lineno)
        ast.fix_missing_locations(body)
        return self.visit(body,f,types)

    def FunctionDef(self,node,f,types):
        self.symtab.push()

        # Create my top level function (and push on
        # the graph stack).  The top of the graph stack
        # is used to build new nodes
        f = self.m.addfunction(node.name)
        self.graphs.append(f)

        # If well formed, the last statement is a Return
        if (node.body is None) or not isinstance(node.body[-1], ast.Return):
            self.error.not_supported_error(node, 'This function is not well formed')
        if node.args.vararg is not None:
            self.error.not_supported_error(node, 'SAP functions cannot have varargs')
        if node.args.kwarg is not None:
            self.error.not_supported_error(node, 'SAP functions cannot have kwargs')
        if len(node.args.defaults) != 0:
            self.error.not_supported_error(node, 'SAP functions cannot have defaults')

        # Load the first level of the symbol table
        if len(types) != len(node.args.args):
            self.error.arity_error(node, len(node.args.args), len(types))

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
        if len(node.targets) != 1:
            self.error.arity_error(node, len(node.targets), 1)
        lhs = []
        target = node.targets[0]
        if isinstance(target,ast.Name):
            targets = node.targets
        elif isinstance(target,ast.Tuple):
            targets = target.elts
        else:
            self.error.semantic_error(node,'invalid lhs')
        for target in targets:
            if not isinstance(target, ast.Name):
                self.error.symbol_name_error(target)
            name = target.id
            if self.symtab.top(name) is not None:
                self.error.single_assignment_error(target)
            self.symtab[name] = Compiler.placeholder
            lhs.append(name)

        rhs = self.visit(node.value)
        if len(lhs) != len(rhs):
            self.error.assignment_arity_error(node, lhs, rhs)
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

    def NameConstant(self,node):
        return (node.value,)

    def Name(self,node):
        # ctx is Param(), Load() [rhs], or Store() [lhs]
        if isinstance(node.ctx,ast.Load):
            val = self.symtab[node.id]
            if val is None:
                self.error.symbol_lookup_error(node)
            if val is Compiler.placeholder:
                self.error.placeholder_error(node)
            return (val,)
        raise NotImplementedError,node.ctx

    def Tuple(self,node):
        # catenate all the tuples for the rhs into one
        return sum((self.visit(x) for x in node.elts),())

    def Add(self,node):
        return self.graphs[-1].addnode(self.m.IFPlus)

    def Sub(self, node):
        return self.graphs[-1].addnode(self.m.IFMinus)

    def Mult(self,node):
        return self.graphs[-1].addnode(self.m.IFTimes)

    def Div(self, node):
        return self.graphs[-1].addnode(self.m.IFDiv)

    def Mod(self, node):
        return self.graphs[-1].addnode(self.m.IFMod)

    def BinOp(self,node):
        "The op is just an ast node that will set the if1 node"

        # This still needs some work to deal with type
        # mismatch, etc...
        a = self.visit(node.left)
        b = self.visit(node.right)
        if len(a) != 1:
            self.error.arity_error(node, len(a), 1)
        if len(b) != 1:
            self.error.arity_error(node, len(b), 1)

        # Get a node and wire in the inputs
        N = self.visit(node.op)
        N(1) << a[0]
        N(2) << b[0]
        if N(1).type is not N(2).type:
            self.error.type_error(node)

        # Set the output edge (which is the output value)
        N[1] = N(1).type
        return (N[1],)

    def UAdd(self, node):
        """Just return None for Unary Add since it doesn't do anything"""
        return None

    def USub(self, node):
        return self.graphs[-1].addnode(self.m.IFNeg)

    def Not(self, node):
        return self.graphs[-1].addnode(self.m.IFNot)

    def UnaryOp(self,node):
        """Unary operator"""

        a = self.visit(node.operand)
        if len(a) != 1:
            self.error.arity_error(node, len(a), 1)

        N = self.visit(node.op)
        if N is not None:
            # Not a Unary Plus
            N(1) << a[0]
            N[1].type = N(1).type
            return (N[1],)

        return a

    def Or(self, node):
        raise NotImplementedError('Or')

    def And(self, node):
        raise NotImplementedError('And')

    def Eq(self, node):
        raise NotImplementedError('Eq')

    def NotEq(self, node):
        raise NotImplementedError('NotEq')

    def Lt(self, node):
        raise NotImplementedError('Lt')

    def LtE(self, node):
        raise NotImplementedError('LtE')

    def Gt(self, node):
        raise NotImplementedError('Gt')

    def GtE(self, node):
        raise NotImplementedError('GtE')

    def Is(self, node):
        raise NotImplementedError('Is')

    def IsNot(self, node):
        raise NotImplementedError('IsNot')

    def In(self, node):
        raise NotImplementedError('In')

    def NotIn(self, node):
        raise NotImplementedError('NotIn')

    def BoolOp(self, node):
        # assuming that boolop is basically a binop and thus only has 2 args
        if len(node.values) != 2:
            self.error.arity_error(node, len(node.values), 2)

        a = self.visit(node.values[0])
        b = self.visit(node.values[1])
        if len(a) != 1:
            self.error.arity_error(node.values[0], len(a), 1)
        if a[0].type is not self.boolean:
            self.error.semantic_error(node.values[0], 'type must be a boolean')
        if len(b) != 1:
            self.error.arity_error(node.values[1], len(b), 1)
        if b[0].type is not self.boolean:
            self.error.semantic_error(node.values[1], 'type must be a boolean')

        N = self.visit(node.op)
        N(1) << a[0]
        N(2) << b[0]

        N[1].type = self.boolean

        return (N[1],)

    def Compare(self, node):
        # assume that comparators has only 1 element
        if len(node.comparators) != 1:
            self.error.arity_error(node, len(node.comparators), 1)

        a = self.visit(node.left)
        b = self.visit(node.comparators[0])
        if len(a) != 1:
            self.error.arity_error(node.left, len(a), 1)
        if len(b) != 1:
            self.error.arity_error(node.comparators[0], len(b), 1)

        # assume that ops has only 1 element
        if len(node.ops) != 1:
            self.error.arity_error(node, len(node.ops), 1)

        N = self.visit(node.ops[0])
        N(1) << a[0]
        N(2) << b[0]

        if N(1).type is not N(2).type:
            self.error.type_error(node)

        N[1].type = self.boolean

        return (N[1],)
