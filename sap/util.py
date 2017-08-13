import inspect
from textwrap import dedent


def get_func_code(f):
    """Get the code of function f without extra indents"""
    return dedent(inspect.getsource(f))
