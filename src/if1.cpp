#include "Python.h"

#include "IFX.h"

#include "module.h"
#include "type.h"
#include "node.h"
#include "graph.h"
#include "inport.h"
#include "outport.h"

PyObject* DEFAULT_OPCODES = nullptr; // See IFX.h

static PyMethodDef IF1_methods[] = {
    {nullptr}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initif1(void) {

  // ----------------------------------------------------------------------
  // Setup
  // ----------------------------------------------------------------------
  PyObject* m = Py_InitModule3("sap.if1", IF1_methods,
                     "IF1 module, types, nodes, graphs, and edge ports.");
  if (!m) return;

  if (!module::ready(m,"Module")) return;
  if (!type::ready(m,"Type")) return;
  if (!node::ready(m,"Node")) return;
  if (!graph::ready(m,"Graph")) return;
  if (!inport::ready(m,"InPort")) return;
  if (!outport::ready(m,"OutPort")) return;

    // ----------------------------------------------------------------------
  // Export macros
  // ----------------------------------------------------------------------

  // Build the opcode map for Python and for node.cpp
  DEFAULT_OPCODES = PyDict_New();
  if (!DEFAULT_OPCODES) return;
    //(insert "\n" (shell-command-to-string "grep IF IFX.h | grep = | awk '{printf \"  PyDict_SetItemString(DEFAULT_OPCODES,\\\"%s\\\",PyInt_FromLong(%s)); if (PyErr_Occurred()) return;\\n\",$1,$1}'"))
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Array",PyInt_FromLong(IF_Array)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Basic",PyInt_FromLong(IF_Basic)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Field",PyInt_FromLong(IF_Field)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Function",PyInt_FromLong(IF_Function)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Multiple",PyInt_FromLong(IF_Multiple)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Record",PyInt_FromLong(IF_Record)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Stream",PyInt_FromLong(IF_Stream)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Tag",PyInt_FromLong(IF_Tag)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Tuple",PyInt_FromLong(IF_Tuple)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Union",PyInt_FromLong(IF_Union)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Wild",PyInt_FromLong(IF_Wild)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Boolean",PyInt_FromLong(IF_Boolean)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Character",PyInt_FromLong(IF_Character)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_DoubleReal",PyInt_FromLong(IF_DoubleReal)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Integer",PyInt_FromLong(IF_Integer)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Null",PyInt_FromLong(IF_Null)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_Real",PyInt_FromLong(IF_Real)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IF_WildBasic",PyInt_FromLong(IF_WildBasic)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFForall",PyInt_FromLong(IFForall)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFSelect",PyInt_FromLong(IFSelect)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFTagCase",PyInt_FromLong(IFTagCase)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFLoopA",PyInt_FromLong(IFLoopA)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFLoopB",PyInt_FromLong(IFLoopB)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFIfThenElse",PyInt_FromLong(IFIfThenElse)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFIterate",PyInt_FromLong(IFIterate)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFWhileLoop",PyInt_FromLong(IFWhileLoop)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFRepeatLoop",PyInt_FromLong(IFRepeatLoop)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFSeqForall",PyInt_FromLong(IFSeqForall)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFUReduce",PyInt_FromLong(IFUReduce)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAAddH",PyInt_FromLong(IFAAddH)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAAddL",PyInt_FromLong(IFAAddL)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAAdjust",PyInt_FromLong(IFAAdjust)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFABuild",PyInt_FromLong(IFABuild)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFACatenate",PyInt_FromLong(IFACatenate)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAElement",PyInt_FromLong(IFAElement)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAFill",PyInt_FromLong(IFAFill)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAGather",PyInt_FromLong(IFAGather)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAIsEmpty",PyInt_FromLong(IFAIsEmpty)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFALimH",PyInt_FromLong(IFALimH)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFALimL",PyInt_FromLong(IFALimL)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFARemH",PyInt_FromLong(IFARemH)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFARemL",PyInt_FromLong(IFARemL)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAReplace",PyInt_FromLong(IFAReplace)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAScatter",PyInt_FromLong(IFAScatter)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFASetL",PyInt_FromLong(IFASetL)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFASize",PyInt_FromLong(IFASize)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAbs",PyInt_FromLong(IFAbs)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFBindArguments",PyInt_FromLong(IFBindArguments)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFBool",PyInt_FromLong(IFBool)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFCall",PyInt_FromLong(IFCall)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFChar",PyInt_FromLong(IFChar)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFDiv",PyInt_FromLong(IFDiv)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFDouble",PyInt_FromLong(IFDouble)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFEqual",PyInt_FromLong(IFEqual)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFExp",PyInt_FromLong(IFExp)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFFirstValue",PyInt_FromLong(IFFirstValue)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFFinalValue",PyInt_FromLong(IFFinalValue)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFFloor",PyInt_FromLong(IFFloor)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFInt",PyInt_FromLong(IFInt)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFIsError",PyInt_FromLong(IFIsError)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFLess",PyInt_FromLong(IFLess)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFLessEqual",PyInt_FromLong(IFGreatEqual)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFGreat",PyInt_FromLong(IFLess)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFGreatEqual",PyInt_FromLong(IFGreatEqual)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFMax",PyInt_FromLong(IFMax)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFMin",PyInt_FromLong(IFMin)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFMinus",PyInt_FromLong(IFMinus)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFMod",PyInt_FromLong(IFMod)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFNeg",PyInt_FromLong(IFNeg)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFNoOp",PyInt_FromLong(IFNoOp)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFNot",PyInt_FromLong(IFNot)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFNotEqual",PyInt_FromLong(IFNotEqual)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFPlus",PyInt_FromLong(IFPlus)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFRangeGenerate",PyInt_FromLong(IFRangeGenerate)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFRBuild",PyInt_FromLong(IFRBuild)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFRElements",PyInt_FromLong(IFRElements)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFRReplace",PyInt_FromLong(IFRReplace)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFRedLeft",PyInt_FromLong(IFRedLeft)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFRedRight",PyInt_FromLong(IFRedRight)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFRedTree",PyInt_FromLong(IFRedTree)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFReduce",PyInt_FromLong(IFReduce)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFRestValues",PyInt_FromLong(IFRestValues)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFSingle",PyInt_FromLong(IFSingle)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFTimes",PyInt_FromLong(IFTimes)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFTrunc",PyInt_FromLong(IFTrunc)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFPrefixSize",PyInt_FromLong(IFPrefixSize)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFError",PyInt_FromLong(IFError)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFReplaceMulti",PyInt_FromLong(IFReplaceMulti)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFConvert",PyInt_FromLong(IFConvert)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFCallForeign",PyInt_FromLong(IFCallForeign)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAElementN",PyInt_FromLong(IFAElementN)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAElementP",PyInt_FromLong(IFAElementP)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFAElementM",PyInt_FromLong(IFAElementM)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFSGraph",PyInt_FromLong(IFSGraph)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFLGraph",PyInt_FromLong(IFLGraph)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFIGraph",PyInt_FromLong(IFIGraph)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFXGraph",PyInt_FromLong(IFXGraph)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFLPGraph",PyInt_FromLong(IFLPGraph)); if (PyErr_Occurred()) return;
  PyDict_SetItemString(DEFAULT_OPCODES,"IFRLGraph",PyInt_FromLong(IFRLGraph)); if (PyErr_Occurred()) return;
}
