#include "module.h"
#include "type.h"
#include "graph.h"

PyTypeObject module::Type;

char const* module::doc = "TBD Module";

PyMethodDef module::methods[] = {
  {(char*)"addtype",(PyCFunction)module::addtype,METH_VARARGS|METH_KEYWORDS,"add a type"},
  {(char*)"addfunction",module::addfunction,METH_VARARGS,"add a new function"},
  {nullptr}
};

PyObject* module::addtype(PyObject* self,PyObject* args,PyObject* kwargs) {
  STATIC_STR(AUX,"aux");
  STATIC_STR(NAME,"name");

  auto cxx = reinterpret_cast<python*>(self)->cxx;

  // The code argument is required
  if (PyTuple_GET_SIZE(args) < 1 ) {
    return PyErr_Format(PyExc_TypeError,"must provide a typecode");
  }
  PyObject* o /*borrowed*/ = PyTuple_GET_ITEM(args,0);
  if (!PyInt_Check(o)) {
    return PyErr_Format(PyExc_TypeError,"typecode must be an integer");
  }
  long code = PyInt_AS_LONG(o);

  long aux=0;
  char const* name = nullptr;
  if (kwargs) {
    PyObject* optAux = PyDict_GetItem(kwargs,AUX);
    PyObject* optName = PyDict_GetItem(kwargs,NAME);
    if ( optAux ) {
      aux = PyInt_AsLong(optAux);
      if (aux < 0 && PyErr_Occurred()) return nullptr;
    }
    if (optName) {
      if (!PyString_Check(optName)) {
	return PyErr_Format(PyExc_TypeError,"name must be a string");
      }
      name = PyString_AS_STRING(optName);
    }
  }

  ssize_t n = 0;
  std::shared_ptr<type> p1,p2;
  switch(code) {
  case IF_Wild:
    n = std::make_shared<type>(code)->connect(cxx,name);
    break;
  case IF_Basic:
    n = std::make_shared<type>(code,aux)->connect(cxx,name);
    break;
  case IF_Array:
  case IF_Multiple:
  case IF_Stream:
    if (PyTuple_GET_SIZE(args) != 2 ) {
      return PyErr_Format(PyExc_TypeError,"requires exactly 2 args");
    }
    o = PyTuple_GET_ITEM(args,1);
    if (!PyObject_TypeCheck(o,&type::Type)) {
      return PyErr_Format(PyExc_TypeError,"parameter1 must be a type");
    }
    p1 = reinterpret_cast<type::python*>(o)->cxx;
    n = std::make_shared<type>(code,aux,p1)->connect(cxx,name);
    break;
  default:
    return PyErr_Format(PyExc_NotImplementedError,"fix code %ld",code);
  }
    
  // The p1/p2 must be missing (nullptr), Py_None, a type, or a tuple
  auto fixtype = [](PyObject* o) {
    if (!o || o == Py_None) {
      return std::shared_ptr<type>(nullptr);
    }
    if (PyObject_TypeCheck(o,&type::Type)) {
      return reinterpret_cast<type::python*>(o)->cxx;
    }
    throw PyErr_Format(PyExc_TypeError,"parameter must be empty, None, a type object, or a tuple");
  };

  if (n < 0) return nullptr;

  // Fetch that item from our list (previous version or newly created)
  PyObject* T = PyList_GET_ITEM(cxx->types.borrow(),n);
  Py_INCREF(T);
  return T;
}

PyObject* module::addfunction(PyObject* self,PyObject* args) {
  //auto cxx = reinterpret_cast<python*>(self)->cxx;
  return PyErr_Format(PyExc_NotImplementedError,"add function");
}

PyGetSetDef module::getset[] = {
  {(char*)"if1",module::get_if1,nullptr,(char*)"IF1 code for entire module",nullptr},
  {(char*)"types",module::get_types,nullptr,(char*)"types",nullptr},
  {(char*)"pragmas",module::get_pragmas,nullptr,(char*)"whole program pragma dictionary (char->string)",nullptr},
  {(char*)"functions",module::get_functions,nullptr,(char*)"top level functions",nullptr},
  {nullptr}
};

PyObject* module::get_if1(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  STATIC_STR(NEWLINE,"\n");
  STATIC_STR(C_DOLLAR_SPACESPACE,"C$  ");
  STATIC_STR(SPACE," ");
  
  // Do the functions first so that any new types get generated first
  PyObject* /*owned*/ functions = PyString_FromString("");
  if (!functions) return nullptr;
  for(ssize_t i=0; i<PyList_GET_SIZE(cxx->functions.borrow()); ++i) {
    PyObject* /*owned*/ if1 = PyObject_GetAttrString(PyList_GET_ITEM(cxx->functions.borrow(),i),"if1");
    if (!if1) return nullptr;
    PyString_ConcatAndDel(&functions,if1);
    if (!functions) return nullptr;
  }
  PyOwned suffix(functions);  // functions now borrowed

    // Add if1 for all the types
  PyObject* result = PyString_FromString("");
  if (!result) return nullptr;
  for(ssize_t i=0;i<PyList_GET_SIZE(cxx->types.borrow());++i) {
    PyObject* if1 /*owned*/ = PyObject_GetAttrString(PyList_GET_ITEM(cxx->types.borrow(),i),"if1");
    PyString_ConcatAndDel(&result,if1);
    if (!result) return nullptr;
    PyString_Concat(&result,NEWLINE);
    if (!result) return nullptr;
  }

  // And now the pragmas
  PyObject* key;
  PyObject* remark;
  Py_ssize_t pos = 0;
  while (PyDict_Next(cxx->pragmas.borrow(), &pos, &key, &remark)) {
    // Only care about 1 char keys with string values
    if (!PyString_Check(key) ||
	PyString_GET_SIZE(key) != 1 ||
	!PyString_Check(remark)) {
      continue;
    }
    PyString_Concat(&result,C_DOLLAR_SPACESPACE);
    if (!result) return nullptr;
    PyString_Concat(&result,key);
    if (!result) return nullptr;
    PyString_Concat(&result,SPACE);
    if (!result) return nullptr;
    PyString_Concat(&result,remark);
    if (!result) return nullptr;
    PyString_Concat(&result,NEWLINE);
    if (!result) return nullptr;
  }
  PyString_Concat(&result,functions);

  return result;
}

PyObject* module::get_types(PyObject* pySelf,void*) {
  auto cxx = reinterpret_cast<python*>(pySelf)->cxx;
  return cxx->types.incref();
}

PyObject* module::get_functions(PyObject* pySelf,void*) {
  auto cxx = reinterpret_cast<python*>(pySelf)->cxx;
  return cxx->functions.incref();
}

PyObject* module::get_pragmas(PyObject* pySelf,void*) {
  auto cxx = reinterpret_cast<python*>(pySelf)->cxx;
  return cxx->pragmas.incref();
}

int module::init(PyObject* pySelf,PyObject* args,PyObject* kwargs) {
  auto status = IF1<module>::init(pySelf,args,kwargs);
  if (status < 0) return status;
  auto cxx = reinterpret_cast<python*>(pySelf)->cxx;

  PyObject* source = nullptr;
  static char* keywords[] = {(char*)"source",nullptr};
  if (!PyArg_ParseTupleAndKeywords(args,kwargs,"|O!",keywords,&PyString_Type,&source)) throw PyErr_Occurred();

  if (source) {
    //PyObject* split /*owned*/ = PyObject_CallMethod(source,(char*)"split",(char*)"s","\n");
    //if (!split) return -1;
    //if (module_read_types(self,split) != 0) { Py_DECREF(split); return -1; }
  } else {

    // We create the built in types here (not in c'tor) because we need to set a weak
    // pointer to the module (can't do with c'tor incomplete)
    std::make_shared<type>(IF_Basic,IF_Boolean)->connect(cxx,"boolean");
    auto character = std::make_shared<type>(IF_Basic,IF_Character);
    character->connect(cxx,"character");
    std::make_shared<type>(IF_Basic,IF_DoubleReal)->connect(cxx,"doublereal");
    std::make_shared<type>(IF_Basic,IF_Integer)->connect(cxx,"integer");
    std::make_shared<type>(IF_Basic,IF_Null)->connect(cxx,"null");
    std::make_shared<type>(IF_Basic,IF_Real)->connect(cxx,"real");
    std::make_shared<type>(IF_Basic,IF_WildBasic)->connect(cxx,"wildbasic");
    std::make_shared<type>(IF_Wild,0)->connect(cxx,"wild");
    std::make_shared<type>(IF_Array,0,character)->connect(cxx,"string");
  }
  
  // Copy the type names (if any) into the dict (for getattro)
  for(ssize_t i=0;i<PyList_GET_SIZE(cxx->types.borrow());++i) {
    PyObject* T = PyList_GET_ITEM(cxx->types.borrow(),i);
    auto p = type::borrow(T);
    PyObject* na = PyDict_GetItemString(p->pragmas.borrow(),"na");
    if (na) {
      PyDict_SetItem(cxx->dict.borrow(),na,T);
    }
  }
  return 0;
}

PyObject* module::getattro(PyObject* o,PyObject* attr) {
  auto cxx = reinterpret_cast<python*>(o)->cxx;
  PyObject* p /*borrowed*/ = PyDict_GetItem(cxx->dict.borrow(),attr);
  if (p) { Py_INCREF(p); return p; }
  return PyObject_GenericGetAttr(o,attr);
}

module:: module(python* self, PyObject* args,PyObject* kwargs)
  : types(nullptr), pragmas(nullptr), functions(nullptr), dict(nullptr)
{
  types = PyList_New(0);
  if (!types) throw PyErr_Occurred();

  pragmas = PyDict_New();
  if (!pragmas) throw PyErr_Occurred();

  functions = PyList_New(0);
  if (!functions) throw PyErr_Occurred();

  dict = PyDict_New();
  if (!dict) throw PyErr_Occurred();
}

PyNumberMethods module::as_number;
PySequenceMethods module::as_sequence;
