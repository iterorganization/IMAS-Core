/*-*-c++-*-*/

/**
   \file ual_context.h
   Contains definition of Context classes and all its subclasses.
*/

#ifndef UAL_CONTEXT_H
#define UAL_CONTEXT_H 1

#include "ual_const.h"



/* c++ only part */
#if defined(__cplusplus)


#include <cstdlib>
#include <iostream>

#include <atomic>

#include "ual_exception.h"


#define CTX_TYPE 100
#define CTX_PULSE_TYPE 101
#define CTX_OPERATION_TYPE 102
#define CTX_ARRAYSTRUCT_TYPE 103


/**
   Generic Context class.
   The simple generic Context is only associated to a given Back-end.
*/
class Context 
{
public:
  /**
     Context destructor.
   */
  virtual ~Context() {}

  friend std::ostream& operator<< (std::ostream& o, Context const& ctx)
  {
    return o << ctx.print();
  }

  /**
     Prints content of the context.
  */
  virtual std::string print() const 
  {
    std::string s = "backend_id \t\t = " + std::to_string(this->backend_id) + " (" + 
      this->getBackendName() + "), uid = " + std::to_string(this->uid) + "\n";
    return s;
  }

  /**
     Returns the ID of associated backend.
     @result backend_id
  */
  int getBackendID() { return backend_id; }

  /**
     Returns the name of associated backend.
     @result name of the backend
  */
  std::string getBackendName() const 
  { 
    return ualconst::backend_id_str.at(backend_id-BACKEND_ID_0); 
  }

  /**
     Returns object unique id.
     @result uid
  */
  unsigned long int getUid() const 
  {
    return uid;
  }

  /**
     Returns the type of context.
     @result CTX_TYPE
  */
  virtual int getType() const 
  {
    return CTX_TYPE;
  }

  /**
     Context copy constructor.
     Explicit definition to handle uid update.
  */
  Context(const Context& ctx) 
  {
    backend_id = ctx.backend_id; 
    ++uid;
  }
  
protected:
  /**
     Context constructor.
     @param id backend identifier (see ual_const.h):
     - NO_BACKEND
     - MDSPLUS_BACKEND
     - HDF5_BACKEND
     @todo need to check if passed id is valid
     @todo how to check/configure available backends on a given system?
  */
  Context(int id)
  { 
    try 
      {
	ualconst::backend_id_str.at(id-BACKEND_ID_0);
      }
    catch (const std::out_of_range& e) 
      {
	throw UALContextException("Wrong backend identifier "+std::to_string(id),LOG);
      }
    backend_id = id; 
    ++uid;
  }

  int backend_id;                       /**< a backend identifier */
  static std::atomic<unsigned long int> uid;
};



/**
   Context class for a given pulse file.
   The PulseContext is a Context associated to a given pulse file.
*/
class PulseContext : public Context 
{
public:
  /**
     Complete constructor.
     It requires all informations as arguments, empty strings will be replaced 
     by environment values.
     @param id backend identifier 
     @param s shot number
     @param r run number
     @param u user name
     @param t tokamak name
     @param v data version
  */
  PulseContext(int id, int s, int r, std::string u, std::string t, std::string v)
    : Context(id), shot(s), run(r)
  {
    char *usr = std::getenv("USER"); 
    if (u=="")
      {
	if (usr!=NULL)
	  user = usr;
	else
	  throw UALContextException("Undefined env variable USER",LOG);
      }
    else 
      user = u;

    char *tok = std::getenv("TOKAMAKNAME"); 
    if (t=="") 
      {
	if (tok!=NULL)
	  tokamak = tok;
	else
	  throw UALContextException("Undefined env variable TOKAMAKNAME",LOG);
      }
    else
      tokamak = t;

    char *ver = std::getenv("DATAVERSION"); 
    if (v=="") 
      {
	if (ver!=NULL) 
	  {
	    version = ver;
	  }
	else
	  throw UALContextException("Undefined env variable DATAVERSION",LOG);
      }
    else
      {
	size_t pos = v.find('.');
	if (pos == std::string::npos)
	  version = v;
	else
	  version = v.substr(0,pos);
      }

    /* no version in env    
    if (version!=ver)
      {
	std::cerr << "WARNING: selected data version (" << version 
		  << ") differs from env DATAVERSION (" << ver
		  << ")" << WHERE << "\n";
    */
	/* if environment version is older than data target version   */
	/* we are probably not able to understand the data: exception */
    /*
	if (ver < version) 
	  throw UALContextException("ERROR: try to read "+ version +
				    " data with "+ ver +
				    " UAL: operation not supported!",LOG);
      }
    */

  }

  /**
     Pulse context destructor.
  */
  virtual ~PulseContext() {}

  friend std::ostream& operator<< (std::ostream& o, PulseContext const& ctx)
  {
    return o << ctx.print();
  }

  /**
     Prints content of the pulse context.
  */
  virtual std::string print() const 
  {
    std::string s = ((Context)*this).print() +
      "shot \t\t\t = " + std::to_string(this->shot) + "\n" +
      "run \t\t\t = " + std::to_string(this->run) + "\n" +
      "user \t\t\t = " + this->user + "\n" +
      "tokamak \t\t = " + this->tokamak + "\n" +
      "version \t\t = " + this->version + "\n";
    return s;
  }

  /**
     Returns the type of context.
     @result CTX_PULSE_TYPE
  */
  virtual int getType() const 
  {
    return CTX_PULSE_TYPE;
  }

  /**
     Returns the shot number.
     @result shot 
  */
  int getShot() { return shot; }

  /**
     Returns the run number.
     @result run
  */
  int getRun() { return run; }

  /**
     Returns the user name.
     @result user 
  */
  std::string getUser() { return user; }

  /**
     Returns the tokamak name.
     @result tokamak 
  */
  std::string getTokamak() { return tokamak; }

  /**
     Returns the version.
     @result version 
  */
  std::string getVersion() { return version; }


 protected:
  int shot;                             /**< shot number */
  int run;                              /**< run number */
  std::string user;                     /**< user name */
  std::string tokamak;                  /**< tokamak name */
  std::string version;                  /**< data version */
};



/**
   Context class for an operation on a DATAOBJECT.
   The OperationContext is a PulseContext associated to a given DATAOBJECT for a given I/O operation.
*/
class OperationContext : public PulseContext 
{
public:
  /**
     Operation context constructor.
     Requires informations for all global put or get operations on a DATAOBJECT.
     @param ctx pulse context
     @param dataobject name of the DATAOBJECT
     @param access access type of the operation 
     - READ_OP: read operation
     - WRITE_OP: write operation
     - REPLACE_OP: replace operation [_for the moment only in sliced mode for 
     "replace last slice"_]
  */
  OperationContext(PulseContext ctx, std::string dataobject, int access)
    : PulseContext(ctx), dataobjectname(dataobject)
  {
    rangemode = ualconst::global_op;
    time = ualconst::undefined_time;
    interpmode = ualconst::undefined_interp;

    try {
      ualconst::op_access_str.at(access-OP_ACCESS_0);
    } 
    catch (std::out_of_range) {
      throw UALContextException("Wrong access mode "+std::to_string(access),LOG);
    }
    accessmode = access;
  }

  /**
     Operation context constructor.
     Requires informations for all possible put or get operations on a DATAOBJECT.
     @param ctx pulse context
     @param dataobject name of the DATAOBJECT
     @param access access type of the operation 
     - READ_OP: read operation
     - WRITE_OP: write operation
     - REPLACE_OP: replace operation [_for the moment only in sliced mode for 
     "replace last slice"_]
     @param range range of the operation [_optional_]
     - GLOBAL_OP: operation on all slices 
     - SLICE_OP: operation on a single slice
     @param t time [_optional_]
     @param interp interpolation mode [_optional_]
     - CLOSEST_INTERP: consider slice at closest time
     - PREVIOUS_INTERP: consider slice at previous step
     - LINEAR_INTERP: interpolating linearly values at previous and next slices
     - UNDEFINED_INTERP: if not relevant [_e.g for write operations_]
  */
  OperationContext(PulseContext ctx, std::string dataobject, int access, 
		   int range, double t, int interp)
    : PulseContext(ctx), dataobjectname(dataobject), time(t)
  {
    try {
      ualconst::op_range_str.at(range-OP_RANGE_0);
    } 
    catch (std::out_of_range) {
      throw UALContextException("Wrong range mode "+std::to_string(range),LOG);
    }
    rangemode = range;

    try {
      ualconst::op_access_str.at(access-OP_ACCESS_0);
    } 
    catch (std::out_of_range) {
      throw UALContextException("Wrong access mode "+std::to_string(access),LOG);
    }
    accessmode = access;

    try {
      ualconst::op_interp_str.at(interp-OP_INTERP_0);
    } 
    catch (std::out_of_range) {
      throw UALContextException("Wrong interp mode "+std::to_string(interp),LOG);
    }
    interpmode = interp;

    // test consistency [missing or wrong expected args, not all possible missmatches!]
    if (rangemode==ualconst::slice_op)
      {
	/* GM: possible negative times
	if (time<0)
	  throw UALContextException("Wrong time value "+std::to_string(time)
				    +" for SLICE_OP",LOG);
	*/
	if (accessmode==ualconst::read_op && interpmode==ualconst::undefined_interp)
	  throw UALContextException("Missing interpmode",LOG);
      }
  }

  /**
     Operation context destructor.
  */
  virtual ~OperationContext() {}

  friend std::ostream& operator<< (std::ostream& o, OperationContext const& ctx)
  {
    return o << ctx.print();
  }

  /**
     Prints content of the operation context.
  */
  virtual std::string print() const 
  {
    std::string s = ((PulseContext)*this).print() +
      "dataobjectname \t\t = " + this->dataobjectname + "\n" +
      "accessmode \t\t = " + std::to_string(this->accessmode) + 
      " (" + ualconst::op_access_str.at(this->accessmode-OP_ACCESS_0) + ")\n" +
      "rangemode \t\t = " + std::to_string(this->rangemode) +
      " (" + ualconst::op_range_str.at(this->rangemode-OP_RANGE_0) + ")\n" +
      "time \t\t\t = " + std::to_string(this->time) + "\n" +
      "interpmode \t\t = " + std::to_string(this->interpmode) +
      " (" + ualconst::op_interp_str.at(this->interpmode-OP_INTERP_0) + ")\n";
    return s;
  }

  /**
     Returns the type of context.
     @result CTX_OPERATION_TYPE
  */
  virtual int getType() const 
  {
    return CTX_OPERATION_TYPE;
  }

  /**
     Returns the name of the DATAOBJECT.
     @result dataobjectname
  */
  std::string getDataobjectName() { return dataobjectname; }

  /**
     Returns access type of the operation.
     @result accessmode
     - READ_OP: read operation
     - WRITE_OP: write operation
     - REPLACE_OP: replace operation [_for the moment only in sliced mode for 
     "replace last slice"_]
  */
  int getAccessmode() { return accessmode; }

  /**
     Returns range of the operation.
     @result rangemode
     - GLOBAL_OP: operation on all slices [_also in non time-dependent case_]
     - SLICE_OP: operation on a single slice
  */
  int getRangemode() { return rangemode; }

  /**
     Returns time of the operation.
     @result time [_=UNDEFINED_TIME when time is not relevant_]
  */
  double getTime() { return time; }

  /**
     Returns type of interpolation selected for the operation.
     @result interpmode
     - CLOSEST_INTERP: consider slice at closest time
     - PREVIOUS_INTERP: consider slice at previous step
     - LINEAR_INTERP: interpolating linearly values at previous and next slices
     - UNDEFINED_INTERP: if not relevant [_e.g for write operations_]     
  */
  int getInterpmode() { return interpmode; }

protected:
  std::string dataobjectname;           /**< DATAOBJECT name */
  int accessmode;                       /**< operation access type */
  int rangemode;                        /**< operation range */
  double time;                          /**< operation time */
  int interpmode;                       /**< operation interpolation type */
};



/**
   Context class for an array of structures.
   The ArraystructContext is an OperationContext associated to a given array of structure.
*/
class ArraystructContext : public OperationContext 
{
 public:
  /**
     Array of structure context constructor.
     Requires informations for describing usage of stand-alone or top-most arrays of structure in a DATAOBJECT.
     @param ctx operation context
     @param p path of the array of structure field [_within the DATAOBJECT if standalone, within its container if nested_]
     @param tb path of the timebase associated with the array of structure
  */
  ArraystructContext(OperationContext ctx, std::string p, std::string tb)
    : OperationContext(ctx), path(p), timebase(tb)
  {
    parent = NULL;
    index = 0;
  }


  /**
     Array of structure context constructor.
     Requires informations for describing usage of nested arrays of structure in a DATAOBJECT.
     @param ctx operation context
     @param p path of the array of structure field [_within the DATAOBJECT if standalone, 
     within its container if nested_]
     @param tb path of the timebase associated with the array of structure
     @param cont context of the container array of structure [_optional: only in nested case_]
     @param idx index of the array of structure within its container [_optional: only in 
     nested case_]
     @param timed time dependency of the DATAOBJECT
  */
  ArraystructContext(OperationContext ctx, std::string p, std::string tb,  
		     ArraystructContext *cont)
    : OperationContext(ctx), path(p), timebase(tb), parent(cont)
  {
    index = 0;
  }
  /**
     Array of structure context constructor.
     Requires informations for describing usage of nested arrays of structure in a DATAOBJECT.
     @param ctx operation context
     @param p path of the array of structure field [_within the DATAOBJECT if standalone, 
     within its container if nested_]
     @param tb path of the timebase associated with the array of structure
     @param cont context of the container array of structure [_optional: only in nested case_]
     @param idx index of the array of structure within its container [_optional: only in 
     nested case_]
     @param timed time dependency of the DATAOBJECT
  */
  ArraystructContext(OperationContext ctx, std::string p, std::string tb,  
		     ArraystructContext *cont, int idx)
    : OperationContext(ctx), path(p), timebase(tb), parent(cont), index(idx)
  {
    index = 0;
  }

  /**
     Array of structure context destructor.
  */
  virtual ~ArraystructContext() {}

  friend std::ostream& operator<< (std::ostream& o, ArraystructContext const& ctx)
  {
    return o << ctx.print();
  }

  /**
     Prints content of the array of structure context.
  */
  virtual std::string print() const
  {
    std::string s = ((OperationContext)*this).print() +
      "path \t\t\t = " + this->path + "\n" +
      "timebase \t\t = " + this->timebase + "\n" +
      "timed \t\t\t = " + std::to_string(!timebase.empty()) + "\n" +
      "parent \t\t\t = " +
      ((this->parent==NULL)?"NULL":this->parent->path) + "\n" +
      "index \t\t\t = " + std::to_string(this->index) + "\n";
    return s;
  }

  /**
     Returns the type of context.
     @result CTX_ARRAYSTRUCT_TYPE
  */
  virtual int getType() const
  {
    return CTX_ARRAYSTRUCT_TYPE;
  }

  /**
     Returns the path of the array of structure.
     This path corresponds to the absolute path in the DATAOBJECT for the topmost array of structure or in.
     @result path
  */
  std::string getPath() { return path; }

 /**
     Returns the timebase path of the array of structure.
     This path corresponds to the absolute path in the DATAOBJECT for the topmost array of structure or in.
     @result path
  */
  std::string getTimebasePath() { return timebase; }

  /**
     Returns whether the array of structure is time-dependent or not.
     @result timed
  */
  bool getTimed() { return !timebase.empty(); }

  /**
     Returns the context of the container array of structure
     @result parent
  */
  ArraystructContext *getParent() { return parent; }

  /**
     Returns the position of the current element of interest within the array of structure.
     @result index
  */
  int getIndex() { return index; }

  /**
     Updates the position of the current element of interest within the array of structure.
     @param[in] step step size for setting the next index
  */
  void nextIndex(int step) { this->index += step; }


protected:
  std::string path;                     /**< path of the array of structure */
  std::string timebase;			/**< path of the timebase associated with the array of structure */
  ArraystructContext* parent;           /**< container of the array of structure */
  int index;                            /**< position of the current element of interest within the array of structure */
};


#endif

#endif
