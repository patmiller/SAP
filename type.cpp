#include "IFX.h"
#include "type.h"
#include "module.h"

void type::setup() {
  type::as_number.nb_int = type::nb_int;
}

PyObject* type::nb_int(PyObject* self) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return PyInt_FromLong(*cxx);
}

PyTypeObject type::Type;

char const* type::doc = "TBD Type";

PyMethodDef type::methods[] = {
  {(char*)"chain",type::chain,METH_VARARGS,(char*)"TBD: chain chain"},
  {nullptr}
};

PyObject* type::chain(PyObject* self, PyObject* args) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  switch(cxx->code) {
  case IF_Field:
  case IF_Tag:
  case IF_Tuple:
    break;

  case IF_Record:
  case IF_Union:
    cxx = cxx->parameter1.lock();
    break;

  default:
    return PyErr_Format(PyExc_TypeError,"unchainable type");
  }

  // How many?
  ssize_t n = 0;
  for(auto p=cxx; p; p = p->parameter2.lock()) n++;

  PyOwned result(PyTuple_New(n));
  if (!result) return nullptr;
  ssize_t i = 0;
  for(auto p=cxx; p; p = p->parameter2.lock()) {
    PyObject* T /*owned*/ = p->parameter1.lock()->lookup();
    PyTuple_SET_ITEM(result.borrow(),i,T);
    ++i;
  }
  return result.incref();
}

PyGetSetDef type::getset[] = {
  {(char*)"code",type::get_code,nullptr,(char*)"type code",nullptr},
  {(char*)"aux",type::get_aux,nullptr,(char*)"basic subcode (if any)",nullptr},
  {(char*)"parameter1",type::get_parameter1,nullptr,(char*)"first subtype (if any)",nullptr},
  {(char*)"parameter2",type::get_parameter2,nullptr,(char*)"second subtype (if any)",nullptr},
  {(char*)"pragmas",type::get_pragmas,nullptr,(char*)"pragma dictionary",nullptr},
  {(char*)"name",type::get_name,type::set_name,(char*)"Type name (linked to %na pragma)",nullptr},
  {(char*)"if1",type::get_if1,nullptr,(char*)"IF1 code for entire type",nullptr},
  {(char*)"label",type::get_label,nullptr,(char*)"type label",nullptr},
  {nullptr}
};

PyObject* type::get_code(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return PyInt_FromLong(cxx->code);
}

PyObject* type::get_aux(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return PyInt_FromLong(cxx->aux);
}

PyObject* type::get_parameter1(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  std::shared_ptr<type> p1 = cxx->parameter1.lock();
  if (p1) return p1->lookup();
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* type::get_parameter2(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  std::shared_ptr<type> p2 = cxx->parameter2.lock();
  if (p2) return p2->lookup();
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* type::get_pragmas(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_pragmas");
}

PyObject* type::get_name(PyObject* self,void*) {
  STATIC_STR(NA,"na");
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  PyObject* name /*borrowed*/ = PyDict_GetItem(cxx->pragmas.borrow(),NA);
  if (!name) name = Py_None;
  Py_INCREF(name);
  return name;
}
int type::set_name(PyObject* self,PyObject* name,void*) {
  STATIC_STR(NA,"na");
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  if (!name || name == Py_None) {
    PyDict_DelItem(cxx->pragmas.borrow(),NA);
  } else {
    PyDict_SetItem(cxx->pragmas.borrow(),NA,name);
  }
  return 0;
}
PyObject* type::get_if1(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;

  long label = *cxx;

  PyOwned result(nullptr);
  switch(cxx->code) {
  case IF_Wild:
    result = PyString_FromFormat("T %ld %ld",label,cxx->code);
    if (!result) return nullptr;
    break;

    // Basic uses the aux, not the weak fields
  case IF_Basic:
    result = PyString_FromFormat("T %ld %ld %ld",label,cxx->code,cxx->aux);
    if (!result) return nullptr;
    break;

    // One parameter case
  case IF_Array:
  case IF_Multiple:
  case IF_Record:
  case IF_Stream:
  case IF_Union: {
    std::shared_ptr<type> parameter1 = cxx->parameter1.lock();
    long p1 = 0;
    if (parameter1) p1 = *parameter1;

    result = PyString_FromFormat("T %ld %ld %ld",label,cxx->code,p1);
    if (!result) return nullptr;
    break;
  }

    // Two parameter case for the rest
  default: {
    std::shared_ptr<type> parameter1 = cxx->parameter1.lock();
    long p1 = 0;
    if (parameter1) p1 = *parameter1;

    std::shared_ptr<type> parameter2 = cxx->parameter2.lock();
    long p2 = 0;
    if (parameter2) p2 = *parameter2;

    result = PyString_FromFormat("T %ld %ld %ld %ld",label,cxx->code,p1,p2);
    if (!result) return nullptr;
  }    
  }

  PyOwned prags(pragma_string(cxx->pragmas.borrow()));
  if (!prags) return nullptr;

  PyObject* final = result.incref();
  PyString_Concat(&final,prags.borrow());
  return final;
}

type::operator long() {
  std::shared_ptr<module> m = weakmodule.lock();
  PyObject* types = m->types.borrow();
  for(ssize_t i=0;i<PyList_GET_SIZE(types);++i) {
    type::python* p = reinterpret_cast<type::python*>(PyList_GET_ITEM(types,i));
    if (p->cxx.get() == this) return i+1;
  }
  return 0;
}

PyObject* type::get_label(PyObject* self,void*) {
  auto cxx = reinterpret_cast<python*>(self)->cxx;
  return PyInt_FromLong(*cxx);
}

PyObject* type::string(std::weak_ptr<type>& weak) {
  auto T = weak.lock();
  if (!T) {
    return PyString_FromString("");
  }
  return string(nullptr,T);
}
PyObject* type::string(PyObject*,std::shared_ptr<type>& cxx) {
  STATIC_STR(ARROW,"->");
  STATIC_STR(NA,"na");
  STATIC_STR(COLON,":");
  static const char* flavor[] = {"array","basic","field","function","multiple","record","stream","tag","tuple","union"};

  // If this type has a name, use it (not for tuple, field, tag)
  if (cxx->code != IF_Tuple && cxx->code != IF_Field && cxx->code != IF_Tag) {
    PyObject* na = PyDict_GetItemString(cxx->pragmas.borrow(),"na");
    if (na) { Py_INCREF(na); return na; }
  }

  switch(cxx->code) {
  case IF_Array:
  case IF_Multiple:
  case IF_Record:
  case IF_Stream:
  case IF_Union:{
    PyOwned basestring(type::string(cxx->parameter1));
    if (!basestring) return nullptr;
    return PyString_FromFormat("%s[%s]",flavor[cxx->code],PyString_AS_STRING(basestring.borrow()));
  }

  case IF_Basic:
    return PyString_FromFormat("Basic(%ld)",cxx->aux);

  case IF_Wild: {
    return PyString_FromString("wild");
  }

  case IF_Field:
  case IF_Tag:
  case IF_Tuple: {
    PyObject* name = PyDict_GetItem(cxx->pragmas.borrow(),NA);
    PyOwned answer(PyString_FromString(""));
    PyObject* result = answer.incref();
    if (name) {
      PyString_Concat(&result,name);
      if (!result) return nullptr;
      PyString_Concat(&result,COLON);
      if (!result) return nullptr;
    }
    PyOwned element(string(cxx->parameter1));
    if (!element) {Py_DECREF(result); return nullptr;}
    PyString_Concat(&result,element.borrow());
    if (!result) return nullptr;
    if (cxx->parameter2.lock()) {
      PyString_Concat(&result,ARROW);
      if (!result) return nullptr;
      PyObject* tail = string(cxx->parameter2);
      if (!tail) {Py_DECREF(result); return nullptr;}
      PyString_ConcatAndDel(&result,tail);
    }
    return result;
  }

  case IF_Function:
    PyOwned inputs(type::string(cxx->parameter1));
    if (!inputs) return nullptr;
    PyOwned outputs(type::string(cxx->parameter2));
    if (!outputs) return nullptr;
    return PyString_FromFormat("%s[%s returns %s]",flavor[cxx->code],PyString_AS_STRING(inputs.borrow()),PyString_AS_STRING(outputs.borrow()));
  }


  return PyErr_Format(PyExc_NotImplementedError,"unknown type code %ld",cxx->code);
}

type::type(python* self, PyObject* args,PyObject* kwargs)
  : code(0),
    aux(0),
    pragmas(nullptr)
{
  throw PyErr_Format(PyExc_TypeError,"Cannot create new types this way.  see Module.addtype()");
}

type::type(long code,long aux,std::shared_ptr<type> p1,std::shared_ptr<type> p2)
  : //weakmodule(nullptr),
    code(code),
    aux(aux),
    parameter1(p1),
    parameter2(p2),
    pragmas(nullptr)
{
  pragmas = PyDict_New();
  if (!pragmas) throw PyErr_Occurred();
}

ssize_t type::connect(std::shared_ptr<module>& module,char const* name) {
  STATIC_STR(NA,"na");

  // If we already have this one, don't add another variant, just return
  // the internal offset
  ssize_t n = PyList_GET_SIZE(module->types.borrow());
  for(ssize_t i=0; i < n; ++i) {
    PyObject* T = PyList_GET_ITEM(module->types.borrow(),i);
    auto p = borrow(T);
    if (p->code == code &&
	p->aux == aux &&
	p->parameter1.lock() == parameter1.lock() &&
	p->parameter2.lock() == parameter2.lock()) {
      return i;
    }
  }

  // We can finally set our owner
  weakmodule = module;

  // Add it in (conveniently at position n)
  PyOwned asPython(package());
  if (PyList_Append(module->types.borrow(),asPython.incref()) < 0) return -1;

  // Set the name if we have it (just the main module setup)
  if (name) {
    PyOwned p(PyString_FromString(name));
    if (p) PyDict_SetItem(pragmas.borrow(),NA,p.borrow());
  }
  return n;
}

// ----------------------------------------------------------------------
// \brief find the type in the module's type list
// ----------------------------------------------------------------------
PyObject* type::lookup() {
  std::shared_ptr<module> m = weakmodule.lock();
  if (!m) return PyErr_Format(PyExc_RuntimeError,"disconnected");

  // Look in the list of types for this one
  for(ssize_t i=0;i<PyList_GET_SIZE(m->types.borrow());++i) {
    PyObject* T = PyList_GET_ITEM(m->types.borrow(),i);
    if (!PyObject_IsInstance(T,(PyObject*)&Type)) continue;

    auto cxx = reinterpret_cast<python*>(T)->cxx;
    if (cxx.get() == this) {
      Py_INCREF(T);
      return T;
    }
  }
  return PyErr_Format(PyExc_RuntimeError,"disconnected");
}

PyNumberMethods type::as_number;
PySequenceMethods type::as_sequence;
