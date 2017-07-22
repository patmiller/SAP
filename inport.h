//-*-c++-*-
#ifndef SAP_INPORT
#define SAP_INPORT

#include "IFX.h"

class type;
class node;
class inport : public IF1<inport> {
  long      iport;

  std::string value;
  std::weak_ptr<type> weakliteral_type;
  std::weak_ptr<type> weaksrc;
  long      oport;
  PyOwned(pragmas);
 public:
  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;

  static PyObject* get_node(PyObject*,void*);
  static PyObject* get_port(PyObject*,void*);
  static PyObject* get_literal(PyObject*,void*);
  static PyObject* get_type(PyObject*,void*);
  static PyObject* get_pragmas(PyObject*,void*);
  static PyObject* get_src(PyObject*,void*);
  static PyObject* get_oport(PyObject*,void*);

  inport (python* self, PyObject* args,PyObject* kwargs);  
};

#endif

