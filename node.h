//-*-c++-*-
#ifndef SAP_NODE
#define SAP_NODE

#include "IFX.h"

class module;

class nodebase {
 protected:
  long opcode;
  std::weak_ptr<nodebase> weakparent;
  PyOwned children;
  PyOwned pragmas;
  std::map<long,std::shared_ptr<int>> inputs;
  std::map<long,std::shared_ptr<int>> outputs;
  
  nodebase(long opcode=-1,
	   std::shared_ptr<nodebase> parent=nullptr)
    : opcode(opcode),
      weakparent(parent)
  {
    children = PyList_New(0);
    if (!children) throw PyErr_Occurred();

    pragmas = PyDict_New();
    if (!pragmas) throw PyErr_Occurred();
  }
  virtual ~nodebase() {}

  PyObject* string() {
    auto p = opcode_to_name.find(opcode);
    if (p == opcode_to_name.end()) {
      return PyString_FromFormat("<Node %ld>",opcode);
    }
    return PyString_FromString(p->second.c_str());
  }
public:
  static std::map<long,std::string> opcode_to_name;
  static std::map<std::string,long> name_to_opcode;

};

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

  node(python* self, PyObject* args,PyObject* kwargs);  
  node(long opcode=-1,std::shared_ptr<nodebase> parent=nullptr);
};

#endif

