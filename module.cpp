#include "module.h"
#include "type.h"
#include "graph.h"

PyTypeObject module::Type;

char const* module::doc = "TBD Module";

PyMethodDef module::methods[] = {
  {(char*)"addtype",module::addtype,METH_VARARGS,"add a type"},
  {(char*)"addfunction",module::addtype,METH_VARARGS,"add a type"},
  {nullptr}
};

PyObject* module::addtype(PyObject* pySelf,PyObject* args) {
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* module::addfunction(PyObject* pySelf,PyObject* args) {
  Py_INCREF(Py_None);
  return Py_None;
}

PyGetSetDef module::getset[] = {
  {(char*)"if1",module::get_if1,nullptr,(char*)"IF1 code for entire module",nullptr},
  {(char*)"types",module::get_types,nullptr,(char*)"types",nullptr},
  {(char*)"pragmas",module::get_pragmas,nullptr,(char*)"whole program pragma dictionary (char->string)",nullptr},
  {(char*)"functions",module::get_functions,nullptr,(char*)"top level functions",nullptr},
  {nullptr}
};

PyObject* module::get_if1(PyObject* self,void*) {
  STATIC_STR(NEWLINE,"\n");
  STATIC_STR(C_DOLLAR_SPACESPACE,"C$  ");
  STATIC_STR(SPACE," ");
  
  auto cxx = reinterpret_cast<python*>(self)->cxx;
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
    PyErr_Format(PyExc_NotImplementedError,"module from source");
    return -1;
    //PyObject* split /*owned*/ = PyObject_CallMethod(source,(char*)"split",(char*)"s","\n");
    //if (!split) return -1;
    //if (module_read_types(self,split) != 0) { Py_DECREF(split); return -1; }
  } else {

    // We create the built in types here (not in c'tor) because we need to set a weak
    // pointer to the module (can't do with c'tor incomplete)
    auto boolean = std::make_shared<type>(cxx,IF_Basic,IF_Boolean,"boolean");
    PyObject* p = *boolean;
    if (PyList_Append(cxx->types.borrow(),p) != 0) throw PyErr_Occurred();
    Py_DECREF(p);

    auto character = std::make_shared<type>(cxx,IF_Basic,IF_Character,"character");
    p = *character;
    if (PyList_Append(cxx->types.borrow(),p) != 0) throw PyErr_Occurred();
    Py_DECREF(p);

    auto doublereal = std::make_shared<type>(cxx,IF_Basic,IF_DoubleReal,"doublereal");
    p = *doublereal;
    if (PyList_Append(cxx->types.borrow(),p) != 0) throw PyErr_Occurred();
    Py_DECREF(p);

    auto integer = std::make_shared<type>(cxx,IF_Basic,IF_Integer,"integer");
    p = *integer;
    if (PyList_Append(cxx->types.borrow(),p) != 0) throw PyErr_Occurred();
    Py_DECREF(p);

    auto null = std::make_shared<type>(cxx,IF_Basic,IF_Null,"null");
    p = *null;
    if (PyList_Append(cxx->types.borrow(),p) != 0) throw PyErr_Occurred();
    Py_DECREF(p);

    auto real = std::make_shared<type>(cxx,IF_Basic,IF_Real,"real");
    p = *real;
    if (PyList_Append(cxx->types.borrow(),p) != 0) throw PyErr_Occurred();
    Py_DECREF(p);

    auto wildbasic = std::make_shared<type>(cxx,IF_Basic,IF_WildBasic,"wildbasic");
    p = *wildbasic;
    if (PyList_Append(cxx->types.borrow(),p) != 0) throw PyErr_Occurred();
    Py_DECREF(p);

    auto wild = std::make_shared<type>(cxx,IF_Wild,0,"wild");
    p = *wild;
    if (PyList_Append(cxx->types.borrow(),p) != 0) throw PyErr_Occurred();
    Py_DECREF(p);

    auto string = std::make_shared<type>(cxx,IF_Array,0,"string",character,nullptr);
    p = *string;
    if (PyList_Append(cxx->types.borrow(),p) != 0) throw PyErr_Occurred();
    Py_DECREF(p);
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
