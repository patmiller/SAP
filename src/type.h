//-*-c++-*-
#ifndef SAP_TYPE
#define SAP_TYPE

#include "IFX.h"
class module;

class type : public IF1<type> {
 public:
  std::weak_ptr<module> weakmodule;
  unsigned long code;
  unsigned long aux;
  std::weak_ptr<type> parameter1;
  std::weak_ptr<type> parameter2;
  PyOwned pragmas;

  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;
  static void setup();

  virtual PyObject* string(PyObject*) override;
  static PyObject* string(std::weak_ptr<type>&);
  operator long();

  static PyObject* nb_int(PyObject*);

  static PyObject* get_code(PyObject*,void*);
  static PyObject* get_aux(PyObject*,void*);
  static PyObject* get_parameter1(PyObject*,void*);
  static PyObject* get_parameter2(PyObject*,void*);
  static PyObject* get_pragmas(PyObject*,void*);
  static PyObject* get_name(PyObject*,void*);
  static int       set_name(PyObject*,PyObject*,void*);
  static PyObject* get_if1(PyObject*,void*);
  static PyObject* get_label(PyObject*,void*);

  static PyObject* chain(PyObject*,PyObject*);

  type();
  type(python* self, PyObject* args,PyObject* kwargs);  
  type(long code,long aux=0,std::shared_ptr<type> p1=nullptr,std::shared_ptr<type> p2=nullptr);

  ssize_t connect(std::shared_ptr<module>&,char const* name=nullptr); // Set the name and connect to module
  PyObject* lookup(); // Find raw pointer in type table
};

#endif

