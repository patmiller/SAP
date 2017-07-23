#include "Python.h"

#include "module.h"
#include "type.h"
#include "node.h"
#include "graph.h"
#include "inport.h"
#include "outport.h"

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
  PyModule_AddIntMacro(m,IF_Array);
  PyModule_AddIntMacro(m,IF_Basic);
  PyModule_AddIntMacro(m,IF_Field);
  PyModule_AddIntMacro(m,IF_Function);
  PyModule_AddIntMacro(m,IF_Multiple);
  PyModule_AddIntMacro(m,IF_Record);
  PyModule_AddIntMacro(m,IF_Stream);
  PyModule_AddIntMacro(m,IF_Tag);
  PyModule_AddIntMacro(m,IF_Tuple);
  PyModule_AddIntMacro(m,IF_Union);
  PyModule_AddIntMacro(m,IF_Wild);
  PyModule_AddIntMacro(m,IF_Boolean);
  PyModule_AddIntMacro(m,IF_Character);
  PyModule_AddIntMacro(m,IF_DoubleReal);
  PyModule_AddIntMacro(m,IF_Integer);
  PyModule_AddIntMacro(m,IF_Null);
  PyModule_AddIntMacro(m,IF_Real);
  PyModule_AddIntMacro(m,IF_WildBasic);
  PyModule_AddIntMacro(m,IFAAddH);
  PyModule_AddIntMacro(m,IFAAddL);
  PyModule_AddIntMacro(m,IFAAdjust);
  PyModule_AddIntMacro(m,IFABuild);
  PyModule_AddIntMacro(m,IFACatenate);
  PyModule_AddIntMacro(m,IFAElement);
  PyModule_AddIntMacro(m,IFAFill);
  PyModule_AddIntMacro(m,IFAGather);
  PyModule_AddIntMacro(m,IFAIsEmpty);
  PyModule_AddIntMacro(m,IFALimH);
  PyModule_AddIntMacro(m,IFALimL);
  PyModule_AddIntMacro(m,IFARemH);
  PyModule_AddIntMacro(m,IFARemL);
  PyModule_AddIntMacro(m,IFAReplace);
  PyModule_AddIntMacro(m,IFAScatter);
  PyModule_AddIntMacro(m,IFASetL);
  PyModule_AddIntMacro(m,IFASize);
  PyModule_AddIntMacro(m,IFAbs);
  PyModule_AddIntMacro(m,IFBindArguments);
  PyModule_AddIntMacro(m,IFBool);
  PyModule_AddIntMacro(m,IFCall);
  PyModule_AddIntMacro(m,IFChar);
  PyModule_AddIntMacro(m,IFDiv);
  PyModule_AddIntMacro(m,IFDouble);
  PyModule_AddIntMacro(m,IFEqual);
  PyModule_AddIntMacro(m,IFExp);
  PyModule_AddIntMacro(m,IFFirstValue);
  PyModule_AddIntMacro(m,IFFinalValue);
  PyModule_AddIntMacro(m,IFFloor);
  PyModule_AddIntMacro(m,IFInt);
  PyModule_AddIntMacro(m,IFIsError);
  PyModule_AddIntMacro(m,IFLess);
  PyModule_AddIntMacro(m,IFLessEqual);
  PyModule_AddIntMacro(m,IFMax);
  PyModule_AddIntMacro(m,IFMin);
  PyModule_AddIntMacro(m,IFMinus);
  PyModule_AddIntMacro(m,IFMod);
  PyModule_AddIntMacro(m,IFNeg);
  PyModule_AddIntMacro(m,IFNoOp);
  PyModule_AddIntMacro(m,IFNot);
  PyModule_AddIntMacro(m,IFNotEqual);
  PyModule_AddIntMacro(m,IFPlus);
  PyModule_AddIntMacro(m,IFRangeGenerate);
  PyModule_AddIntMacro(m,IFRBuild);
  PyModule_AddIntMacro(m,IFRElements);
  PyModule_AddIntMacro(m,IFRReplace);
  PyModule_AddIntMacro(m,IFRedLeft);
  PyModule_AddIntMacro(m,IFRedRight);
  PyModule_AddIntMacro(m,IFRedTree);
  PyModule_AddIntMacro(m,IFReduce);
  PyModule_AddIntMacro(m,IFRestValues);
  PyModule_AddIntMacro(m,IFSingle);
  PyModule_AddIntMacro(m,IFTimes);
  PyModule_AddIntMacro(m,IFTrunc);
  PyModule_AddIntMacro(m,IFPrefixSize);
  PyModule_AddIntMacro(m,IFError);
  PyModule_AddIntMacro(m,IFReplaceMulti);
  PyModule_AddIntMacro(m,IFConvert);
  PyModule_AddIntMacro(m,IFCallForeign);
  PyModule_AddIntMacro(m,IFAElementN);
  PyModule_AddIntMacro(m,IFAElementP);
  PyModule_AddIntMacro(m,IFAElementM);
  PyModule_AddIntMacro(m,IFSGraph);
  PyModule_AddIntMacro(m,IFLGraph);
  PyModule_AddIntMacro(m,IFIGraph);
  PyModule_AddIntMacro(m,IFXGraph);
  PyModule_AddIntMacro(m,IFLPGraph);
  PyModule_AddIntMacro(m,IFRLGraph);

  // Build the opcode map for Python and for node.cpp
  PyOwned o2n(PyDict_New());
  if (!o2n) return;
  PyOwned n2o(PyDict_New());
  if (!n2o) return;
  PyObject* dict = PyModule_GetDict(m);
  PyObject* key;
  PyObject* value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(dict, &pos, &key, &value)) {
    if (PyString_Check(key) && PyInt_Check(value) &&
	PyString_AS_STRING(key)[0] == 'I' &&
	PyString_AS_STRING(key)[1] == 'F' &&
	PyString_AS_STRING(key)[2] != '_') {
      long opcode = PyInt_AS_LONG(value);
      char const* name = PyString_AS_STRING(key)+2;
      nodebase::opcode_to_name[opcode] = name;
      nodebase::name_to_opcode[name] = opcode;
      PyOwned nm(PyString_FromString(name));
      PyDict_SetItem(o2n.borrow(),value,nm.borrow());
      PyDict_SetItem(n2o.borrow(),nm.borrow(),value);
    }
  }

  PyModule_AddObject(m,"opnames",o2n.incref());
  PyModule_AddObject(m,"opcodes",n2o.incref());
}
//(insert "\n" (shell-command-to-string "awk '/ IF/{printf \"PyModule_AddIntMacro(m,%s);\\n\", $1}' IFX.h"))
