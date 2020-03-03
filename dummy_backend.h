#ifndef DUMMY_BACKEND_H
#define DUMMY_BACKEND_H 1

#include "ual_backend.h"


#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif


class LIBRARY_API DummyBackend:public Backend 
{

public:
  // virtual desctructor
    DummyBackend() 
    {
    }
  /**
     Opens a database entry.
     This function opens a database entry described by the passed pulse context.
     @param[in] ctx pointer on pulse context
     @param[in] mode opening option:
     - OPEN_PULSE = open an existing pulse (only if exist)
     - FORCE_OPEN_PULSE = open a pulse (create it if not exist)
     - CREATE_PULSE = create a new pulse (do not overwrite if already exist)
     - FORCE_CREATE_PULSE = create a new pulse (erase old one if already exist)
     @param[in] options additional options, ex: "name=euitm refShot=1 refRun=2"
     (possibly backend specific)
     @throw BackendException
  */
    virtual void openPulse(PulseContext *ctx,
			 int mode,
			 std::string options)
    {
    }

  /**
     Closes a database entry.
     This function closes a database entry described by the passed pulse context.
     @param[in] ctx pointer on pulse context
     @param[in] mode closing option:
     - CLOSE_PULSE = close the pulse
     - ERASE_PULSE = close and remove the pulse 
     @param[in] options additional options (possibly backend specific)
     @throw BackendException
  */
    virtual void closePulse(PulseContext *ctx,
			  int mode,
			  std::string options)

    {
    }

  /**
     Writes data.
     This function writes a signal in the database given the passed operation context.
     @param[in] ctx pointer on operation context
     @param[in] fieldname field name 
     @param[in] timebasename time base field name
     @param[in] data pointer on the data to be written
     @param[in] datatype type of data to be written:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[in] dim dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[in] size array of the size of each dimension (NULL is dim=0)
     @throw BackendException
  */
     void writeData(Context *ctx,
			 std::string fieldname,
			 std::string timebasename, 
			 void* data,
			 int datatype,
			 int dim,
			 int* size)
    {
	throw  UALBackendException("writeData method cannot be called for Dummy Backend",LOG);
    }


  /**
     Reads data.
     This function reads a signal in the database given the passed operation context.
     @param[in] ctx pointer on operation context
     @param[in] fieldname field name
     @param[in] istimed specify the time-dependency of the field
     @param[out] data returned pointer on the read data 
     @param[out] datatype type of data to be read:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[out] dim returned dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[out] size array returned with elements filled at the size of each dimension 
     @throw BackendException
  */
  virtual int readData(Context *ctx,
			std::string fieldname,
			std::string timebase,
			void** data,
			int* datatype,
			int* dim,
			int* size)
    {
	throw  UALBackendException("readData method cannot be called for Dummy Backend",LOG);
	return 0;
    }

  /*
    Deletes data.
    This function deletes some data (can be a signal, a structure, the whole DATAOBJECT) in the database 
    given the passed context.
    @param[in] ctx pointer on operation context 
    @param[in] path path of the data structure element to delete (suppress the whole subtree)
    @throw BackendException
  **/
    virtual void deleteData(OperationContext *ctx,
			  std::string path)
    {
	throw  UALBackendException("deleteData method cannot be called for Dummy Backend",LOG);
    }
    virtual void beginArraystructAction(ArraystructContext *ctx,
				      int *size)
    {
	//throw  UALBackendException("beginArraystructAction method cannot be called for Dummy Backend",LOG);
	*size = 0;
    }

  	virtual void endAction(Context *ctx){}
		
  	virtual void beginAction(OperationContext *ctx){}

};

#ifdef __cplusplus
}
#endif

#endif // DUMMY_BACKEND_H
