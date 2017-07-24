//-*-c++-*-
#ifndef SAP_OUTPORT
#define SAP_OUTPORT

#include "IFX.h"

class type;
class outport : public IF1<outport> {
 public:
  std::weak_ptr<type> weaktype;
  unsigned long port;
  PyOwned pragmas;

  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;

  static PyObject* get_node(PyObject*,void*);
  static PyObject* get_port(PyObject*,void*);
  static PyObject* get_type(PyObject*,void*);
  static PyObject* get_pragmas(PyObject*,void*);
  static PyObject* get_edges(PyObject*,void*);

  outport (python* self, PyObject* args,PyObject* kwargs);  
};

#endif

