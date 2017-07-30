//-*-c++-*-
#ifndef SAP_MODULE
#define SAP_MODULE

#include "IFX.h"

class type;
class module : public IF1<module> {
 public:
  PyOwned types;
  PyOwned pragmas;
  PyOwned functions;
  PyOwned opcodes;
  PyOwned dict;

  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;

  static int init(PyObject* pySelf,PyObject* args,PyObject* kwargs);
  static PyObject* getattro(PyObject*,PyObject*);

  static void setup();

  static PyObject* addtype(PyObject*,PyObject*,PyObject*);
  static PyObject* addtypechain(PyObject*,PyObject*,PyObject*);
  static PyObject* addfunction(PyObject*,PyObject*);

  static PyObject* get_if1(PyObject*,void*);
  static PyObject* get_types(PyObject*,void*);
  static PyObject* get_functions(PyObject*,void*);
  static PyObject* get_pragmas(PyObject*,void*);
  static PyObject* get_opcodes(PyObject*,void*);
  static int set_opcodes(PyObject*,PyObject*,void*);
  static PyObject* item(PyObject*,ssize_t);

  bool strip_blanks(PyObject*);
  bool read_types(PyObject*,std::map<long,std::shared_ptr<type>>& typemap);
  bool read_pragmas(PyObject*);
  bool read_functions(PyObject*,std::map<long,std::shared_ptr<type>>& typemap);
  std::string lookup(long);
  long lookup(std::string const&);

  module(python* self, PyObject* args,PyObject* kwargs);
  
};

#endif

