# TODO: better error statements (maybe create a function kind of like yyerror() to pass in a string to so that line numbers can be used?)
# TODO: for all add_var statements, actually add some var_value
# TODO: add types
# TODO: make cg_* return (node, outport)
# input ()
# output []
# input for function []
# output for function ()
import inspect, ast
import sap.if1

class SymbolTable(object):
    def __init__(self):
        self.st = [dict()] # have a symbol table with a top level scope
        return

    def push(self):
        self.st.append(dict())
        return

    def get_level(self, var_name):
        for i in reversed(xrange(self.num_levels)):
            if (var_name in self.st[i]):
                return i
        return -1

    def exists(self, var_name):
        return self.get_level(var_name) != -1

    def get(self, var_name):
        for i in reversed(xrange(self.num_levels)):
            if (var_name in self.st[i]):
                return self.st[i][var_name]
        return None

    def add_var(self, var_name, var_value, lvl = -1):
        # TODO: var_value should be (type, node_name, outport)
        assert self.num_levels != 0, 'Symbol table is empty! Top level scope no longer exists'
        assert var_name not in self.st[lvl], 'Adding a variable that already exists'
        self.st[lvl][var_name] = var_value
        return

    def del_var(self, var_name, lvl = -1):
        assert self.num_levels != 0, 'Symbol table is empty! Top level scope no longer exists'
        if var_name in self.st[lvl]:
            del self.st[lvl][var_name]
        return

    def modify_var(self, var_name, lvl = -1):
        assert self.num_levels != 0, 'Symbol table is empty! Top level scope no longer exists'
        assert var_name in self.st[lvl], var_name + ' does not exist in the ' + str(lvl) + ' level of the symbol table'
        self.st[lvl][var_name] = var_value

    def pop(self):
        assert self.num_levels > 1, 'Attempting to pop the global scope symbol table'
        self.st.pop()
        return

    @property
    def function_list(self):
        assert self.num_levels >= 1, 'Symbol Table is empty!'
        return self.st[0]

    @property
    def num_levels(self):
        return len(self.st)

class SAP(object):
    __ST = SymbolTable()
    __ARG_TYPES = {bool: '__MODULE.boolean', float: '__MODULE.doublereal', int: '__MODULE.integer', None: 'Error'}
    __OP_DICT = {ast.Add: sap.if1.IFPlus, ast.Sub: sap.if1.IFMinus, ast.Mult: sap.if1.IFTimes, ast.Div: sap.if1.IFDiv}
    __SAP_PTRS = []

    def __init__(self, *arg_types):
        self.arg_types = [SAP.__ARG_TYPES[arg_type] for arg_type in arg_types]
        raise NotImplementedError('__init__')

    def __call__(self, f):
        func_def = ast.parse(inspect.getsource(f)).body[0]
        self.name = func_def.name
        self.arg_names = [t.arg for t in func_def.args.args]
        assert len(self.arg_names) == len(self.arg_types), 'There must be the same number of args as arg_types'
        self.body = func_def.body

        # TODO: remove these constraints
        assert isinstance(self.body[-1], ast.Return), 'The last statement must be a return statement'
        for stmt in self.body[:-1]:
            assert isinstance(stmt, ast.Assign), 'Every statement but the last must be of type Assign'
        # END TODO

        # add function
        self.code = []
        self.mangled_name = '__IF1_' + self.name
        self.code.append(mangled_name + ' = __MODULE.addfunction("' + self.name + '")')

        # add function vars to symbol table
        SAP.__ST.push()
        # TODO: use xzip??
        for idx, i, j in zip(self.arg_names, self.arg_types):
            SAP.__ST.add_var(i, None)
            self.code.append(mangled_name + '[' + (idx + 1) + '] = ' + SAP.__ARG_TYPES[j])
        # code generate
        self.code_generate()
        # pop function vars scope off symbol table
        SAP.__ST.pop()
        # add to global symbol table
        SAP.add_var(self.name, None)
        SAP.__SAP_PTRS.append(self)
        return

    def code_generate(self):
        # TODO: code generate return statement to get type
        for stmt in self.body:
            self.cg(stmt)
        if (self.name == 'main'):
            all_code = ['\n'.join(func.code) for func in SAP.__SAP_PTRS]
            all_code.insert('__MODULE = sap.if1.Module()')
            print('\n'.join(all_code))

    def cg(self, node):
        # returns tuple((node,outport))
        m = getattr(self, 'cg_' + type(node).__name__, None)
        if m is None:
            raise NotImplementedError('The function of type ' + type(node).__name__ + ' has not been implemented yet')
        return m(node)

    def cg_Assign(self, node):
        raise NotImplementedError(type(node).__name__)

    def cg_BinOp(self, node):
        raise NotImplementedError(type(node).__name__)

    def cg_Call(self, node):
        raise NotImplementedError(type(node).__name__)

    def cg_Expr(self, node):
        raise NotImplementedError(type(node).__name__)

    def cg_FunctionDef(self, node):
        raise NotImplementedError(type(node).__name__)

    def cg_Name(self, node):
        raise NotImplementedError(type(node).__name__)

    def cg_Num(self, node):
        return str(node.n)

    def cg_Return(self, node):
        self.code.append(self.mangled_name + '(1) << ' + cg(node.value))
        return

    def cg_Tuple(self, node):
        raise NotImplementedError(type(node).__name__)
