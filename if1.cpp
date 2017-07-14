#include "Python.h"
#include "structmember.h"
#include "IFX.h"
#include <map>
#include <string>

PyObject* OPNAMES = NULL;
PyObject* OPCODES = NULL;

PyObject* FUNCTIONTYPE = nullptr;

PyObject* ADDTYPE = NULL; // addtype
PyObject* ARROW = NULL; // ->
PyObject* COLON = NULL; // :
PyObject* COMMA = NULL; // ,
PyObject* LPAREN = NULL; // (
PyObject* RPAREN = NULL; // )
PyObject* SPACE = NULL;
PyObject* DQUOTE = NULL; // "
PyObject* NEWLINE = NULL;


enum literal_parser_state_t {
  ERROR = 0,
  START,
  INTEGER,
  REAL,
  REAL_EXP_OPTSIGN,
  REAL_EXP_DIGIT,
  REAL_EXP,
  DOUBLEREAL_EXP_OPTSIGN,
  DOUBLEREAL_EXP_DIGIT,
  DOUBLEREAL_EXP,
  NIL0,
  NIL1,
  NIL2,
  TRUE0,
  TRUE1,
  TRUE2,
  TRUE3,
  FALSE0,
  FALSE1,
  FALSE2,
  FALSE3,
  FALSE4,
  CHAR_BODY,
  CHAR_ESCAPE,
  CHAR_END,
  CHAR_NULL,
  STRING_BODY,
  STRING_ESCAPE,
  STRING_END,
  NSTATES,		// Just marks the last state
  BOOLEAN_LITERAL = -1,
  CHAR_LITERAL = -2,
  DOUBLEREAL_LITERAL = -3,
  INTEGER_LITERAL = -4,
  NULL_LITERAL = -5,
  REAL_LITERAL = -6,
  STRING_LITERAL = -7
};

bool initialize_literal_parser_table(literal_parser_state_t table[]) {
  // Initialize all entries to ERROR
  for(int i=0;i<NSTATES*256;++i) table[i] = ERROR;

  // ----------------------------------------------------------------------
  // In the start state, we figure out what kind of literal this "looks like"
  // ----------------------------------------------------------------------
  table[START*256+'n'] = NIL0;
  table[START*256+'N'] = NIL0;
  table[START*256+'t'] = TRUE0;
  table[START*256+'T'] = TRUE0;
  table[START*256+'f'] = FALSE0;
  table[START*256+'F'] = FALSE0;
  table[START*256+'0'] = INTEGER;
  table[START*256+'1'] = INTEGER;
  table[START*256+'2'] = INTEGER;
  table[START*256+'3'] = INTEGER;
  table[START*256+'4'] = INTEGER;
  table[START*256+'5'] = INTEGER;
  table[START*256+'6'] = INTEGER;
  table[START*256+'7'] = INTEGER;
  table[START*256+'8'] = INTEGER;
  table[START*256+'9'] = INTEGER;
  table[START*256+'+'] = INTEGER;
  table[START*256+'-'] = INTEGER;
  table[START*256+'\''] = CHAR_BODY;
  table[START*256+'"'] = STRING_BODY;

  // ----------------------------------------------------------------------
  // Booleans are true or false
  // ----------------------------------------------------------------------
  table[TRUE0*256+'r'] = TRUE1;
  table[TRUE0*256+'R'] = TRUE1;
  table[TRUE1*256+'u'] = TRUE2;
  table[TRUE1*256+'U'] = TRUE2;
  table[TRUE2*256+'e'] = TRUE3;
  table[TRUE2*256+'E'] = TRUE3;
  table[TRUE3*256+0] = BOOLEAN_LITERAL;

  table[FALSE0*256+'a'] = FALSE1;
  table[FALSE0*256+'A'] = FALSE1;
  table[FALSE1*256+'l'] = FALSE2;
  table[FALSE1*256+'L'] = FALSE2;
  table[FALSE2*256+'s'] = FALSE3;
  table[FALSE2*256+'S'] = FALSE3;
  table[FALSE3*256+'e'] = FALSE4;
  table[FALSE3*256+'E'] = FALSE4;
  table[FALSE4*256+0] = BOOLEAN_LITERAL;

  // ----------------------------------------------------------------------
  // Characters are single quote with an escape
  // ----------------------------------------------------------------------
  for(char c=' '; c < 127; ++c) {
    if (c == '\\') {
      table[CHAR_BODY*256+c] = CHAR_ESCAPE;
    } else if (c == '\'') {
      table[CHAR_BODY*256+c] = ERROR;
    } else {
      table[CHAR_BODY*256+c] = CHAR_END;
    }
  }

  for(char c=' '; c < 127; ++c) {
    table[CHAR_ESCAPE*256+c] = CHAR_END;
  }

  table[CHAR_END*256+'\''] = CHAR_NULL;
  table[CHAR_NULL*256+0] = CHAR_LITERAL;

  // ----------------------------------------------------------------------
  // DoubleReal
  // ----------------------------------------------------------------------      
  table[DOUBLEREAL_EXP_OPTSIGN*256+0] = DOUBLEREAL_LITERAL;
  table[DOUBLEREAL_EXP_OPTSIGN*256+'+'] = DOUBLEREAL_EXP_DIGIT;
  table[DOUBLEREAL_EXP_OPTSIGN*256+'-'] = DOUBLEREAL_EXP_DIGIT;
  for(char c='0'; c <= '9'; ++c) {
    table[DOUBLEREAL_EXP_OPTSIGN*256+c] = DOUBLEREAL_EXP;
  }

  for(char c='0'; c <= '9'; ++c) {
    table[DOUBLEREAL_EXP_DIGIT*256+c] = DOUBLEREAL_EXP;
  }

  table[DOUBLEREAL_EXP*256+0] = DOUBLEREAL_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[DOUBLEREAL_EXP*256+c] = DOUBLEREAL_EXP;
  }

  // ----------------------------------------------------------------------
  // Integer
  // ----------------------------------------------------------------------  
  table[INTEGER*256+0] = INTEGER_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[INTEGER*256+c] = INTEGER;
  }
  table[INTEGER*256+'.'] = REAL;
  table[INTEGER*256+'e'] = REAL_EXP_OPTSIGN;
  table[INTEGER*256+'E'] = REAL_EXP_OPTSIGN;
  table[INTEGER*256+'d'] = DOUBLEREAL_EXP_OPTSIGN;
  table[INTEGER*256+'D'] = DOUBLEREAL_EXP_OPTSIGN;

  // ----------------------------------------------------------------------
  // Null
  // ----------------------------------------------------------------------      
  table[NIL0*256+'i'] = NIL1;
  table[NIL0*256+'I'] = NIL1;
  table[NIL1*256+'l'] = NIL2;
  table[NIL1*256+'L'] = NIL2;
  table[NIL2*256+  0] = NULL_LITERAL;

  // ----------------------------------------------------------------------
  // Real
  // ----------------------------------------------------------------------      
  table[REAL*256+0] = REAL_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[REAL*256+c] = REAL;
  }
  table[REAL*256+'e'] = REAL_EXP_OPTSIGN;
  table[REAL*256+'E'] = REAL_EXP_OPTSIGN;
  table[REAL*256+'d'] = DOUBLEREAL_EXP_OPTSIGN;
  table[REAL*256+'D'] = DOUBLEREAL_EXP_OPTSIGN;
      
  table[REAL_EXP_OPTSIGN*256+0] = REAL_LITERAL;
  table[REAL_EXP_OPTSIGN*256+'+'] = REAL_EXP_DIGIT;
  table[REAL_EXP_OPTSIGN*256+'-'] = REAL_EXP_DIGIT;
  for(char c='0'; c <= '9'; ++c) {
    table[REAL_EXP_OPTSIGN*256+c] = REAL_EXP;
  }

  for(char c='0'; c <= '9'; ++c) {
    table[REAL_EXP_DIGIT*256+c] = REAL_EXP;
  }

  table[REAL_EXP*256+0] = REAL_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[REAL_EXP*256+c] = REAL_EXP;
  }

  // ----------------------------------------------------------------------
  // String
  // ----------------------------------------------------------------------
  for(char c=' '; c < 127; ++c) {
    if (c == '\\') {
      table[STRING_BODY*256+c] = STRING_ESCAPE;
    } else if (c == '"') {
      table[STRING_BODY*256+c] = STRING_END;
    } else {
      table[STRING_BODY*256+c] = STRING_BODY;
    }
  }
  for(char c=' '; c < 127; ++c) {
    table[STRING_ESCAPE*256+c] = STRING_BODY;
  }
  table[STRING_END*256+0] = STRING_LITERAL;

  return true;
}



// ----------------------------------------------------------------------
// InPort
// ----------------------------------------------------------------------
const char* inport_doc = "TBD: input port";

static PyTypeObject IF1_InPortType;

typedef struct {
  PyObject_HEAD

  PyObject* weakdst;
  ssize_t port;

} IF1_InPortObject;

static PyNumberMethods inport_number;

// ----------------------------------------------------------------------
// PortInfo
// ----------------------------------------------------------------------
struct PortInfo {
  int x;
};

// ----------------------------------------------------------------------
// Edge
// ----------------------------------------------------------------------
struct Edge {
  std::string value;
  PyObject* literal_type;

  PyObject* weaksrc;
  long      oport;

  Edge() : literal_type(nullptr),weaksrc(nullptr),oport(0) {}
  Edge(const char* value, PyObject* type) : value(value), literal_type(type), weaksrc(nullptr), oport(0) {
    Py_XINCREF(type);
  }
  Edge(PyObject* weaksrc, long oport) : literal_type(nullptr), weaksrc(weaksrc), oport(oport) {
    Py_XINCREF(weaksrc);
  }
  ~Edge() {
    Py_XDECREF(literal_type);
    Py_XDECREF(weaksrc);
  }
  Edge(const Edge& other) {
    value = other.value;
    Py_XINCREF(literal_type = other.literal_type);

    Py_XINCREF(weaksrc = other.weaksrc);
    oport = other.oport;
  }
  Edge& operator=(const Edge& other) {
    if (&other != this) {
      value = other.value;
      Py_XINCREF(literal_type = other.literal_type);

      Py_XINCREF(weaksrc = other.weaksrc);
      oport = other.oport;
    }
    return *this;
  }

  PyObject* /*borrowed*/ type() {
    if (oport == 0) return literal_type;
    return PyErr_Format(PyExc_NotImplementedError,"have to do real edge");
  }
};

// ----------------------------------------------------------------------
// Node
// ----------------------------------------------------------------------
const char* node_doc = "TBD: node";

static PyTypeObject IF1_NodeType;

typedef struct {
  PyObject_HEAD
  PyObject* weakmodule;
  PyObject* weakparent;
  long opcode;
  
  PyObject* children;

  std::map<long,Edge>* edges;
  std::map<long,PortInfo>* outputs;

  // Weak reference support
  PyObject* weak;
} IF1_NodeObject;

PyNumberMethods node_number;

static PyMemberDef node_members[] = {
  {(char*)"opcode",T_LONG,offsetof(IF1_NodeObject,opcode),0,(char*)"op code: IFPlus, IFMinus, ..."},
  {(char*)"children",T_OBJECT_EX,offsetof(IF1_NodeObject,children),READONLY,(char*)"children of componds (or exception)"},
  {NULL}
};

// ----------------------------------------------------------------------
// Graph
// ----------------------------------------------------------------------
const char* graph_doc = "TBD: Graph (isa Node)";

static PyTypeObject IF1_GraphType;

typedef struct {
  IF1_NodeObject node;

  // Optional name & type (function graphs)
  PyObject* name;
  PyObject* type;

  // Graphs have a list of nodes (in node.children), a list of literals, and a list of edges
  PyObject* literals;
  PyObject* edges;

} IF1_GraphObject;

PyNumberMethods graph_number;

static PyMemberDef graph_members[] = {
  {(char*)"name",T_OBJECT,offsetof(IF1_GraphObject,name),0,(char*)"Optional graph name"},
  {NULL}
};


// ----------------------------------------------------------------------
// Type
// ----------------------------------------------------------------------
const char* type_doc =
  "TODO: Add docsn for type\n"
  ;

static PyTypeObject IF1_TypeType;

typedef struct {
  PyObject_HEAD
  PyObject* weakmodule;
  unsigned long code;
  unsigned long aux;
  PyObject* weak1;
  PyObject* weak2;
  PyObject* name;

  // Weak reference slot
  PyObject* weak;
} IF1_TypeObject;

static PyMemberDef type_members[] = {
  {(char*)"code",T_LONG,offsetof(IF1_TypeObject,code),READONLY,(char*)"type code: IF_Array, IF_Basic..."},
  {(char*)"name",T_OBJECT,offsetof(IF1_TypeObject,name),0,(char*)"type name or None"},
  {NULL}  /* Sentinel */
};

PyNumberMethods type_number;

// ----------------------------------------------------------------------
// Module
// ----------------------------------------------------------------------
const char* module_doc =
  "TODO: Add docs for module\n"
  ;

static PyTypeObject IF1_ModuleType;

typedef struct {
  PyObject_HEAD
  PyObject* dict;
  PyObject* types;
  PyObject* pragmas;
  PyObject* functions;

  // Weak reference slot
  PyObject* weak;
} IF1_ModuleObject;


static PyMemberDef module_members[] = {
  {(char*)"__dict__",T_OBJECT_EX,offsetof(IF1_ModuleObject,dict),0,(char*)"instance dictionary"},
  {(char*)"types",T_OBJECT_EX,offsetof(IF1_ModuleObject,types),0,(char*)"type table"},
  {(char*)"pragmas",T_OBJECT_EX,offsetof(IF1_ModuleObject,pragmas),0,(char*)"pragma set"},
  {(char*)"functions",T_OBJECT_EX,offsetof(IF1_ModuleObject,functions),0,(char*)"function graphs"},
  {NULL}  /* Sentinel */
};

/*
  ###         ###                #    
   #          #  #               #    
   #    ###   #  #   ##   ###   ###   
   #    #  #  ###   #  #  #  #   #    
   #    #  #  #     #  #  #      #    
  ###   #  #  #      ##   #       ##  
*/

void inport_dealloc(PyObject* pySelf) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);

  Py_XDECREF(self->weakdst);

  Py_TYPE(pySelf)->tp_free(pySelf);
}

PyObject* inport_str(PyObject* pySelf) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);

  PyObject* sport /*owned*/ = PyString_FromFormat("%ld",self->port);
  if (!sport) return NULL;

  PyString_Concat(&sport,COLON);
  if (!sport) return NULL;

  PyObject* dst /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakdst);
  if (dst == Py_None) { Py_DECREF(sport); return PyErr_Format(PyExc_ValueError,"in port is disconnected"); }
  PyObject* str /*owned*/ = PyObject_Str(dst);
  if (!str) { Py_DECREF(sport); return NULL; }
  
  PyString_ConcatAndDel(&sport,str);
  return sport;
}

PyObject* inport_literal(PyObject* module, PyObject* literal) {

    static literal_parser_state_t table[NSTATES*256];
    static bool table_ready = false;
    if (!table_ready) table_ready = initialize_literal_parser_table(table);

    const char* p = PyString_AS_STRING(literal);
    literal_parser_state_t state = START;
    while(state > 0) {
      state = table[state*256+*p++];
    }

    switch(state) {
    case BOOLEAN_LITERAL: return PyObject_GetAttrString(module,"boolean");
    case CHAR_LITERAL: return PyObject_GetAttrString(module,"character");
    case DOUBLEREAL_LITERAL: return PyObject_GetAttrString(module,"doublereal");
    case INTEGER_LITERAL: return PyObject_GetAttrString(module,"integer");
    case NULL_LITERAL: return PyObject_GetAttrString(module,"null");
    case REAL_LITERAL: return PyObject_GetAttrString(module,"real");
    case STRING_LITERAL: return PyObject_GetAttrString(module,"string");

    case ERROR: return PyErr_Format(PyExc_ValueError,"unparseable literal: %s at %c",PyString_AS_STRING(literal),*p);
    default:
      ;
    }
    return PyErr_Format(PyExc_NotImplementedError,"should be impossible");
}

PyObject* inport_dst(PyObject* pySelf) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);
  PyObject* dst /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakdst);
  if (dst == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected port");
  return dst;
}

PyObject* node_module(PyObject* pySelf);
PyObject* inport_lshift(PyObject* pySelf, PyObject* value) {
  IF1_InPortObject* self = reinterpret_cast<IF1_InPortObject*>(pySelf);

  // Remove any old value stored at this port
  long port = self->port;
  
  IF1_NodeObject* dst /*borrowed*/ = reinterpret_cast<IF1_NodeObject*>(inport_dst(pySelf));
  if (!dst) return nullptr;

  // Sending in a None will erase this port
  if (value == Py_None) {
    dst->edges->erase(self->port);
    Py_INCREF(Py_None);
    return Py_None;
  }

  // Do we have a good literal?  Figure out the type
  if (PyString_Check(value)) {
    PyObject* module /*borrowed*/ = node_module((PyObject*)dst);
    if (!module) return NULL;

    PyObject* T /*owned*/ = inport_literal(module,value);
    if (!T) return NULL;

    dst->edges->operator[](port) = Edge(PyString_AS_STRING(value),T);
    
    return T;
  }

  return PyErr_Format(PyExc_NotImplementedError,"lshift for inport << oport");
}

/*
  #  #           #        
  ## #           #        
  ## #   ##    ###   ##   
  # ##  #  #  #  #  # ##  
  # ##  #  #  #  #  ##    
  #  #   ##    ###   ##  
*/

int node_init(PyObject* pySelf, PyObject* module, long opcode) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);

  self->weakmodule = PyWeakref_NewRef(module,0);
  if (!self->weakmodule) return -1;

  self->opcode = opcode;

  self->children = PyList_New(0);
  if (!self->children) return -1;

  self->edges = new std::map<long,Edge>;
  self->outputs = new std::map<long,PortInfo>;

  return 0;
}

void node_dealloc(PyObject* pySelf) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  if (self->weak) PyObject_ClearWeakRefs(pySelf);

  Py_XDECREF(self->weakmodule);
  Py_XDECREF(self->weakparent);
  Py_XDECREF(self->children);

  delete self->edges;
  delete self->outputs;
  Py_TYPE(pySelf)->tp_free(pySelf);
}

PyObject* node_str(PyObject* pySelf) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);

  PyObject* pyOpcode /*owned*/ = PyLong_FromLong(self->opcode);
  if (not pyOpcode) return NULL;

  PyObject* gstr /*borrowed*/ = PyDict_GetItem(OPCODES,pyOpcode);
  Py_DECREF(pyOpcode);
  Py_XINCREF(gstr);
  if (!gstr) {
    gstr = PyString_FromFormat("<opcode %ld>",self->opcode);
  }
  return gstr;
}

static PyObject* node_inport(PyObject* pySelf, PyObject* args, PyObject* kwargs) {
  PyObject* p /*owned*/ = PyType_GenericNew(&IF1_InPortType,NULL,NULL);
  IF1_InPortObject* port = reinterpret_cast<IF1_InPortObject*>(p);

  if (!PyArg_ParseTuple(args,"l",&port->port)) return NULL;
  if (port->port <= 0) return PyErr_Format(PyExc_ValueError,"port must be > 0 (%ld)",port->port);

  port->weakdst = PyWeakref_NewRef(pySelf,0);
  if (!port->weakdst) { Py_DECREF(p); return NULL; }
  return p;
}

PyObject* node_module(PyObject* pySelf) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  PyObject* module /*borrowed*/ = PyWeakref_GET_OBJECT(self->weakmodule);
  if (module == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected node");
  return module;
}

PyObject* node_int(PyObject* pySelf) {
  return PyInt_FromLong(88888);
}

PyObject* node_positive(PyObject* pySelf) {
  return PyErr_Format(PyExc_NotImplementedError,"not done with node's graph");
}

PyObject* node_inedge_if1(PyObject* pySelf,PyObject** pif1) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  PyObject* if1 = *pif1;
  for(std::map<long,Edge>::iterator ii = self->edges->begin(), end = self->edges->end();
	ii != end; ++ii) {
    PyObject* eif1 = nullptr;
    long port = ii->first;
    Edge& e = ii->second;
    if (e.oport) {
      eif1 = PyString_FromString("E ? ? ? ? ?\n");
    } else {
      PyObject* nodelabel /*owned*/ = PyNumber_Int(pySelf);
      if (!nodelabel) { Py_DECREF(if1); return NULL; }
      long label = PyLong_AsLong(nodelabel);
      Py_DECREF(nodelabel);
      if (label < 0 && PyErr_Occurred()) { Py_DECREF(if1); return NULL; }

      PyObject* typelabel /*owned*/ = PyNumber_Int(e.literal_type);
      if (!typelabel) { Py_DECREF(if1); return NULL; }
      long T = PyLong_AsLong(typelabel);
      Py_DECREF(typelabel);
      if (T < 0 && PyErr_Occurred()) { Py_DECREF(if1); return NULL; }

      eif1 = PyString_FromFormat("L      %ld %ld %ld \"%s\"\n",label,port,T,e.value.c_str());
    }
    if (!eif1) { Py_DECREF(if1); return NULL; }
    PyString_ConcatAndDel(pif1,eif1);
    if (!if1) return NULL;
  }
  return if1;
}

PyObject* node_get_if1(PyObject* pySelf,void*) {
  IF1_NodeObject* self = reinterpret_cast<IF1_NodeObject*>(pySelf);
  PyObject* if1 /*owned*/ = PyString_FromFormat("N %ld\n",self->opcode);
  if (!if1) return NULL;
  return node_inedge_if1(pySelf,&if1);
}

PyGetSetDef node_getset[] = {
  {(char*)"if1",node_get_if1,nullptr,(char*)"IF1 representation (including edges)",nullptr},
  {nullptr}
};


/*
   ##                     #     
  #  #                    #     
  #     ###    ###  ###   ###   
  # ##  #  #  #  #  #  #  #  #  
  #  #  #     # ##  #  #  #  #  
   ###  #      # #  ###   #  #  
                    #           
 */

void graph_init(PyObject* pySelf,
		PyObject* module,
		long opcode,
		PyObject* name,
		PyObject* type
		) {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);
  node_init(pySelf,module,opcode);

  Py_XINCREF( self->name = name );
  Py_XINCREF( self->type = type );
}

void graph_dealloc(PyObject* pySelf) {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);

  Py_XDECREF(self->name);
  node_dealloc(pySelf);
}

PyObject* graph_int(PyObject* pySelf) {
  return PyInt_FromLong(0);
}

PyObject* graph_positive(PyObject* pySelf) {
  Py_INCREF(pySelf);
  return pySelf;
}

PyObject* graph_get_type(PyObject* pySelf,void*) {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);
  // Did we set an explicit type?
  if (self->type) {
    Py_INCREF(self->type);
    return self->type;
  }

  // If not a function graph, we return None here
  if (self->node.opcode != IFXGraph && self->node.opcode != IFLGraph) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // function inputs (graph outputs) are optional
  ssize_t ninputs = self->node.outputs->size();
  PyObject* inputs = PyTuple_New(ninputs);
  if (!inputs) return nullptr;
  for(std::map<long,PortInfo>::iterator p=self->node.outputs->begin(), end = self->node.outputs->end();
      p != end; ++p) {
    Py_DECREF(inputs);
    return PyErr_Format(PyExc_NotImplementedError,"not ready for function outputs");
  }

  // Do we have function outputs (graph inputs)?  Are they wired with no gaps?
  ssize_t noutputs = self->node.edges->size();
  if ( noutputs == 0 ) {
    Py_DECREF(inputs);
    Py_INCREF(Py_None);
    return Py_None;
  }
  if (self->node.edges->rbegin()->first != noutputs) {
    Py_DECREF(inputs);
    return PyErr_Format(PyExc_ValueError,
			"Function produces %ld values, but the high port is %ld",noutputs,self->node.edges->rbegin()->first);
  }
  PyObject* outputs = PyTuple_New(self->node.edges->size());
  if (!outputs) { Py_DECREF(inputs); return nullptr; }
  for(std::map<long,Edge>::iterator ii=self->node.edges->begin(), end=self->node.edges->end();
      ii != end; ++ii) {
    PyObject* T /*borrowed*/ = ii->second.type();
    if (!T) { Py_DECREF(inputs); Py_DECREF(outputs); return NULL; }
    PyTuple_SET_ITEM(outputs,ii->first-1,T);
    Py_INCREF(T);
  }
  PyObject* module /*borrowed*/ = node_module(pySelf);
  PyObject* T /*owned*/ = PyObject_CallMethodObjArgs(module,ADDTYPE,FUNCTIONTYPE,inputs,outputs,0);
  Py_DECREF(inputs); Py_DECREF(outputs);
  if (!T) return nullptr;
  return T;
}


int graph_set_type(PyObject* pySelf,PyObject* T, void*) {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);
  Py_XDECREF(self->type);

  // Delete type?
  if (!T || T == Py_None) {
    self->type = nullptr;
  }

  else if (!PyObject_IsInstance(T,(PyObject*)&IF1_TypeType) ) {
    PyErr_SetString(PyExc_TypeError,"graph type must be an IF1 type");
    return -1;
  }

  else {
    self->type = T;
  }
  return 0;
}


PyObject* graph_get_if1(PyObject* pySelf,void*) {
  IF1_GraphObject* self = reinterpret_cast<IF1_GraphObject*>(pySelf);
  long typecode = 0;
  char tag = 'G';
  switch(self->node.opcode) {
  case IFIGraph:
    tag = 'I';
    if (self->type) {
      PyObject* label /*owned*/ = PyNumber_Int(pySelf);
      if (!label) return nullptr;
      typecode = PyInt_AsLong(label);
      if (typecode < 0 && PyErr_Occurred()) return nullptr;
    }
    break;
  case IFXGraph: {
    tag = 'X';
    typecode = 11111;
    PyObject* T /*owned*/ = graph_get_type(pySelf,nullptr);
    if (!T) return nullptr;
    if (T == Py_None) {
      typecode = 0;
    } else {
      PyObject* label /*owned*/ = PyNumber_Int(T);
      if (!label) { Py_DECREF(T); return nullptr; }
      typecode = PyInt_AsLong(label);
      Py_DECREF(label);
      if (typecode < 0 && PyErr_Occurred()) { Py_DECREF(T); return nullptr; }
    }
    Py_DECREF(T);
  } break;
  case IFSGraph:
  case IFLGraph:
  case IFLPGraph:
  case IFRLGraph:
  default:
    tag = 'G';
  }
  
  PyObject* if1 /*owned*/ = PyString_FromFormat("%c %ld",tag,typecode);
  if (!if1) return NULL;

  if (self->name) {
    PyString_Concat(&if1,SPACE);
    if (!if1) return nullptr;
    PyString_Concat(&if1,DQUOTE);
    if (!if1) return nullptr;
    PyString_Concat(&if1,self->name);
    if (!if1) return nullptr;
    PyString_Concat(&if1,DQUOTE);
    if (!if1) return nullptr;
  }
  PyString_Concat(&if1,NEWLINE);
  if (!if1) return nullptr;

  node_inedge_if1(pySelf,&if1);
  if (!if1) return NULL;

  // Now, get the if1 for each node
  for(ssize_t i=0;i<PyList_GET_SIZE(self->node.children);++i) {
    Py_DECREF(if1);
    return PyErr_Format(PyExc_NotImplementedError,"adding nodes");
  }
  
  return if1;
}
PyGetSetDef graph_getset[] = {
  {(char*)"if1",graph_get_if1,nullptr,(char*)"IF1 representation (including edges)",nullptr},
  {(char*)"type",graph_get_type,graph_set_type,(char*)"graph type (only useful for functions)",nullptr},
  {nullptr}
};

/*
  ###                     
   #                      
   #    #  #  ###    ##   
   #    #  #  #  #  # ##  
   #     # #  #  #  ##    
   #      #   ###    ## 
   #     #    #
 */


void type_dealloc(PyObject* pySelf) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  if (self->weak) PyObject_ClearWeakRefs(pySelf);

  Py_XDECREF(self->weakmodule);
  Py_XDECREF(self->weak1);
  Py_XDECREF(self->weak2);
  Py_XDECREF(self->name);
  Py_TYPE(pySelf)->tp_free(pySelf);
}

long type_label(PyObject* pySelf) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  PyObject* w = PyWeakref_GET_OBJECT(self->weakmodule);
  if (w == Py_None) { PyErr_SetString(PyExc_RuntimeError,"disconnected type"); return -1; }
  IF1_ModuleObject* m = reinterpret_cast<IF1_ModuleObject*>(w);
  for(long label=0; label < PyList_GET_SIZE(m->types); ++label) {
    if (PyList_GET_ITEM(m->types,label) == pySelf) return label+1;
  }
  PyErr_SetString(PyExc_RuntimeError,"disconnected type");
  return -1;
}

PyObject* type_int(PyObject* self) {
  long label = type_label(self);
  if (label <= 0) {
    PyErr_SetString(PyExc_TypeError,"disconnected type");
    return NULL;
  }
  return PyInt_FromLong(label);
}

PyObject* type_str(PyObject* pySelf) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);

  static const char* flavor[] = {"array","basic","field","function","multiple","record","stream","tag","tuple","union"};

  // If we named the type, just use the name (unless it is a tuple, field, or tag)
  if (self->name && self->code != IF_Tuple && self->code != IF_Field && self->code != IF_Tag) {
    Py_INCREF(self->name); 
    return self->name;
  }

  switch(self->code) {
  case IF_Record:
  case IF_Union: {
    PyObject* basetype /*borrowed*/ = PyWeakref_GET_OBJECT(self->weak1);
    if (basetype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected structure");
    PyObject* base /*owned*/ = PyObject_Str(basetype);
    if (!base) return NULL;
    PyObject* result = PyString_FromFormat("%s%s",flavor[self->code],PyString_AS_STRING(base));
    Py_DECREF(base);
    return result;
  }

  case IF_Array:
  case IF_Multiple:
  case IF_Stream: {
    PyObject* basetype /*borrowed*/ = PyWeakref_GET_OBJECT(self->weak1);
    if (basetype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected base type");
    PyObject* base /*owned*/ = PyObject_Str(basetype);
    if (!base) return NULL;
    PyObject* result = PyString_FromFormat("%s[%s]",flavor[self->code],PyString_AS_STRING(base));
    Py_DECREF(base);
    return result;
  }
  case IF_Basic:
    return PyString_FromFormat("Basic(%ld)",self->aux);
  case IF_Field:
  case IF_Tag:
  case IF_Tuple: {
    PyObject* result /*owned*/ = PyString_FromString("");
    if (!result) return NULL;
    PyString_Concat(&result,LPAREN);
    if (!result) return NULL;

    for(;self;) {
      if (self->name) {
        PyObject* fname = PyObject_Str(self->name);
        if (!fname) { Py_DECREF(result); return NULL; }
        PyString_ConcatAndDel(&result,fname);
        if (!result) return NULL;
        PyString_Concat(&result,COLON);
        if (!result) return NULL;
      }

      PyObject* elementtype /*borrowed*/ = PyWeakref_GET_OBJECT(self->weak1);
      if (elementtype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected tuple element type");

      PyObject* element /*owned*/ = PyObject_Str(elementtype);
      if (!element) return NULL;

      PyString_ConcatAndDel(&result,element);
      if (!result) return NULL;

      PyObject* next = self->weak2?PyWeakref_GET_OBJECT(self->weak2):NULL;
      if (next == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected tuple element link");
      self = reinterpret_cast<IF1_TypeObject*>(next);
      if (self) PyString_Concat(&result,COMMA);
      if (!result) return NULL;
    }
    PyString_Concat(&result,RPAREN);
    return result;
  }
  case IF_Function: {
    PyObject* intype = (self->weak1)?PyWeakref_GET_OBJECT(self->weak1):NULL;
    if (intype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected function input");

    PyObject* outtype = PyWeakref_GET_OBJECT(self->weak2);
    if (outtype == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected function output");

    PyObject* ins /*owned*/ = (intype)?PyObject_Str(intype):PyString_FromString("");
    if (!ins) return NULL;
    PyObject* outs /*owned*/ = PyObject_Str(outtype);
    if (!outs) { Py_DECREF(ins); return NULL; }

    PyString_Concat(&ins,ARROW);
    PyString_ConcatAndDel(&ins,outs);
    return ins;
  }
  }
  return PyErr_Format(PyExc_NotImplementedError,"Code %ld",self->code);
}

PyObject* type_get_if1(PyObject* pySelf,void*) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  long label = type_label(pySelf);
  if (label < 0) return NULL;

  switch(self->code) {
    // Wild is a special case
  case IF_Wild:
    if (self->name && PyString_Check(self->name))
      return PyString_FromFormat("T %ld %ld %%na=%s",label,self->code,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld",label,self->code);

    // Basic uses the aux, not the weak fields
  case IF_Basic:
    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %ld %%na=%s",label,self->code,self->aux,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld %ld",label,self->code,self->aux);

    // One parameter case
  case IF_Array:
  case IF_Multiple:
  case IF_Record:
  case IF_Stream:
  case IF_Union: {
    PyObject* one = PyWeakref_GET_OBJECT(self->weak1);
    if (one == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected type");
    long one_label = type_label(one);
    if ( one_label < 0 ) return NULL;
    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %ld %%na=%s",label,self->code,one_label,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld %ld",label,self->code,one_label);
  }

    // Two parameter case for the rest
  default: {
    long one_label = 0;
    if (self->weak1) {
      PyObject* one = PyWeakref_GET_OBJECT(self->weak1);
      if (one == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected type");
      one_label = type_label(one);
      if ( one_label < 0 ) return NULL;
    }

    long two_label = 0;
    if (self->weak2) {
      PyObject* two = PyWeakref_GET_OBJECT(self->weak2);
      if (two == Py_None) return PyErr_Format(PyExc_RuntimeError,"disconnected type");
      two_label = type_label(two);
      if ( two_label < 0 ) return NULL;
    }

    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %ld %ld %%na=%s",label,self->code,one_label,two_label,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld %ld %ld",label,self->code,one_label,two_label);
  }
  }
  return PyErr_Format(PyExc_NotImplementedError,"finish get if1 for %ld\n",self->code);

#if 0
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);

  switch(self->code) {
    // No parameter case
  case IF_Wild:
    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %%na=%s",type_label(pySelf),self->code,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld",type_label(pySelf),self->code);

    // One parameter case (int)
  case IF_Array:
  case IF_Basic:
  case IF_Stream:
    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %ld %%na=%s",type_label(pySelf),self->code,self->parameter1,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld %ld",type_label(pySelf),self->code,self->parameter1);

  default:
    // Two parameter case
    if (self->name && PyString_Check(self->name)) 
      return PyString_FromFormat("T %ld %ld %ld %ld %%na=%s",type_label(pySelf),self->code,self->parameter1,self->parameter2,PyString_AS_STRING(self->name));
    return PyString_FromFormat("T %ld %ld %ld %ld",type_label(pySelf),self->code,self->parameter1,self->parameter2);

  }
  return PyErr_Format(PyExc_NotImplementedError,"Unknown type code %ld",self->code);
#endif
}

PyObject* type_get_aux(PyObject* pySelf,void*) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  if (self->code == IF_Basic) return PyInt_FromLong(self->aux);
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* type_get_parameter1(PyObject* pySelf,void*) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  if (!self->weak1) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyObject* strong = PyWeakref_GET_OBJECT(self->weak1);
  if (strong == Py_None) {
    return PyErr_Format(PyExc_RuntimeError,"disconnected parameter1");
  }
  Py_INCREF(strong);
  return strong;
}

PyObject* type_get_parameter2(PyObject* pySelf,void*) {
  IF1_TypeObject* self = reinterpret_cast<IF1_TypeObject*>(pySelf);
  if (!self->weak2) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyObject* strong = PyWeakref_GET_OBJECT(self->weak2);
  if (strong == Py_None) {
    return PyErr_Format(PyExc_RuntimeError,"disconnected parameter2");
  }
  Py_INCREF(strong);
  return strong;
}

PyObject* type_get_label(PyObject* self,void*) {
  return type_int(self);
}

PyGetSetDef type_getset[] = {
  {(char*)"aux",type_get_aux,NULL,(char*)"aux code (basic only)",NULL},
  {(char*)"parameter1",type_get_parameter1,NULL,(char*)"parameter1 type (if any)",NULL},
  {(char*)"parameter2",type_get_parameter2,NULL,(char*)"parameter2 type (if any)",NULL},
  {(char*)"if1",type_get_if1,NULL,(char*)"if1 code",NULL},
  {(char*)"label",type_get_label,NULL,(char*)"type label",NULL},
  {NULL}
};

/*
                    #                       
   #   #            #           ##          
   ## ##            #            #          
   # # #   ##     ###   #  #     #     ##   
   # # #  #  #   #  #   #  #     #    # ##  
   #   #  #  #   #  #   #  #     #    ##    
   #   #   ##     ###    ###    ###    ###  
*/


static PyObject* rawtype(PyObject* module,
                         unsigned long code,
                         unsigned long aux,
                         PyObject* strong1,
                         PyObject* strong2,
                         PyObject* name,
                         const char* cname) {

  PyObject* typelist /*borrowed*/ = reinterpret_cast<IF1_ModuleObject*>(module)->types;

  // We may already have this type (possibly with a different name). In IF1 those are equivalent,
  // so return that
  for(ssize_t i=0;i<PyList_GET_SIZE(typelist);++i) {
    PyObject* p /*borrowed*/ = PyList_GET_ITEM(typelist,i);
    IF1_TypeObject* t = reinterpret_cast<IF1_TypeObject*>(p);
    if (t->code != code) continue;
    if (t->aux != aux) continue;
    PyObject* strong = (t->weak1)?PyWeakref_GET_OBJECT(t->weak1):NULL;
    if (strong1 != strong) continue;
    strong = (t->weak2)?PyWeakref_GET_OBJECT(t->weak2):NULL;
    if (strong2 != strong) continue;
    Py_INCREF(p);
    return p;
  }

  PyObject* result /*owned*/ = PyType_GenericNew(&IF1_TypeType,NULL,NULL);
  if (!result) return NULL;
  IF1_TypeObject* T = reinterpret_cast<IF1_TypeObject*>(result);
  T->weakmodule = NULL;
  T->weak1 = NULL;
  T->weak2 = NULL;
  T->name = NULL;

  T->weakmodule /*borrowed*/ = PyWeakref_NewRef(module,NULL);
  if (!T->weakmodule) { Py_DECREF(T); return NULL;}
  T->code = code;
  T->aux = aux;
  T->weak1 = strong1?PyWeakref_NewRef(strong1,NULL):NULL;
  if (strong1 && !T->weak1) { Py_DECREF(T); return NULL;}
  T->weak2 = strong2?PyWeakref_NewRef(strong2,NULL):NULL;
  if (strong2 && !T->weak2) { Py_DECREF(T); return NULL;}
  if (name) {
    Py_INCREF(T->name = name);
  } else if (cname) {
    T->name = PyString_FromString(cname);
    if (!T->name) { Py_DECREF(T); return NULL;}
  } else {
    T->name = NULL;
  }

  if (PyList_Append(typelist,result) != 0) { Py_DECREF(result); return NULL;}

  return result;
}

static PyObject* rawtype(IF1_ModuleObject* module,
                         unsigned long code,
                         unsigned long aux,
                         PyObject* sub1,
                         PyObject* sub2,
                         PyObject* name) {
  return rawtype(reinterpret_cast<PyObject*>(module),
                 code,
                 aux,
                 sub1,
                 sub2,
                 name,
                 NULL);
}

int module_init(PyObject* pySelf, PyObject* args, PyObject* kwargs) {
  IF1_ModuleObject* self = reinterpret_cast<IF1_ModuleObject*>(pySelf);
  self->types = NULL;
  self->pragmas = NULL;

  self->dict = PyDict_New();
  if (!self->dict) return -1;

  self->types = PyList_New(0);
  if (!self->types) return -1;

  self->pragmas = PyDict_New();
  if (!self->pragmas) return -1;

  self->functions = PyList_New(0);
  if (!self->functions) return -1;

  PyObject* boolean /*owned*/ = rawtype(pySelf,IF_Basic,IF_Boolean,NULL,NULL,NULL,"boolean");
  if (!boolean) return -1;
  Py_DECREF(boolean);

  PyObject* character /*owned*/ = rawtype(pySelf,IF_Basic,IF_Character,NULL,NULL,NULL,"character");
  if (!character) return -1;
  Py_DECREF(character);

  PyObject* doublereal /*owned*/ = rawtype(pySelf,IF_Basic,IF_DoubleReal,NULL,NULL,NULL,"doublereal");
  if (!doublereal) return -1;
  Py_DECREF(doublereal);

  PyObject* integer /*owned*/ = rawtype(pySelf,IF_Basic,IF_Integer,NULL,NULL,NULL,"integer");
  if (!integer) return -1;
  Py_DECREF(integer);

  PyObject* null /*owned*/ = rawtype(pySelf,IF_Basic,IF_Null,NULL,NULL,NULL,"null");
  if (!null) return -1;
  Py_DECREF(null);

  PyObject* real /*owned*/ = rawtype(pySelf,IF_Basic,IF_Real,NULL,NULL,NULL,"real");
  if (!real) return -1;
  Py_DECREF(real);

  PyObject* wildbasic /*owned*/ = rawtype(pySelf,IF_Basic,IF_WildBasic,NULL,NULL,NULL,"wildbasic");
  if (!wildbasic) return -1;
  Py_DECREF(wildbasic);

  PyObject* wild /*owned*/ = rawtype(pySelf,IF_Wild,0,NULL,NULL,NULL,"wild");
  if (!wild) return -1;
  Py_DECREF(wild);

  PyObject* string /*owned*/ = rawtype(pySelf,IF_Array,0,character,NULL,NULL,"string");
  if (!string) return -1;
  Py_DECREF(string);

  for(int i=0;i<PyList_GET_SIZE(self->types);++i) {
    PyObject* p /*borrowed*/ = PyList_GET_ITEM(self->types,i);
    IF1_TypeObject* t /*borrowed*/ = reinterpret_cast<IF1_TypeObject*>(p);
    if (t->name) PyDict_SetItem(self->dict,t->name,p);
  }
  return 0;
}

void module_dealloc(PyObject* pySelf) {
  IF1_ModuleObject* self = reinterpret_cast<IF1_ModuleObject*>(pySelf);
  if (self->weak) PyObject_ClearWeakRefs(pySelf);

  Py_XDECREF(self->dict);
  Py_XDECREF(self->types);
  Py_XDECREF(self->pragmas);
  Py_XDECREF(self->functions);
  Py_TYPE(pySelf)->tp_free(pySelf);
}

PyObject* module_str(PyObject* pySelf) {
  return PyString_FromString("<module>");
}

PyObject* module_get_if1(PyObject* pySelf,void*) {
  IF1_ModuleObject* self = reinterpret_cast<IF1_ModuleObject*>(pySelf);
  PyObject* result = PyString_FromString("");
  if (!result) return NULL;

  // Make sure we have a type for all the functions
  for(ssize_t i=0; i<PyList_GET_SIZE(self->functions); ++i) {
    PyObject* T = PyObject_GetAttrString(PyList_GET_ITEM(self->functions,i),"type");
    Py_XDECREF(T);
  }
  PyErr_Clear();

  // Add if1 for all the types
  for(ssize_t i=0;i<PyList_GET_SIZE(self->types);++i) {
    PyObject* if1 /*owned*/ = PyObject_GetAttrString(PyList_GET_ITEM(self->types,i),"if1");
    PyString_ConcatAndDel(&result,if1);
    if (!result) return nullptr;
    PyString_Concat(&result,NEWLINE);
    if (!result) return nullptr;
  }

  // Add all the pragmas
  PyObject* key;
  PyObject* remark;
  Py_ssize_t pos = 0;
  while (PyDict_Next(self->pragmas, &pos, &key, &remark)) {
    if (!PyString_Check(key)) {
      Py_DECREF(result);
      return PyErr_Format(PyExc_TypeError,"pragma key is not a string");
    }
    if (!PyString_Check(remark)) {
      Py_DECREF(result);
      return PyErr_Format(PyExc_TypeError,"pragma value is not a string");
    }
    PyObject* if1 /*owned*/ = PyString_FromFormat("C$  %c %s",PyString_AS_STRING(key)[0],PyString_AS_STRING(remark));
    if (!if1) { Py_DECREF(result); return NULL; }
    PyString_ConcatAndDel(&result,if1);
    if (!result) return nullptr;
    PyString_Concat(&result,NEWLINE);
    if (!result) nullptr;
  }
  if (PyErr_Occurred()) { Py_DECREF(result); return nullptr; }

  // And all the functions
  for(ssize_t i=0; i<PyList_GET_SIZE(self->functions); ++i) {
    PyObject* if1 = PyObject_GetAttrString(PyList_GET_ITEM(self->functions,i),"if1");
    if (!if1) { Py_DECREF(result); return nullptr; }
    PyString_ConcatAndDel(&result,if1);
    if (!result) return nullptr;
  }

  return result;
}

PyObject* module_addtype_array(IF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  long code;
  PyObject* base = NULL;
  if (!PyArg_ParseTuple(args,"lO!",&code,&IF1_TypeType,&base)) return NULL;
  return rawtype(self,code,0,base,NULL,name);
}

PyObject* module_addtype_basic(IF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  long code;
  long basic;
  if (!PyArg_ParseTuple(args,"ll",&code,&basic)) return NULL;
  if (basic < 0 || basic > IF_WildBasic) return PyErr_Format(PyExc_ValueError,"invalid basic code %ld",basic);
  return rawtype(self,code,basic,NULL,NULL,name);
}

template<int IFXXX>
PyObject* module_addtype_chain(IF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist, ssize_t first) {
  PyObject* last /*owned*/ = NULL;
  for(ssize_t i=PyTuple_GET_SIZE(args)-1; i>=first; --i) {
    PyObject* Tname = namelist?PyList_GetItem(namelist,i-first):NULL;
    PyErr_Clear();
    PyObject* T /*borrowed*/ = PyTuple_GET_ITEM(args,i);
    if (!PyObject_IsInstance(T,reinterpret_cast<PyObject*>(&IF1_TypeType))) {
      Py_XDECREF(last);
      return PyErr_Format(PyExc_TypeError,"Argument %ld is not a type",(long)i+1);
    }
    PyObject* next = rawtype(self,IFXXX,0,T,last,Tname);
    Py_XDECREF(last);
    last = next;
  }

  // We return the last type or None for the empty argument set
  if (!last) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  return last;
}

template<int STRUCTURE,int CHAIN>
PyObject* module_addtype_structure(IF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  PyObject* chain = module_addtype_chain<CHAIN>(self,args,NULL,namelist,1);
  if (!chain) return NULL;

  return rawtype(self,STRUCTURE,0,chain,NULL,name);
}


PyObject* module_addtype_function(IF1_ModuleObject* self,PyObject* args, PyObject* name, PyObject* namelist) {
  // I want this to be of the form: (code,tupletype,tupletype), but I might get
  // ... (code,(T,T,T),(T,T)) or (code,None,(T,T)) or even (code,(T,T,T),T)
  // at least it all looks like (code, ins, outs)
  long code;
  PyObject* ins;
  PyObject* outs;
  if (!PyArg_ParseTuple(args,"lOO",&code,&ins,&outs)) return NULL;

  // Inputs may be a tupletype, a tuple of types, a normal type, or None
  if (PyObject_IsInstance(ins,(PyObject*)&IF1_TypeType)) {
    IF1_TypeObject* tins = reinterpret_cast<IF1_TypeObject*>(ins);
    if (tins->code == IF_Tuple) {
      Py_INCREF(ins);
    } else {
      // Build a tuple to pass this in
      PyObject* tup = PyTuple_New(1);
      PyTuple_SET_ITEM(tup,0,ins);
      Py_INCREF(ins);
      ins = module_addtype_chain<IF_Tuple>(self,tup,NULL,namelist,0);
      if (!ins) return NULL;
      Py_DECREF(tup);
    }
  }
  else if (PyTuple_Check(ins)) {
    // With a tuple, just construct the new IF_Tuple from those
    if (PyTuple_GET_SIZE(ins) == 0) {
      ins = NULL;
    } else {
      ins = module_addtype_chain<IF_Tuple>(self,ins,NULL,namelist,0);
      if (!ins) return NULL;
    }
  }
  else if (ins == Py_None) {
    ins = NULL;
  }
  else {
    return PyErr_Format(PyExc_TypeError,"function input cannot be %s",ins->ob_type->tp_name);
  }

  // Outputs may be a tupletype, a tuple of types, a normal type, but not None
  if (PyObject_IsInstance(outs,(PyObject*)&IF1_TypeType)) {
    IF1_TypeObject* touts = reinterpret_cast<IF1_TypeObject*>(outs);
    if (touts->code == IF_Tuple) {
      Py_INCREF(outs);
    } else {
      // Build a tuple to pass this in
      PyObject* tup = PyTuple_New(1);
      PyTuple_SET_ITEM(tup,0,outs);
      Py_INCREF(outs);
      outs = module_addtype_chain<IF_Tuple>(self,tup,NULL,namelist,0);
      if (!outs) return NULL;
      Py_DECREF(tup);
    }
  }
  else if (PyTuple_Check(outs)) {
    // With a tuple, just construct the new IF_Tuple from those
    if (PyTuple_GET_SIZE(outs) == 0) return PyErr_Format(PyExc_ValueError,"Cannot have an empty output type");
    outs = module_addtype_chain<IF_Tuple>(self,outs,NULL,namelist,0);
    if (!outs) return NULL;
  }
  else {
    Py_XDECREF(ins);
    return PyErr_Format(PyExc_TypeError,"function output cannot be %s",outs->ob_type->tp_name);
  }

  PyObject* result = rawtype(self,IF_Function,0,ins,outs,name);
  Py_XDECREF(ins);
  Py_XDECREF(outs);
  return result;
}

PyObject* module_addtype(PyObject* pySelf,PyObject* args,PyObject* kwargs) {
  IF1_ModuleObject* self = reinterpret_cast<IF1_ModuleObject*>(pySelf);
  PyObject* name /*borrowed*/ = NULL;
  PyObject* namelist /*owned*/ = NULL;
  if (kwargs) {
    name = PyDict_GetItemString(kwargs,"name");
    PyDict_DelItemString(kwargs,"name");

    PyObject* names = PyDict_GetItemString(kwargs,"names");
    PyDict_DelItemString(kwargs,"names");
    if (names) {
      PyObject* namesiter /*owned*/ = PyObject_GetIter(names);
      if (!namesiter) return NULL;
      PyObject* item;
      namelist = PyList_New(0);
      if (!namelist) { Py_DECREF(namesiter); return NULL; }
      while((PyErr_Clear(), item=PyIter_Next(namesiter))) {
        int a = PyList_Append(namelist,item);
        Py_DECREF(item);
        if(a<0) {
          Py_DECREF(namesiter);
          Py_DECREF(namelist);
          return NULL;
        }
        Py_DECREF(item);
      }
      Py_DECREF(namesiter);
    }
  }

  if (PyTuple_GET_SIZE(args) < 1) {
    return PyErr_Format(PyExc_TypeError,"the code is required");
  }
  long code = PyInt_AsLong(PyTuple_GET_ITEM(args,0));
  if ( code < 0 && PyErr_Occurred() ) return NULL;

  PyObject* result = NULL;
  switch(code) {
  case IF_Array:
    result = module_addtype_array(self,args,name, namelist);
    break;
  case IF_Basic: 
    result = module_addtype_basic(self,args,name, namelist);
    break;
  case IF_Field: 
    result = module_addtype_chain<IF_Field>(self,args,name, namelist,1);
    break;
  case IF_Function: 
    result = module_addtype_function(self,args,name, namelist);
    break;
  case IF_Multiple: 
    result = module_addtype_array(self,args,name, namelist);
    break;
  case IF_Record: 
    result = module_addtype_structure<IF_Record,IF_Field>(self,args,name, namelist);
    break;
  case IF_Stream: 
    result = module_addtype_array(self,args,name, namelist);
    break;
  case IF_Tag: 
    result = module_addtype_chain<IF_Tag>(self,args,name, namelist,1);
    break;
  case IF_Tuple: 
    result = module_addtype_chain<IF_Tuple>(self,args,name, namelist,1);
    break;
  case IF_Union: 
    result = module_addtype_structure<IF_Union,IF_Tag>(self,args,name, namelist);
    break;
  default:
    result = PyErr_Format(PyExc_ValueError,"Unknown type code %ld",code);
  }

  Py_XDECREF(namelist);
  return result;
}

PyObject* module_addfunction(PyObject* pySelf,PyObject* args) {
  IF1_ModuleObject* self = reinterpret_cast<IF1_ModuleObject*>(pySelf);

  PyObject* name;
  long opcode = IFXGraph;
  PyObject* type = NULL;
  if (!PyArg_ParseTuple(args,"O!|lO!",&PyString_Type,&name,&opcode,&IF1_TypeType,&type)) return NULL;

  PyObject* N /*owned*/ = PyType_GenericNew(&IF1_GraphType,NULL,NULL);
  if (!N) return NULL;
  graph_init(N, pySelf, opcode, name, type);
  PyList_Append(self->functions,N);

  return N;
}

static PyMethodDef module_methods[] = {
  {(char*)"addtype",(PyCFunction)module_addtype,METH_VARARGS|METH_KEYWORDS,(char*)"adds a new type"},
  {(char*)"addfunction",module_addfunction,METH_VARARGS,(char*)"create a new function graph"},
  {NULL}  /* Sentinel */
};

static PyGetSetDef module_getset[] = {
  {(char*)"if1",module_get_if1,NULL,(char*)"IF1 code for entire module",NULL},
  {NULL}
};

/*
   ###    ####    #    
    #     #      ##    
    #     ###     #    
    #     #       #    
    #     #       #    
   ###    #      ###  
 */

static PyMethodDef IF1_methods[] = {
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initif1(void) 
{
  FUNCTIONTYPE = PyInt_FromLong(IF_Function);
  if (!FUNCTIONTYPE) return;

  ADDTYPE = PyString_InternFromString("addtype");
  if (!ADDTYPE) return;

  ARROW = PyString_InternFromString("->");
  if (!ARROW) return;

  COLON = PyString_InternFromString(":");
  if (!COLON) return;

  COMMA = PyString_InternFromString(",");
  if (!COMMA) return;

  LPAREN = PyString_InternFromString("(");
  if (!LPAREN) return;

  RPAREN = PyString_InternFromString(")");
  if (!RPAREN) return;

  SPACE = PyString_InternFromString(" ");
  if (!SPACE) return;

  DQUOTE = PyString_InternFromString("\"");
  if (!DQUOTE) return;

  NEWLINE = PyString_InternFromString("\n");
  if (!NEWLINE) return;

  // ----------------------------------------------------------------------
  // Setup
  // ----------------------------------------------------------------------
  PyObject* m = Py_InitModule3("sap.if1", IF1_methods,
                     "Example module that creates an extension type.");
  if (!m) return;

  // ----------------------------------------------------------------------
  // InPort
  // ----------------------------------------------------------------------
  IF1_InPortType.tp_name = "if1.InPort";
  IF1_InPortType.tp_basicsize = sizeof(IF1_InPortObject);
  IF1_InPortType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
  IF1_InPortType.tp_doc = inport_doc;
  IF1_InPortType.tp_dealloc = inport_dealloc;
  IF1_InPortType.tp_str = inport_str;
  IF1_InPortType.tp_repr = inport_str;
  IF1_InPortType.tp_as_number = &inport_number;
  inport_number.nb_lshift = inport_lshift;
  if (PyType_Ready(&IF1_InPortType) < 0) return;

  // ----------------------------------------------------------------------
  // Node
  // ----------------------------------------------------------------------
  IF1_NodeType.tp_name = "if1.Node";
  IF1_NodeType.tp_basicsize = sizeof(IF1_NodeObject);
  IF1_NodeType.tp_flags = Py_TPFLAGS_DEFAULT;
  IF1_NodeType.tp_doc = node_doc;
  IF1_NodeType.tp_dealloc = node_dealloc;
  IF1_NodeType.tp_members = node_members;
  IF1_NodeType.tp_getset = node_getset;
  IF1_NodeType.tp_call = node_inport;
  IF1_NodeType.tp_str = node_str;
  IF1_NodeType.tp_repr = node_str;
  IF1_NodeType.tp_weaklistoffset = offsetof(IF1_NodeObject,weak);
  IF1_NodeType.tp_as_number = &node_number;
  node_number.nb_int = node_int;
  node_number.nb_positive = node_positive;
  if (PyType_Ready(&IF1_NodeType) < 0) return;

  // ----------------------------------------------------------------------
  // Graph
  // ----------------------------------------------------------------------
  IF1_GraphType.tp_name = "if1.Graph";
  IF1_GraphType.tp_basicsize = sizeof(IF1_GraphObject);
  IF1_GraphType.tp_base = &IF1_NodeType;
  IF1_GraphType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  IF1_GraphType.tp_doc = graph_doc;
  IF1_GraphType.tp_dealloc = graph_dealloc;
  IF1_GraphType.tp_members = graph_members;
  IF1_GraphType.tp_getset = graph_getset;
  IF1_GraphType.tp_as_number = &graph_number;
  graph_number.nb_int = graph_int;
  graph_number.nb_positive = graph_positive;
  if (PyType_Ready(&IF1_GraphType) < 0) return;

  // ----------------------------------------------------------------------
  // Type
  // ----------------------------------------------------------------------
  IF1_TypeType.tp_name = "if1.Type";
  IF1_TypeType.tp_basicsize = sizeof(IF1_TypeObject);
  IF1_TypeType.tp_flags = Py_TPFLAGS_DEFAULT;
  IF1_TypeType.tp_doc = type_doc;
  IF1_TypeType.tp_weaklistoffset = offsetof(IF1_TypeObject,weak);
  IF1_TypeType.tp_dealloc = type_dealloc;
  IF1_TypeType.tp_str = type_str;
  IF1_TypeType.tp_repr = type_str;
  IF1_TypeType.tp_members = type_members;
  IF1_TypeType.tp_getset = type_getset;
  IF1_TypeType.tp_as_number = &type_number;
  type_number.nb_int = type_int;
  if (PyType_Ready(&IF1_TypeType) < 0) return;
  Py_INCREF(&IF1_TypeType);
  PyModule_AddObject(m, "Type", (PyObject *)&IF1_TypeType);

  // ----------------------------------------------------------------------
  // Module
  // ----------------------------------------------------------------------
  IF1_ModuleType.tp_name = "if1.Module";
  IF1_ModuleType.tp_basicsize = sizeof(IF1_ModuleObject);
  IF1_ModuleType.tp_flags = Py_TPFLAGS_DEFAULT;
  IF1_ModuleType.tp_doc = module_doc;
  IF1_ModuleType.tp_weaklistoffset = offsetof(IF1_ModuleObject,weak);
  IF1_ModuleType.tp_new = PyType_GenericNew;
  IF1_ModuleType.tp_init = module_init;
  IF1_ModuleType.tp_dealloc = module_dealloc;
  IF1_ModuleType.tp_methods = module_methods;
  IF1_ModuleType.tp_str = module_str;
  IF1_ModuleType.tp_repr = module_str;
  IF1_ModuleType.tp_members = module_members;
  IF1_ModuleType.tp_getset = module_getset;
  IF1_ModuleType.tp_getattro = PyObject_GenericGetAttr;
  IF1_ModuleType.tp_dictoffset = offsetof(IF1_ModuleObject,dict);
  if (PyType_Ready(&IF1_ModuleType) < 0) return;
  Py_INCREF(&IF1_ModuleType);
  PyModule_AddObject(m, "Module", (PyObject *)&IF1_ModuleType);

  // ----------------------------------------------------------------------
  // Export macros
  // ----------------------------------------------------------------------
  PyModule_AddIntMacro(m,IF_Array);
  PyModule_AddIntMacro(m,IF_Basic);
  PyModule_AddIntMacro(m,IF_Field);
  PyModule_AddIntMacro(m,IF_Function);
  PyModule_AddIntMacro(m,IF_Multiple);
  PyModule_AddIntMacro(m,IF_Record);
  PyModule_AddIntMacro(m,IF_Stream);
  PyModule_AddIntMacro(m,IF_Tag);
  PyModule_AddIntMacro(m,IF_Tuple);
  PyModule_AddIntMacro(m,IF_Union);
  PyModule_AddIntMacro(m,IF_Wild);
  PyModule_AddIntMacro(m,IF_Boolean);
  PyModule_AddIntMacro(m,IF_Character);
  PyModule_AddIntMacro(m,IF_DoubleReal);
  PyModule_AddIntMacro(m,IF_Integer);
  PyModule_AddIntMacro(m,IF_Null);
  PyModule_AddIntMacro(m,IF_Real);
  PyModule_AddIntMacro(m,IF_WildBasic);
  PyModule_AddIntMacro(m,IFAAddH);
  PyModule_AddIntMacro(m,IFAAddL);
  PyModule_AddIntMacro(m,IFAAdjust);
  PyModule_AddIntMacro(m,IFABuild);
  PyModule_AddIntMacro(m,IFACatenate);
  PyModule_AddIntMacro(m,IFAElement);
  PyModule_AddIntMacro(m,IFAFill);
  PyModule_AddIntMacro(m,IFAGather);
  PyModule_AddIntMacro(m,IFAIsEmpty);
  PyModule_AddIntMacro(m,IFALimH);
  PyModule_AddIntMacro(m,IFALimL);
  PyModule_AddIntMacro(m,IFARemH);
  PyModule_AddIntMacro(m,IFARemL);
  PyModule_AddIntMacro(m,IFAReplace);
  PyModule_AddIntMacro(m,IFAScatter);
  PyModule_AddIntMacro(m,IFASetL);
  PyModule_AddIntMacro(m,IFASize);
  PyModule_AddIntMacro(m,IFAbs);
  PyModule_AddIntMacro(m,IFBindArguments);
  PyModule_AddIntMacro(m,IFBool);
  PyModule_AddIntMacro(m,IFCall);
  PyModule_AddIntMacro(m,IFChar);
  PyModule_AddIntMacro(m,IFDiv);
  PyModule_AddIntMacro(m,IFDouble);
  PyModule_AddIntMacro(m,IFEqual);
  PyModule_AddIntMacro(m,IFExp);
  PyModule_AddIntMacro(m,IFFirstValue);
  PyModule_AddIntMacro(m,IFFinalValue);
  PyModule_AddIntMacro(m,IFFloor);
  PyModule_AddIntMacro(m,IFInt);
  PyModule_AddIntMacro(m,IFIsError);
  PyModule_AddIntMacro(m,IFLess);
  PyModule_AddIntMacro(m,IFLessEqual);
  PyModule_AddIntMacro(m,IFMax);
  PyModule_AddIntMacro(m,IFMin);
  PyModule_AddIntMacro(m,IFMinus);
  PyModule_AddIntMacro(m,IFMod);
  PyModule_AddIntMacro(m,IFNeg);
  PyModule_AddIntMacro(m,IFNoOp);
  PyModule_AddIntMacro(m,IFNot);
  PyModule_AddIntMacro(m,IFNotEqual);
  PyModule_AddIntMacro(m,IFPlus);
  PyModule_AddIntMacro(m,IFRangeGenerate);
  PyModule_AddIntMacro(m,IFRBuild);
  PyModule_AddIntMacro(m,IFRElements);
  PyModule_AddIntMacro(m,IFRReplace);
  PyModule_AddIntMacro(m,IFRedLeft);
  PyModule_AddIntMacro(m,IFRedRight);
  PyModule_AddIntMacro(m,IFRedTree);
  PyModule_AddIntMacro(m,IFReduce);
  PyModule_AddIntMacro(m,IFRestValues);
  PyModule_AddIntMacro(m,IFSingle);
  PyModule_AddIntMacro(m,IFTimes);
  PyModule_AddIntMacro(m,IFTrunc);
  PyModule_AddIntMacro(m,IFPrefixSize);
  PyModule_AddIntMacro(m,IFError);
  PyModule_AddIntMacro(m,IFReplaceMulti);
  PyModule_AddIntMacro(m,IFConvert);
  PyModule_AddIntMacro(m,IFCallForeign);
  PyModule_AddIntMacro(m,IFAElementN);
  PyModule_AddIntMacro(m,IFAElementP);
  PyModule_AddIntMacro(m,IFAElementM);
  PyModule_AddIntMacro(m,IFSGraph);
  PyModule_AddIntMacro(m,IFLGraph);
  PyModule_AddIntMacro(m,IFIGraph);
  PyModule_AddIntMacro(m,IFXGraph);
  PyModule_AddIntMacro(m,IFLPGraph);
  PyModule_AddIntMacro(m,IFRLGraph);


  // We want opcode->name and name->opcode maps
  OPNAMES = PyDict_New();
  PyModule_AddObject(m,"OPNAMES",OPNAMES);
  OPCODES = PyDict_New();
  PyModule_AddObject(m,"OPCODES",OPCODES);
  PyObject* key;
  PyObject* value;
  Py_ssize_t pos = 0;
  PyObject* dict /*borrowed*/ = PyModule_GetDict(m);
  if (!dict) return;
  while(PyDict_Next(dict,&pos,&key,&value)) {
    if (PyString_Check(key) &&
        PyString_AS_STRING(key)[0] == 'I' && PyString_AS_STRING(key)[1] == 'F' &&
        PyString_AS_STRING(key)[2] != '_'
        ) {
      PyDict_SetItem(OPNAMES,key,value);
      PyDict_SetItem(OPCODES,value,key);
    }
  }

  

}
//(insert "\n" (shell-command-to-string "awk '/ IF/{printf \"PyModule_AddIntMacro(m,%s);\\n\", $1}' IFX.h"))
