//-*-c++-*-
#ifndef SAP_MODULE
#define SAP_MODULE

#include "IFX.h"


class module : public IF1<module> {
 public:
  PyOwned types;
  PyOwned pragmas;
  PyOwned functions;
  PyOwned dict;

  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;

  static int init(PyObject* pySelf,PyObject* args,PyObject* kwargs);
  static PyObject* getattro(PyObject*,PyObject*);

  static PyObject* addtype(PyObject*,PyObject*);
  static PyObject* addfunction(PyObject*,PyObject*);

  static PyObject* get_if1(PyObject*,void*);
  static PyObject* get_types(PyObject*,void*);
  static PyObject* get_functions(PyObject*,void*);
  static PyObject* get_pragmas(PyObject*,void*);

  module(python* self, PyObject* args,PyObject* kwargs);  
};

#endif

