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
initjjj(void) {

  // ----------------------------------------------------------------------
  // Setup
  // ----------------------------------------------------------------------
  PyObject* m = Py_InitModule3("sap.jjj", IF1_methods,
                     "IF1 module, types, nodes, graphs, and edge ports.");
  if (!m) return;

  if (!module::ready(m,"Module")) return;
  if (!type::ready(m,"Type")) return;
  if (!node::ready(m,"Node")) return;
  if (!graph::ready(m,"Graph")) return;
  if (!inport::ready(m,"InPort")) return;
  if (!outport::ready(m,"OutPort")) return;
}
