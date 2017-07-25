#include "node.h"
#include "type.h"
#include "outport.h"

std::map<long,std::string> nodebase::opcode_to_name;
std::map<std::string,long> nodebase::name_to_opcode;

PyTypeObject node::Type;

char const* node::doc = "TBD Node";

PyMethodDef node::methods[] = {
  {nullptr}
};

PyGetSetDef node::getset[] = {
  {(char*)"opcode",node::get_opcode,node::set_opcode,(char*)"TBD",nullptr},
  {(char*)"children",node::get_children,nullptr,(char*)"TBD",nullptr},
  {(char*)"pragmas",node::get_pragmas,nullptr,(char*)"TBD",nullptr},
  {(char*)"if1",node::get_if1,nullptr,(char*)"IF1 code for entire node",nullptr},
  {(char*)"inputs",node::get_inputs,nullptr,(char*)"TBD",nullptr},
  {(char*)"outputs",node::get_outputs,nullptr,(char*)"TBD",nullptr},
  {nullptr}
};

PyObject* node::get_opcode(PyObject* pySelf,void*) {
  return TODO("get_opcode");
}
int node::set_opcode(PyObject* pySelf,PyObject*,void*) {
  TODO("set_if1");
  return -1;
}
PyObject* node::get_children(PyObject* pySelf,void*) {
  return TODO("get_children");
}
PyObject* node::get_pragmas(PyObject* pySelf,void*) {
  return TODO("get_pragmas");
}
PyObject* node::get_if1(PyObject* pySelf,void*) {
  return TODO("get_if1");
}
PyObject* node::get_inputs(PyObject* pySelf,void*) {
  return TODO("get_inputs");
}
PyObject* node::get_outputs(PyObject* pySelf,void*) {
  return TODO("get_outputs");
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

void node::setup() {
  as_sequence.sq_length = length;
  as_sequence.sq_item = item;
  as_sequence.sq_ass_item = ass_item;
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
