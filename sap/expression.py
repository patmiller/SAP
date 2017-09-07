import ast

import sap.if1
import sap.compiler_base


class CompilerExpression(sap.compiler_base.CompilerBase):
    def __init__(self):
        sap.compiler_base.CompilerBase.__init__(self)
        return

    def Num(self, node):
        "Num just returns the literal value"
        # This is done as a tuple because, in general,
        # an expression can return an arbitrary number
        # of values
        return (node.n,)

    def NameConstant(self, node):
        if node.value is None:
            self.error.not_supported_error(
                'None is not a supported NameConstant')
        return (node.value,)

    def Name(self, node):
        # ctx is Param(), Load() [rhs], or Store() [lhs]
        if isinstance(node.ctx, ast.Load):
            val = self.symtab[node.id]
            if val is None:
                self.error.symbol_lookup_error(node)
            if val is sap.compiler_base.CompilerBase.placeholder:
                self.error.placeholder_error(node)
            return (val,)
        raise NotImplementedError(node.ctx)

    def Tuple(self, node):
        # concatenate all the tuples for the rhs into one
        return sum((self.visit(x) for x in node.elts), ())

    def Add(self, node):
        return self.graphs[-1].addnode(self.m.IFPlus)

    def Sub(self, node):
        return self.graphs[-1].addnode(self.m.IFMinus)

    def Mult(self, node):
        return self.graphs[-1].addnode(self.m.IFTimes)

    def Div(self, node):
        return self.graphs[-1].addnode(self.m.IFDiv)

    def Mod(self, node):
        return self.graphs[-1].addnode(self.m.IFMod)

    def Pow(self, node):
        return self.graphs[-1].addnode(self.m.IFExp)

    def BinOp(self, node):
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

    def UnaryOp(self, node):
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
        return self.graphs[-1].addnode(self.m.IFOr)

    def And(self, node):
        return self.graphs[-1].addnode(self.m.IFAnd)

    def Eq(self, node):
        return self.graphs[-1].addnode(self.m.IFEqual)

    def NotEq(self, node):
        return self.graphs[-1].addnode(self.m.IFNotEqual)

    def Lt(self, node):
        return self.graphs[-1].addnode(self.m.IFLess)

    def LtE(self, node):
        return self.graphs[-1].addnode(self.m.IFLessEqual)

    def Gt(self, node):
        return self.graphs[-1].addnode(self.m.IFGreat)

    def GtE(self, node):
        return self.graphs[-1].addnode(self.m.IFGreatEqual)

    def Is(self, node):
        self.default(node)
        return

    def IsNot(self, node):
        self.default(node)
        return

    def In(self, node):
        self.default(node)
        return

    def NotIn(self, node):
        self.default(node)
        return

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

    def Call(self, node):
        # TODO: implement call
        self.default(node)
        #N = self.graphs[-1].addnode(self.m.IFCall)
        return

    def BuiltInFunction(self, node):
        # built-in function list: max, min, abs
        self.default(node)
        return
