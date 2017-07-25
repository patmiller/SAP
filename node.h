//-*-c++-*-
#ifndef SAP_NODE
#define SAP_NODE

#include "IFX.h"
#include "nodebase.h"


class node : public nodebase, public IF1<node> {
 public:
  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;

  virtual PyObject* string(PyObject*) override;

  static PyObject* get_opcode(PyObject*,void*);
  static int set_opcode(PyObject*,PyObject*,void*);
  static PyObject* get_children(PyObject*,void*);
  static PyObject* get_pragmas(PyObject*,void*);
  static PyObject* get_if1(PyObject*,void*);
  static PyObject* get_inputs(PyObject*,void*);
  static PyObject* get_outputs(PyObject*,void*);

  static ssize_t length(PyObject*);
  static PyObject* item(PyObject*,ssize_t);
  static int ass_item(PyObject*,ssize_t,PyObject*);

  static void setup();

  node(python* self, PyObject* args,PyObject* kwargs);  
  node(long opcode=-1,std::shared_ptr<nodebase> parent=nullptr);
};

#endif

