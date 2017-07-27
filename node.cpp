#include "node.h"
#include "type.h"
#include "inport.h"
#include "outport.h"

void node::setup() {
  as_sequence.sq_length = length;
  as_sequence.sq_item = item;
  as_sequence.sq_ass_item = ass_item;

  Type.tp_call = call;

}

std::map<long,std::string> nodebase::opcode_to_name;
std::map<std::string,long> nodebase::name_to_opcode;

PyTypeObject node::Type;

char const* node::doc = "TBD Node";

PyMethodDef node::methods[] = {
  {nullptr}
};

PyObject* node::call(PyObject* self,PyObject* args,PyObject* kwargs) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  long port;
  char* keywords[] = {(char*)"port",nullptr};
  if (!PyArg_ParseTupleAndKeywords(args,kwargs,"l",keywords,&port)) return nullptr;
  if (port <= 0) {
    return PyErr_Format(PyExc_TypeError,"invalid port %ld",port);
  }

  // We either have a port here or we must make one
  auto p = cxx->inputs.find(port);
  std::shared_ptr<inport> E;
  if (p != cxx->inputs.end()) {
    E = p->second;
  }
  else {
    E = cxx->inputs[port] = std::make_shared<inport>(cxx);
  }

  return E->package();
}

PyGetSetDef node::getset[] = {
  {(char*)"opcode",node::get_opcode,node::set_opcode,(char*)"TBD",nullptr},
  {(char*)"children",node::get_children,nullptr,(char*)"TBD",nullptr},
  {(char*)"pragmas",node::get_pragmas,nullptr,(char*)"TBD",nullptr},
  {(char*)"if1",node::get_if1,nullptr,(char*)"IF1 code for entire node",nullptr},
  {(char*)"inputs",node::get_inputs,nullptr,(char*)"TBD",nullptr},
  {(char*)"outputs",node::get_outputs,nullptr,(char*)"TBD",nullptr},
  {nullptr}
};

PyObject* node::get_opcode(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return PyInt_FromLong(cxx->opcode);
}
int node::set_opcode(PyObject* self,PyObject* op,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  long opcode = PyInt_AsLong(op);
  if (opcode < 0 && PyErr_Occurred()) return -1;
  cxx->opcode = opcode;
  return 0;
}
PyObject* node::get_children(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->children.incref();
}
PyObject* node::get_pragmas(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return cxx->pragmas.incref();
}

PyObject* node::get_if1(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  long label = cxx->operator long();

  // Compounds are different
  if (PyList_GET_SIZE(cxx->children.borrow())) {
    return TODO("node compound if1");
  }

  PyOwned result(PyString_FromFormat("N %ld %ld",
				     label,
				     cxx->opcode));
  PyOwned prags(pragma_string(cxx->pragmas.borrow()));
  if (!prags) return nullptr;

  // We also need the input edges (we are label 0)
  PyOwned edges(cxx->edge_if1(label));
  if (!edges) return nullptr;
  

  PyString_Concat(result.addr(),prags.borrow());
  if (!result) return nullptr;
  PyString_Concat(result.addr(),edges.borrow());
  if (!result) return nullptr;

  return result.incref();
}

PyObject* node::get_inputs(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  ssize_t arity = cxx->inputs.size();
  PyOwned tup(PyTuple_New(arity));
  ssize_t i=0;
  for(auto x:cxx->inputs) {
    PyTuple_SET_ITEM(tup.borrow(),i++,x.second->package());
  }
  return tup.incref();
}
PyObject* node::get_outputs(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  ssize_t arity = cxx->outputs.size();
  PyOwned tup(PyTuple_New(arity));
  ssize_t i=0;
  for(auto x:cxx->outputs) {
    PyTuple_SET_ITEM(tup.borrow(),i++,x.second->package());
  }
  return tup.incref();
}

PyObject* node::string(PyObject*) {
  return nodebase::string();
}

ssize_t node::length(PyObject*) {
  TODO("length");
  return -1;
}
PyObject* node::item(PyObject* self,ssize_t idx) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  auto it = cxx->outputs.find(idx);
  if (it == cxx->outputs.end()) {
    return PyErr_Format(PyExc_IndexError,"no edge at this port");
  }

  return it->second->package();
}
int node::ass_item(PyObject* self,ssize_t idx,PyObject* rhs) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  // Delete?
  if (!rhs) {
    auto it = cxx->outputs.find(idx);
    if (it != cxx->outputs.end()) cxx->outputs.erase(it);
    return 0;
  }

  if ( !PyObject_TypeCheck(rhs,&type::Type) ) {
    PyErr_SetString(PyExc_TypeError,"only types can be assigned to ports");
    return -1;
  }
  auto& op = cxx->outputs[idx];
  if (!op) op = std::make_shared<outport>(cxx);
  op->weaktype = reinterpret_cast<type::python*>(rhs)->cxx;
  return 0;
}


node::node(python* self, PyObject* args,PyObject* kwargs)
{
  throw PyErr_Format(PyExc_TypeError,"Cannot create new nodes this way.  see Graph.addnode()");
}

node::node(long opcode, std::shared_ptr<nodebase> parent)
  : nodebase(opcode,parent)
{
}



PyNumberMethods node::as_number;
PySequenceMethods node::as_sequence;
