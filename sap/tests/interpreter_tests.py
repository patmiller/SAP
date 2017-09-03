import unittest
from sap.interpreter import Interpreter
from sap.if1 import Module


class TestInterpreter(unittest.TestCase):
    INTERPRETER = Interpreter()

    def monop(self, opname, inout):
        for (atype, rtype), tests in inout.iteritems():
            m = Module()
            main = m.addfunction("main")
            main[1] = getattr(m, atype)

            N = main.addnode(getattr(m, opname))
            N(1) << main[1]
            N[1] = getattr(m, rtype)

            main(1) << N[1]

            for a, r in tests:
                v = m.interpret(self.INTERPRETER, main, a)
                self.assertEquals(v, (r,), " | %s(%s %s) -> %s (%s)" % (
                    opname, atype, a, r, v[0]
                ))
        return

    def binop(self, opname, inout):
        for (atype, btype, rtype), tests in inout.iteritems():
            m = Module()
            main = m.addfunction("main")
            main[1] = getattr(m, atype)
            main[2] = getattr(m, btype)

            N = main.addnode(getattr(m, opname))
            N(1) << main[1]
            N(2) << main[2]
            N[1] = getattr(m, rtype)

            main(1) << N[1]
            for (a, b), r in tests.iteritems():
                v = m.interpret(self.INTERPRETER, main, a, b)
                self.assertEquals(v, (r,), " | %s %s %s -> %s (%s)" % (
                    a, opname, b, r, v[0]
                ))
        return

    def disabled_test_IFForall(self):
        raise NotImplementedError("IFForall")

    def disabled_test_IFSelect(self):
        raise NotImplementedError("IFSelect")

    def disabled_test_IFTagCase(self):
        raise NotImplementedError("IFTagCase")

    def disabled_test_IFLoopA(self):
        raise NotImplementedError("IFLoopA")

    def disabled_test_IFLoopB(self):
        raise NotImplementedError("IFLoopB")

    def test_IFIfThenElse(self):
        m = Module()
        main = m.addfunction("main")
        main[1] = m.integer
        main[2] = m.integer

        ifthen = main.addnode(m.IFIfThenElse)
        ifthen(1) << main[1]
        ifthen(2) << main[2]

        # Build subgraphs for 3 tests, 3 bodies, and 1 else clause
        T0 = ifthen.addgraph()
        T0[1] = m.integer
        T0[2] = m.integer
        B0 = ifthen.addgraph()
        B0[1] = m.integer
        B0[2] = m.integer

        T1 = ifthen.addgraph()
        T1[1] = m.integer
        T1[2] = m.integer
        B1 = ifthen.addgraph()
        B1[1] = m.integer
        B1[2] = m.integer

        T2 = ifthen.addgraph()
        T2[1] = m.integer
        T2[2] = m.integer
        B2 = ifthen.addgraph()
        B2[1] = m.integer
        B2[2] = m.integer

        E = ifthen.addgraph()
        E[1] = m.integer
        E[2] = m.integer

        # Test 0 is a+b < 100 -> 1234
        p = T0.addnode(m.IFPlus)
        p(1) << T0[1]
        p(2) << T0[2]
        p[1] = m.integer

        t = T0.addnode(m.IFLess)
        t(1) << p[1]
        t(2) << 100
        t[1] = m.boolean
        T0(1) << t[1]

        B0(1) << 1234

        # Test 1 is a-b < 100 -> 4321
        p = T1.addnode(m.IFMinus)
        p(1) << T1[1]
        p(2) << T1[2]
        p[1] = m.integer

        t = T1.addnode(m.IFLess)
        t(1) << p[1]
        t(2) << 100
        t[1] = m.boolean
        T1(1) << t[1]

        B1(1) << 4321

        # Test 2 is a*b < 100 -> 10000
        p = T2.addnode(m.IFTimes)
        p(1) << T2[1]
        p(2) << T2[2]
        p[1] = m.integer

        t = T2.addnode(m.IFLess)
        t(1) << p[1]
        t(2) << 100
        t[1] = m.boolean
        T2(1) << t[1]

        B2(1) << 9999

        # Else clause returns -1
        E(1) << -1


        # We take the final results out of port 1
        ifthen[1] = m.integer
        main(1) << ifthen[1]

        self.assertEquals(m.interpret(self.INTERPRETER,main,10,20),(1234,))
        self.assertEquals(m.interpret(self.INTERPRETER,main,1000,950),(4321,))
        self.assertEquals(m.interpret(self.INTERPRETER,main,1000,0),(9999,))
        self.assertEquals(m.interpret(self.INTERPRETER,main,1000,10),(-1,))
        return

    def test_IFIterate(self):
        # main(integer n)
        #   i = 0
        #   sum = 0
        #   while i < n:
        #      i = old i + 1
        #      sum = old sum + i
        #      yield sum
        #   return sum
        m = Module()
        main = m.addfunction("main")
        main[1] = m.integer

        test = main.addnode(m.IFLess)  # i < n
        test[1] = m.boolean
        test(1) << 0  # i
        test(2) << main[1]  # n

        it = main.addnode(m.IFIterate)
        it(1) << test[1]  # test
        it(2) << 0       # i
        it(3) << main[1]  # n
        it(4) << 0       # sum

        it[1] = m.boolean  # test
        it[2] = m.integer  # i
        it[3] = m.integer  # n
        it[4] = m.integer  # sum

        g = it.addgraph()
        g[1] = m.boolean  # old test
        g[2] = m.integer  # old i
        g[3] = m.integer  # old n
        g[4] = m.integer  # old sum

        plus_i = g.addnode(m.IFPlus)  # new i = old i + 1
        plus_i(1) << g[2]  # old i
        plus_i(2) << 1    # 1
        plus_i[1] = m.integer  # new i

        plus_sum = g.addnode(m.IFPlus)  # new sum = old sum + i
        plus_sum(1) << g[4]  # old sum
        plus_sum(2) << plus_i[1]  # i
        plus_sum[1] = m.integer

        lt = g.addnode(m.IFLess)  # new test = new i < n
        lt(1) << plus_i[1]
        lt(2) << g[3]
        lt[1] = m.boolean

        g(1) << lt[1]       # new test
        g(2) << plus_i[1]   # new i
        g(4) << plus_sum[1]  # new sum

        main(1) << it[4]
        self.assertEquals(m.interpret(self.INTERPRETER, main, 10), (55,))
        return

    def disabled_test_IFWhileLoop(self):
        raise NotImplementedError("IFWhileLoop")

    def disabled_test_IFRepeatLoop(self):
        raise NotImplementedError("IFRepeatLoop")

    def disabled_test_IFSeqForall(self):
        raise NotImplementedError("IFSeqForall")

    def disabled_test_IFUReduce(self):
        raise NotImplementedError("IFUReduce")

    def disabled_test_IFAAddH(self):
        raise NotImplementedError("IFAAddH")

    def disabled_test_IFAAddL(self):
        raise NotImplementedError("IFAAddL")

    def disabled_test_IFAAdjust(self):
        raise NotImplementedError("IFAAdjust")

    def disabled_test_IFABuild(self):
        raise NotImplementedError("IFABuild")

    def disabled_test_IFACatenate(self):
        raise NotImplementedError("IFACatenate")

    def disabled_test_IFAElement(self):
        raise NotImplementedError("IFAElement")

    def disabled_test_IFAFill(self):
        raise NotImplementedError("IFAFill")

    def disabled_test_IFAGather(self):
        raise NotImplementedError("IFAGather")

    def disabled_test_IFAIsEmpty(self):
        raise NotImplementedError("IFAIsEmpty")

    def disabled_test_IFALimH(self):
        raise NotImplementedError("IFALimH")

    def disabled_test_IFALimL(self):
        raise NotImplementedError("IFALimL")

    def disabled_test_IFARemH(self):
        raise NotImplementedError("IFARemH")

    def disabled_test_IFARemL(self):
        raise NotImplementedError("IFARemL")

    def disabled_test_IFAReplace(self):
        raise NotImplementedError("IFAReplace")

    def disabled_test_IFAScatter(self):
        raise NotImplementedError("IFAScatter")

    def disabled_test_IFASetL(self):
        raise NotImplementedError("IFASetL")

    def disabled_test_IFASize(self):
        raise NotImplementedError("IFASize")

    def test_IFAbs(self):
        return self.monop('IFAbs', {
            ('integer', 'integer'): [
                (3, 3),
                (-2, 2),
                (0, 0),
            ],
            ('real', 'real'): [
                (3.5, 3.5),
                (-2.75, 2.75),
                (0, 0),
            ],
            ('doublereal', 'doublereal'): [
                (3.5, 3.5),
                (-2.75, 2.75),
                (0, 0),
            ],
        })

    def disabled_test_IFBindArguments(self):
        raise NotImplementedError("IFBindArguments")

    def test_IFBool(self):
        return self.monop('IFBool', {
            ('boolean', 'boolean'): [
                (True, True),
                (False, False),
            ],
            ('character', 'boolean'): [
                ('a', True),
                ('\0', False),
            ],
            ('integer', 'boolean'): [
                (0, False),
                (-1, True),
                (7, True),
            ],
            ('real', 'boolean'): [
                (0.0, False),
                (-1.5, True),
                (7.25, True),
            ],
            ('doublereal', 'boolean'): [
                (0.0, False),
                (-1.5, True),
                (7.25, True),
            ],
        })
        raise NotImplementedError("IFBool")

    def disabled_test_IFCall(self):
        raise NotImplementedError("IFCall")

    def test_IFChar(self):
        return self.monop('IFChar', {
            ('boolean', 'character'): [
                (True, '\001'),
                (False, '\000'),
            ],
            ('integer', 'character'): [
                (32, ' '),
                (10, '\n'),
                (65, 'A'),
            ],
            ('character', 'character'): [
                ('c', 'c'),
            ],
        })

    def test_IFDiv(self):
        return self.binop('IFDiv', {
            ('integer', 'integer', 'integer'):
            {
                (3, 4): 0,
                (3, -4): -1,
            },
            ('real', 'real', 'real'):
            {
                (100.0, 2.5): 40.0,
                (31.5, -4.5): -7.0,
            },
            ('doublereal', 'doublereal', 'doublereal'):
            {
                (100.0, 2.5): 40.0,
                (31.5, -4.5): -7.0,
            },
        })

    def test_IFDouble(self):
        return self.monop('IFDouble', {
            ('boolean', 'doublereal'): [
                (True, 1.0),
                (False, 0.0),
            ],
            ('integer', 'doublereal'): [
                (3, 3.0),
                (0, 0.0),
                (-3, -3.0),
            ],
            ('real', 'doublereal'): [
                (3.0, 3.0),
                (3.5, 3.5),
                (0.0, 0.0),
                (-3.0, -3.0),
                (-3.5, -3.5),
            ],
            ('doublereal', 'doublereal'): [
                (3.0, 3.0),
                (3.5, 3.5),
                (0.0, 0.0),
                (-3.0, -3.0),
                (-3.5, -3.5),
            ],
        })

    def test_IFEqual(self):
        return self.binop('IFEqual', {
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): True,
                (True, False): False,
                (False, True): False,
                (False, False): True,
            },
            ('character', 'character', 'boolean'):
            {
                ('a', 'a'): True,
                ('b', 'a'): False,
                ('a', 'b'): False,
            },
            ('integer', 'integer', 'boolean'):
            {
                (3, 3): True,
                (3, 4): False,
                (3, -4): False,
                (-3, 4): False,
            },
            ('real', 'real', 'boolean'):
            {
                (3.25, 3.25): True,
                (3.25, 4.5): False,
                (3.75, -4.125): False,
                (-3.5, 4.25): False,
            },
            ('doublereal', 'doublereal', 'boolean'):
            {
                (3.25, 3.25): True,
                (3.25, 4.5): False,
                (3.75, -4.125): False,
                (-3.5, 4.25): False,
            },
        })

    def test_IFExp(self):
        return self.binop('IFExp', {
            ('integer', 'integer', 'real'):
            {
                (2, 3): 8.0,
                (2, -4): 0.0625,
            },
            ('integer', 'real', 'real'):
            {
                (16, .5): 4.0,
            },
            ('integer', 'doublereal', 'doublereal'):
            {
                (16, .5): 4.0,
            },
            ('real', 'integer', 'real'):
            {
                (2.5, 3): 15.625,
            },
            ('real', 'real', 'real'):
            {
                (16.0, .5): 4.0,
            },
            ('real', 'doublereal', 'doublereal'):
            {
                (16.0, .5): 4.0,
            },
            ('doublereal', 'integer', 'doublereal'):
            {
                (3.5, 4): 150.0625,
            },
            ('doublereal', 'real', 'doublereal'):
            {
                (16.0, .5): 4.0,
            },
            ('doublereal', 'doublereal', 'doublereal'):
            {
                (16.0, .5): 4.0,
            },
        })

    def disabled_test_IFFirstValue(self):
        raise NotImplementedError("IFFirstValue")

    def disabled_test_IFFinalValue(self):
        raise NotImplementedError("IFFinalValue")

    def test_IFFloor(self):
        return self.monop('IFFloor', {
            ('boolean', 'real'): [
                (True, 1.0),
                (False, 0.0),
            ],
            ('integer', 'real'): [
                (3, 3.0),
                (0, 0.0),
                (-3, -3.0),
            ],
            ('real', 'real'): [
                (3.0, 3.0),
                (3.5, 3.0),
                (0.0, 0.0),
                (-3.0, -3.0),
                (-3.5, -4.0),
            ],
            ('doublereal', 'doublereal'): [
                (3.0, 3.0),
                (3.5, 3.0),
                (0.0, 0.0),
                (-3.0, -3.0),
                (-3.5, -4.0),
            ],
        })

    def test_IFInt(self):
        return self.monop('IFInt', {
            ('boolean', 'integer'): [
                (True, 1),
                (False, 0),
            ],
            ('integer', 'integer'): [
                (3, 3),
                (0, 0),
                (-3, -3),
            ],
            ('real', 'integer'): [
                (3.0, 3),
                (3.5, 3),
                (0.0, 0),
                (-3.0, -3),
                (-3.5, -3),
            ],
            ('doublereal', 'integer'): [
                (3.0, 3),
                (3.5, 3),
                (0.0, 0),
                (-3.0, -3),
                (-3.5, -3),
            ],
        })

    def disabled_test_IFIsError(self):
        raise NotImplementedError("IFIsError")

    def test_IFLess(self):
        return self.binop('IFLess', {
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): False,
                (True, False): False,
                (False, True): True,
                (False, False): False,
            },
            ('character', 'character', 'boolean'):
            {
                ('a', 'a'): False,
                ('b', 'a'): False,
                ('a', 'b'): True,
            },
            ('integer', 'integer', 'boolean'):
            {
                (3, 3): False,
                (3, 4): True,
                (3, -4): False,
                (-3, 4): True,
            },
            ('real', 'real', 'boolean'):
            {
                (3.25, 3.25): False,
                (3.25, 4.5): True,
                (3.75, -4.125): False,
                (-3.5, 4.25): True,
            },
            ('doublereal', 'doublereal', 'boolean'):
            {
                (3.25, 3.25): False,
                (3.25, 4.5): True,
                (3.75, -4.125): False,
                (-3.5, 4.25): True,
            },
        })

    def test_IFLessEqual(self):
        return self.binop('IFLessEqual', {
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): True,
                (True, False): False,
                (False, True): True,
                (False, False): True,
            },
            ('character', 'character', 'boolean'):
            {
                ('a', 'a'): True,
                ('b', 'a'): False,
                ('a', 'b'): True,
            },
            ('integer', 'integer', 'boolean'):
            {
                (3, 3): True,
                (3, 4): True,
                (3, -4): False,
                (-3, 4): True,
            },
            ('real', 'real', 'boolean'):
            {
                (3.25, 3.25): True,
                (3.25, 4.5): True,
                (3.75, -4.125): False,
                (-3.5, 4.25): True,
            },
            ('doublereal', 'doublereal', 'boolean'):
            {
                (3.25, 3.25): True,
                (3.25, 4.5): True,
                (3.75, -4.125): False,
                (-3.5, 4.25): True,
            },
        })

    def test_IFMax(self):
        return self.binop('IFMax', {
            ('integer', 'integer', 'integer'):
            {
                (3, 4): 4,
                (3, -4): 3,
            },
            ('real', 'real', 'real'):
            {
                (3.5, 4.25): 4.25,
                (3.25, -4.25): 3.25,
            },
            ('doublereal', 'doublereal', 'doublereal'):
            {
                (3.5, 4.25): 4.25,
                (3.25, -4.25): 3.25,
            },
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): True,
                (True, False): True,
                (False, True): True,
                (False, False): False,
            },
        })

    def test_IFMin(self):
        return self.binop('IFMin', {
            ('integer', 'integer', 'integer'):
            {
                (3, 4): 3,
                (3, -4): -4,
            },
            ('real', 'real', 'real'):
            {
                (3.5, 4.25): 3.5,
                (3.25, -4.25): -4.25,
            },
            ('doublereal', 'doublereal', 'doublereal'):
            {
                (3.5, 4.25): 3.5,
                (3.25, -4.25): -4.25,
            },
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): True,
                (True, False): False,
                (False, True): False,
                (False, False): False,
            },
        })

    def test_IFMinus(self):
        return self.binop('IFMinus', {
            ('integer', 'integer', 'integer'):
            {
                (3, 4): -1,
                (3, -4): 7,
            },
            ('real', 'real', 'real'):
            {
                (3.5, 4.25): -0.75,
                (3.25, -4.25): 7.5,
            },
            ('doublereal', 'doublereal', 'doublereal'):
            {
                (3.5, 4.25): -0.75,
                (3.25, -4.25): 7.5,
            },
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): True,
                (True, False): True,
                (False, True): True,
                (False, False): False,
            },
        })

    def test_IFMod(self):
        return self.binop('IFMod', {
            ('integer', 'integer', 'integer'):
            {
                (3, 4): 3,
                (4, 3): 1,
                (100, 3): 1,
                (3, -4): -1,
            },
        })

    def test_IFNeg(self):
        return self.monop('IFNeg', {
            ('integer', 'integer'): [
                (3, -3.0),
                (0, 0.0),
                (-3, 3.0),
            ],
            ('real', 'real'): [
                (3.0, -3.0),
                (3.5, -3.5),
                (0.0, 0.0),
                (-3.0, 3.0),
                (-3.5, 3.5),
            ],
            ('doublereal', 'doublereal'): [
                (3.0, -3.0),
                (3.5, -3.5),
                (0.0, 0.0),
                (-3.0, 3.0),
                (-3.5, 3.5),
            ],
        })

    def test_IFNoOp(self):
        return self.monop('IFNoOp', {
            ('boolean', 'boolean'): [
                (True, True),
                (False, False),
            ],
            ('character', 'character'): [
                ('a', 'a'),
            ],
            ('doublereal', 'doublereal'): [
                (3.5, 3.5),
            ],
            ('integer', 'integer'): [
                (2, 2),
            ],
            ('real', 'real'): [
                (3.5, 3.5),
            ],
            ('string', 'string'): [
                ('', ''),
                ('hello', 'hello'),
            ],
        })

    def test_IFNot(self):
        return self.monop('IFNot', {
            ('boolean', 'boolean'): [
                (True, False),
                (False, True),
            ],
        })

    def test_IFNotEqual(self):
        return self.binop('IFNotEqual', {
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): False,
                (True, False): True,
                (False, True): True,
                (False, False): False,
            },
            ('character', 'character', 'boolean'):
            {
                ('a', 'a'): False,
                ('b', 'a'): True,
                ('a', 'b'): True,
            },
            ('integer', 'integer', 'boolean'):
            {
                (3, 3): False,
                (3, 4): True,
                (3, -4): True,
                (-3, 4): True,
            },
            ('real', 'real', 'boolean'):
            {
                (3.25, 3.25): False,
                (3.25, 4.5): True,
                (3.75, -4.125): True,
                (-3.5, 4.25): True,
            },
            ('doublereal', 'doublereal', 'boolean'):
            {
                (3.25, 3.25): False,
                (3.25, 4.5): True,
                (3.75, -4.125): True,
                (-3.5, 4.25): True,
            },
        })

    def test_IFPlus(self):
        return self.binop('IFPlus', {
            ('integer', 'integer', 'integer'):
            {
                (3, 4): 7,
                (3, -4): -1,
            },
            ('real', 'real', 'real'):
            {
                (3.5, 4.25): 7.75,
                (3.25, -4.25): -1,
            },
            ('doublereal', 'doublereal', 'doublereal'):
            {
                (3.5, 4.25): 7.75,
                (3.25, -4.25): -1,
            },
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): True,
                (True, False): True,
                (False, True): True,
                (False, False): False,
            },
        })

    def disabled_test_IFRangeGenerate(self):
        raise NotImplementedError("IFRangeGenerate")

    def disabled_test_IFRBuild(self):
        raise NotImplementedError("IFRBuild")

    def disabled_test_IFRElements(self):
        raise NotImplementedError("IFRElements")

    def disabled_test_IFRReplace(self):
        raise NotImplementedError("IFRReplace")

    def disabled_test_IFRedLeft(self):
        raise NotImplementedError("IFRedLeft")

    def disabled_test_IFRedRight(self):
        raise NotImplementedError("IFRedRight")

    def disabled_test_IFRedTree(self):
        raise NotImplementedError("IFRedTree")

    def disabled_test_IFReduce(self):
        raise NotImplementedError("IFReduce")

    def disabled_test_IFRestValues(self):
        raise NotImplementedError("IFRestValues")

    def test_IFSingle(self):
        return self.monop('IFSingle', {
            ('boolean', 'real'): [
                (True, 1.0),
                (False, 0.0),
            ],
            ('integer', 'real'): [
                (3, 3.0),
                (0, 0.0),
                (-3, -3.0),
            ],
            ('real', 'real'): [
                (3.0, 3.0),
                (3.5, 3.5),
                (0.0, 0.0),
                (-3.0, -3.0),
                (-3.5, -3.5),
            ],
            ('doublereal', 'real'): [
                (3.0, 3.0),
                (3.5, 3.5),
                (0.0, 0.0),
                (-3.0, -3.0),
                (-3.5, -3.5),
            ],
        })

    def test_IFTimes(self):
        return self.binop('IFTimes', {
            ('integer', 'integer', 'integer'):
            {
                (3, 4): 12,
                (3, -4): -12,
            },
            ('real', 'real', 'real'):
            {
                (3.5, 4.25): 14.875,
                (3.25, -4.25): -13.8125,
            },
            ('doublereal', 'doublereal', 'doublereal'):
            {
                (3.5, 4.25): 14.875,
                (3.25, -4.25): -13.8125,
            },
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): True,
                (True, False): False,
                (False, True): False,
                (False, False): False,
            },
        })

    def test_IFTrunc(self):
        return self.monop('IFTrunc', {
            ('boolean', 'real'): [
                (True, 1.0),
                (False, 0.0),
            ],
            ('integer', 'real'): [
                (3, 3.0),
                (0, 0.0),
                (-3, -3.0),
            ],
            ('real', 'real'): [
                (3.0, 3.0),
                (3.5, 3.0),
                (0.0, 0.0),
                (-3.0, -3.0),
                (-3.5, -3.0),
            ],
            ('doublereal', 'doublereal'): [
                (3.0, 3.0),
                (3.5, 3.0),
                (0.0, 0.0),
                (-3.0, -3.0),
                (-3.5, -3.0),
            ],
        })

    def disabled_test_IFPrefixSize(self):
        raise NotImplementedError("IFPrefixSize")

    def disabled_test_IFError(self):
        raise NotImplementedError("IFError")

    def disabled_test_IFReplaceMulti(self):
        raise NotImplementedError("IFReplaceMulti")

    def disabled_test_IFConvert(self):
        raise NotImplementedError("IFConvert")

    def disabled_test_IFCallForeign(self):
        raise NotImplementedError("IFCallForeign")

    def disabled_test_IFAElementN(self):
        raise NotImplementedError("IFAElementN")

    def disabled_test_IFAElementP(self):
        raise NotImplementedError("IFAElementP")

    def disabled_test_IFAElementM(self):
        raise NotImplementedError("IFAElementM")

    def test_Great(self):
        return self.binop('IFGreat', {
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): False,
                (True, False): True,
                (False, True): False,
                (False, False): False,
            },
            ('character', 'character', 'boolean'):
            {
                ('a', 'a'): False,
                ('b', 'a'): True,
                ('a', 'b'): False,
            },
            ('integer', 'integer', 'boolean'):
            {
                (3, 3): False,
                (3, 4): False,
                (3, -4): True,
                (-3, 4): False,
            },
            ('real', 'real', 'boolean'):
            {
                (3.25, 3.25): False,
                (3.25, 4.5): False,
                (3.75, -4.125): True,
                (-3.5, 4.25): False,
            },
            ('doublereal', 'doublereal', 'boolean'):
            {
                (3.25, 3.25): False,
                (3.25, 4.5): False,
                (3.75, -4.125): True,
                (-3.5, 4.25): False,
            },
        })

    def test_GreatEqual(self):
        return self.binop('IFGreatEqual', {
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): True,
                (True, False): True,
                (False, True): False,
                (False, False): True,
            },
            ('character', 'character', 'boolean'):
            {
                ('a', 'a'): True,
                ('b', 'a'): True,
                ('a', 'b'): False,
            },
            ('integer', 'integer', 'boolean'):
            {
                (3, 3): True,
                (3, 4): False,
                (3, -4): True,
                (-3, 4): False,
            },
            ('real', 'real', 'boolean'):
            {
                (3.25, 3.25): True,
                (3.25, 4.5): False,
                (3.75, -4.125): True,
                (-3.5, 4.25): False,
            },
            ('doublereal', 'doublereal', 'boolean'):
            {
                (3.25, 3.25): True,
                (3.25, 4.5): False,
                (3.75, -4.125): True,
                (-3.5, 4.25): False,
            },
        })

    def test_And(self):
        return self.binop('IFAnd', {
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): True,
                (True, False): False,
                (False, True): False,
                (False, False): False,
            },
        })

    def test_Or(self):
        return self.binop('IFOr', {
            ('boolean', 'boolean', 'boolean'):
            {
                (True, True): True,
                (True, False): True,
                (False, True): True,
                (False, False): False,
            },
        })
