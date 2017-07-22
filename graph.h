//-*-c++-*-
#ifndef SAP_GRAPH
#define SAP_GRAPH

#include "IFX.h"
#include "node.h"

class type;
class graph : public nodebase, public IF1<graph> {
  std::string name;
  std::weak_ptr<type> weaktype;
 public:
  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;

  PyObject* addnode(PyObject*,PyObject*);

  static PyObject* get_name(PyObject*,void*);
  static int set_name(PyObject*,PyObject*,void*);
  static PyObject* get_type(PyObject*,void*);
  static int set_type(PyObject*,PyObject*,void*);
  static PyObject* get_nodes(PyObject*,void*);

  static long flags;
  static PyTypeObject* basetype();

  graph(python* self, PyObject* args,PyObject* kwargs);  
};

#endif

