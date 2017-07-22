//-*-c++-*-
#ifndef SAP_NODE
#define SAP_NODE

#include "IFX.h"

class module;

class nodebase {
 protected:
  std::weak_ptr<module> weakmodule;
  std::weak_ptr<nodebase> weakparent;
  long opcode;
  PyOwned children;
  PyOwned pragmas;
  std::map<long,std::shared_ptr<int>> inputs;
  std::map<long,std::shared_ptr<int>> outputs;
  
  nodebase()
    : opcode(-1), children(nullptr), pragmas(nullptr)
  {
    children = PyList_New(0);
    if (!children) throw PyErr_Occurred();

    pragmas = PyDict_New();
    if (!pragmas) throw PyErr_Occurred();
  }
  virtual ~nodebase() {}
};

class node : public nodebase, public IF1<node> {
 public:
  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;

  static PyObject* get_opcode(PyObject*,void*);
  static int set_opcode(PyObject*,PyObject*,void*);
  static PyObject* get_children(PyObject*,void*);
  static PyObject* get_pragmas(PyObject*,void*);
  static PyObject* get_if1(PyObject*,void*);
  static PyObject* get_inputs(PyObject*,void*);
  static PyObject* get_outputs(PyObject*,void*);

  node(python* self, PyObject* args,PyObject* kwargs);  
  node();
};

#endif

