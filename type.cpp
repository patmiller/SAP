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
  {nullptr}
};

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

PyObject* type::get_code(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_code");
}
PyObject* type::get_aux(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_aux");
}
PyObject* type::get_parameter1(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_parameter1");
}
PyObject* type::get_parameter2(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_parameter2");
}
PyObject* type::get_pragmas(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_pragmas");
}
PyObject* type::get_name(PyObject* pySelf,void*) {
  return PyErr_Format(PyExc_NotImplementedError,"get_name");
}
int type::set_name(PyObject* pySelf,PyObject*,void*) {
  PyErr_Format(PyExc_NotImplementedError,"set_name");
  return -1;
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
    long p1 = *parameter1;
    result = PyString_FromFormat("T %ld %ld %ld",label,cxx->code,p1);
    if (!result) return nullptr;
    break;
  }

    // Two parameter case for the rest
  default: {
    std::shared_ptr<type> parameter1 = cxx->parameter1.lock();
    long p1 = *parameter1;
    std::shared_ptr<type> parameter2 = cxx->parameter2.lock();
    long p2 = *parameter2;
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

PyObject* type::string(PyObject*,std::shared_ptr<type>& cxx) {
  static const char* flavor[] = {"array","basic","field","function","multiple","record","stream","tag","tuple","union"};

  // If this type has a name, use it (not for tuple, field, tag)
  if (cxx->code != IF_Tuple && cxx->code != IF_Field && cxx->code != IF_Tag) {
    PyObject* na = PyDict_GetItemString(cxx->pragmas.borrow(),"na");
    if (na) { Py_INCREF(na); return na; }
  }

  switch(cxx->code) {
  case IF_Record:
  case IF_Union: {
    break;
#if 0
    PyObject* basetype /*borrowed*/ = PyWeakref_GET_OBJECT(self->weak1);
    if (basetype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected structure");
    PyObject* base /*owned*/ = PyObject_Str(basetype);
    if (!base) return nullptr;
    PyObject* result = PyString_FromFormat("%s%s",flavor[self->code],PyString_AS_STRING(base));
    Py_DECREF(base);
    return result;
#endif
  }

  case IF_Array:
  case IF_Multiple:
  case IF_Stream: {
    auto base = cxx->parameter1.lock();
    if (!base) return PyErr_Format(PyExc_RuntimeError,"malformed %s",flavor[cxx->code]);
    PyOwned basestring(type::string(nullptr,base));
    if (!basestring) return nullptr;
    return PyString_FromFormat("%s[%s]",flavor[cxx->code],PyString_AS_STRING(basestring.borrow()));
  }

  case IF_Basic:
    return PyString_FromFormat("Basic(%ld)",cxx->aux);

  case IF_Field:
  case IF_Tag:
  case IF_Tuple: {
    break;
#if 0
    PyObject* result /*owned*/ = PyString_FromString("");
    if (!result) return nullptr;
    PyString_Concat(&result,LPAREN);
    if (!result) return nullptr;

    for(;self;) {
      PyObject* name /*borrowed*/= PyDict_GetItemString(self->pragmas,"na");
      if (name) {
        PyObject* fname = PyObject_Str(name);
        if (!fname) { Py_DECREF(result); return nullptr; }
        PyString_ConcatAndDel(&result,fname);
        if (!result) return nullptr;
        PyString_Concat(&result,COLON);
        if (!result) return nullptr;
      }

      PyObject* elementtype /*borrowed*/ = PyWeakref_GET_OBJECT(self->weak1);
      if (elementtype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected tuple element type");

      PyObject* element /*owned*/ = PyObject_Str(elementtype);
      if (!element) return nullptr;

      PyString_ConcatAndDel(&result,element);
      if (!result) return nullptr;

      PyObject* next = self->weak2?PyWeakref_GET_OBJECT(self->weak2):nullptr;
      if (next == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected tuple element link");
      self = reinterpret_cast<IF1_TypeObject*>(next);
      if (self) PyString_Concat(&result,COMMA);
      if (!result) return nullptr;
    }
    PyString_Concat(&result,RPAREN);
    return result;
#endif
  }
  case IF_Function: {
    break;
#if 0
    PyObject* intype = (self->weak1)?PyWeakref_GET_OBJECT(self->weak1):nullptr;
    if (intype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected function input");

    PyObject* outtype = PyWeakref_GET_OBJECT(self->weak2);
    if (outtype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected function output");

    PyObject* ins /*owned*/ = (intype)?PyObject_Str(intype):PyString_FromString("");
    if (!ins) return nullptr;
    PyObject* outs /*owned*/ = PyObject_Str(outtype);
    if (!outs) { Py_DECREF(ins); return nullptr; }

    PyString_Concat(&ins,ARROW);
    PyString_ConcatAndDel(&ins,outs);
    return ins;
#endif
  }
  case IF_Wild: {
    return PyString_FromString("wild");
  }
    
  }

  return PyErr_Format(PyExc_NotImplementedError,"type code %ld not done yet",cxx->code);
}

type::type(python* self, PyObject* args,PyObject* kwargs)
  : code(0),
    aux(0),
    pragmas(nullptr)
{
  throw PyErr_Format(PyExc_TypeError,"Cannot create new types this way.  see Module.addtype()");
}

type::type(std::shared_ptr<module>& m,long code,long aux,char const* name,std::shared_ptr<type> p1,std::shared_ptr<type> p2)
  : weakmodule(m),
    code(code),
    aux(aux),
    parameter1(p1),
    parameter2(p2),
    pragmas(nullptr)
{
  pragmas = PyDict_New();
  if (!pragmas) throw PyErr_Occurred();

  if (name) {
    PyOwned na(PyString_FromString(name));
    if (!na) throw PyErr_Occurred();
    if (PyDict_SetItemString(pragmas.borrow(),"na",na.borrow()) != 0) throw PyErr_Occurred();
  }
}
PyNumberMethods type::as_number;
PySequenceMethods type::as_sequence;
