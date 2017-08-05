//-*-c++-*-
#ifndef SAP_INPORT
#define SAP_INPORT

#include "IFX.h"

class type;
class nodebase;
class outport;
class graph;

class inport : public IF1<inport> {
public:
  std::weak_ptr<nodebase> weaknode;

  // For a literal edge
  std::string literal;
  std::weak_ptr<type> weakliteral_type;

  // For a inter-node edge
  std::weak_ptr<outport> weakport;
  PyOwned(pragmas);

  static long flags;
  static PyTypeObject Type;
  static char const* doc;
  static PyMethodDef methods[];
  static PyGetSetDef getset[];
  static PyNumberMethods as_number;
  static PySequenceMethods as_sequence;

  static void setup();

  virtual PyObject* string(PyObject*) override;
  
  long port();
  std::shared_ptr<type> my_type();

  static PyObject* richcompare(PyObject*,PyObject*,int);

  static PyObject* cross_graph(std::shared_ptr<inport>,
			       std::shared_ptr<nodebase>,
			       std::shared_ptr<graph>,
			       std::shared_ptr<outport>,
			       std::shared_ptr<nodebase>,
			       std::shared_ptr<graph>);
  static PyObject* lshift(PyObject*,PyObject*);

  static PyObject* get_dst(PyObject*,void*);
  static PyObject* get_port(PyObject*,void*);
  static PyObject* get_literal(PyObject*,void*);
  static PyObject* get_type(PyObject*,void*);
  static PyObject* get_pragmas(PyObject*,void*);
  static PyObject* get_src(PyObject*,void*);
  static PyObject* get_oport(PyObject*,void*);

  std::shared_ptr<nodebase> my_node();

  inport(python* self, PyObject* args,PyObject* kwargs);
  inport(std::shared_ptr<nodebase>);
};

#endif

