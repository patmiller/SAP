import ast

import sap.util
import sap.if1
import sap.expression


class Compiler(sap.expression.CompilerExpression):
    def __init__(self):
        sap.compiler_base.CompilerBase.__init__(self)
        return

    def __call__(self, *types):
        for t in types:
            if not isinstance(t, sap.if1.Type):
                raise TypeError('argss must be types')

        def compile_function(f):
            return self.compile(f, types)
        return compile_function

    def compile(self, f, types):
        self.set_base_attr(f)
        baseline = f.func_code.co_firstlineno
        self.graphs = []
        src = sap.util.get_func_code(f)

        body = ast.parse(src).body[0]

        self.func_def = body
        ast.increment_lineno(body, baseline - body.lineno)
        ast.fix_missing_locations(body)
        return self.visit(body, f, types)

    def FunctionDef(self, node, f, types):
        self.symtab.push()

        # Create my top level function (and push on
        # the graph stack).  The top of the graph stack
        # is used to build new nodes
        f = self.m.addfunction(node.name)
        self.graphs.append(f)

        # If well formed, the last statement is a Return
        if (node.body is None) or not isinstance(node.body[-1], ast.Return):
            self.error.not_supported_error(
                node, 'This function is not well formed')
        if node.args.vararg is not None:
            self.error.not_supported_error(
                node, 'SAP functions cannot have varargs')
        if node.args.kwarg is not None:
            self.error.not_supported_error(
                node, 'SAP functions cannot have kwargs')
        if len(node.args.defaults) != 0:
            self.error.not_supported_error(
                node, 'SAP functions cannot have defaults')

        # Load the first level of the symbol table
        if len(types) != len(node.args.args):
            self.error.arity_error(node, len(node.args.args), len(types))

        for i, (arg, type) in enumerate(zip(node.args.args, types)):
            # Assign the type to the port and save the
            # name
            port = i + 1
            f[port] = type
            f[port].na = arg.id
            self.symtab[arg.id] = f[port]

        # Visit the children
        for stmt in node.body[:-1]:
            self.visit(stmt)

        # Visit the expression side of the return
        ret = node.body[-1]
        values = self.visit(ret.value)
        for i, v in enumerate(values):
            f(i + 1) << v

        # Add function to top level of symbol table
        self.symtab.pop()
        fval = ([], [])
        for i in range(len(types)):
            fval[0].append(f[i + 1])
        for i in range(len(values)):
            fval[1].append(f(i + 1))
        self.symtab[f] = (tuple(fval[0]), tuple(fval[1]))

        return f

    def Assign(self, node):
        # The lhs is either a single Name or a Tuple of Name
        # Other things are legal python, but are not supported
        # right now
        if len(node.targets) != 1:
            self.error.arity_error(node, len(node.targets), 1)
        lhs = []
        target = node.targets[0]
        if isinstance(target, ast.Name):
            targets = node.targets
        elif isinstance(target, ast.Tuple):
            targets = target.elts
        else:
            self.error.semantic_error(node, 'invalid lhs')
        for target in targets:
            if not isinstance(target, ast.Name):
                self.error.symbol_name_error(target)
            name = target.id
            if self.symtab.top(name) is not None:
                self.error.single_assignment_error(target)
            self.symtab[name] = sap.compiler_base.CompilerBase.placeholder
            lhs.append(name)

        rhs = self.visit(node.value)
        if len(lhs) != len(rhs):
            self.error.assignment_arity_error(node, lhs, rhs)
        # Replace the placeholder with the real value
        for sym, val in zip(lhs, rhs):
            self.symtab[sym] = val
            # Set the %na pragma (if possible)
            try:
                setattr(val, 'na', sym)
            except AttributeError:
                pass
        return lhs
