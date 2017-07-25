//-*-c++-*-
#ifndef SAP_GRAPH
#define SAP_GRAPH

#include "IFX.h"
#include "node.h"

class type;
class graph : public nodebase, public IF1<graph> {
  std::weak_ptr<module> weakmodule;
  std::string name;
 public:
  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;
  static void setup();

  static long flags;
  static PyTypeObject* basetype();

  static PyObject* call(PyObject*,PyObject*,PyObject*);

  virtual PyObject* string(PyObject*) override;

  virtual PyObject* lookup() override;

  virtual operator long() override;

  PyObject* addnode(PyObject*,PyObject*);

  static PyObject* get_name(PyObject*,void*);
  static int set_name(PyObject*,PyObject*,void*);
  static PyObject* get_type(PyObject*,void*);
  static PyObject* get_nodes(PyObject*,void*);
  static PyObject* get_if1(PyObject*,void*);

  PyObject* my_type();

  virtual std::shared_ptr<graph> my_graph() override {
    return shared();
  }

  virtual std::shared_ptr<module> my_module() {
    auto m = weakmodule.lock();
    if (m) return m;
    TODO("inner graph");
    return nullptr;
  }

  graph(python* self, PyObject* args,PyObject* kwargs);
  graph(long opcode,
	std::shared_ptr<module> module,
	std::string const& name);
};

#endif

