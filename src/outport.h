//-*-c++-*-
#ifndef SAP_OUTPORT
#define SAP_OUTPORT

#include "IFX.h"

class nodebase;
class type;
class outport : public IF1<outport> {
 public:
  std::weak_ptr<nodebase> weaknode;
  std::weak_ptr<type> weaktype;
  PyOwned pragmas;

  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;

  static void setup();
  virtual PyObject* string(PyObject*) override;

  static PyObject* get_node(PyObject*,void*);
  static PyObject* get_port(PyObject*,void*);
  static PyObject* get_type(PyObject*,void*);
  static PyObject* get_pragmas(PyObject*,void*);
  static PyObject* get_edges(PyObject*,void*);

  static PyObject* richcompare(PyObject*,PyObject*,int);

  operator long();

  outport(python* self, PyObject* args,PyObject* kwargs);
  outport(std::shared_ptr<nodebase> node) : weaknode(node) {
    pragmas = PyDict_New();
    if (!pragmas) throw PyErr_Occurred();
  }
  
};

#endif

