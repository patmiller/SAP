#ifndef SAP_NODEBASE
#define SAP_NODEBASE

#include "IFX.h"

class module;
class inport;
class outport;
class graph;

class nodebase {
public:
  long opcode;
  std::weak_ptr<nodebase> weakparent;
  PyOwned children;
  PyOwned pragmas;
  std::map<long,std::shared_ptr<inport>> inputs;
  std::map<long,std::shared_ptr<outport>> outputs;
  
  PyObject* string();

  virtual operator long();
  virtual PyObject* lookup();
  virtual std::shared_ptr<graph> my_graph();
  virtual std::shared_ptr<module> my_module();
  PyObject* edge_if1(long);

  nodebase(long opcode=-1,std::shared_ptr<nodebase> parent=nullptr);
  virtual ~nodebase() {}
};

#endif
