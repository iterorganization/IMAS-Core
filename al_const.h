/*-*-c++-*-*/

/**
   \file al_const.h
   Contains all constants used within the AL low-levels.
*/
#ifndef AL_CONST_H
#define AL_CONST_H 1

#include "al_defs.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

/* C defs */

#ifdef __cplusplus
extern "C" {
#endif

enum BACKEND
{
	NO_BACKEND      = BACKEND_ID_0,
	ASCII_BACKEND   = BACKEND_ID_0+1,
	MDSPLUS_BACKEND = BACKEND_ID_0+2,
	HDF5_BACKEND    = BACKEND_ID_0+3,
	MEMORY_BACKEND  = BACKEND_ID_0+4,
	UDA_BACKEND     = BACKEND_ID_0+5
};


#ifdef __cplusplus
}

#include <array>
#include <map>
#include <string>

namespace alconst {

  const int no_backend      = BACKEND::NO_BACKEND;
  const int ascii_backend   = BACKEND::ASCII_BACKEND;
  const int mdsplus_backend = BACKEND::MDSPLUS_BACKEND;
  const int hdf5_backend    = BACKEND::HDF5_BACKEND;
  const int memory_backend  = BACKEND::MEMORY_BACKEND;
  const int uda_backend     = BACKEND::UDA_BACKEND;

  const int global_op = GLOBAL_OP;
  const int slice_op = SLICE_OP;

  const int read_op = READ_OP;
  const int write_op = WRITE_OP;

  const int closest_interp = CLOSEST_INTERP;
  const int previous_interp = PREVIOUS_INTERP;
  const int linear_interp = LINEAR_INTERP;
  const int undefined_interp = UNDEFINED_INTERP;

  const double undefined_time = UNDEFINED_TIME;
  
  const int open_pulse = OPEN_PULSE;
  const int force_open_pulse = FORCE_OPEN_PULSE;
  const int create_pulse = CREATE_PULSE;
  const int force_create_pulse = FORCE_CREATE_PULSE;
  const int close_pulse = CLOSE_PULSE;
  const int erase_pulse = ERASE_PULSE;
    
  const int char_data = CHAR_DATA;
  const int integer_data = INTEGER_DATA;
  const int double_data = DOUBLE_DATA;
  const int complex_data = COMPLEX_DATA;

  const int ascii_serializer_protocol = ASCII_SERIALIZER_PROTOCOL;
  const int default_serializer_protocol = ASCII_SERIALIZER_PROTOCOL;

  const std::array<int,6> backend_id_list =
    {
      {
	NO_BACKEND,
	ASCII_BACKEND,
	MDSPLUS_BACKEND,
	HDF5_BACKEND,
	MEMORY_BACKEND,
	UDA_BACKEND
      }
    };
  const std::array<int,2> op_range_list =
    {
      {
	GLOBAL_OP,
	SLICE_OP
      }
    };
  const std::array<int,3> op_access_list =
    {
      {
	READ_OP,
	WRITE_OP,
      }
    };
  const std::array<int,4> op_interp_list =
    {
      {
	UNDEFINED_INTERP,
	CLOSEST_INTERP,
	PREVIOUS_INTERP,
	LINEAR_INTERP
      }
    };
  const std::array<int,6> access_pulse_list =
    {
      {
	OPEN_PULSE,
	FORCE_OPEN_PULSE,
	CREATE_PULSE,
	FORCE_CREATE_PULSE,
	CLOSE_PULSE,ERASE_PULSE
      }
    };
  const std::array<int,4> data_type_list =
    {
      {CHAR_DATA,
       INTEGER_DATA,
       DOUBLE_DATA,
       COMPLEX_DATA
      }
    };
  const std::array<int,1> serializer_protocol_list =
    {
      {
	ASCII_SERIALIZER_PROTOCOL
      }
    };
  
  const std::map<int,std::string> constmap =
    {
      {NO_BACKEND, "NO_BACKEND"},
      {ASCII_BACKEND, "ASCII_BACKEND"},
      {MDSPLUS_BACKEND, "MDSPLUS_BACKEND"},
      {HDF5_BACKEND, "HDF5_BACKEND"},
      {MEMORY_BACKEND, "MEMORY_BACKEND"},
      {UDA_BACKEND, "UDA_BACKEND"},
      {GLOBAL_OP, "GLOBAL_OP"},
      {SLICE_OP, "SLICE_OP"},
      {READ_OP, "READ_OP"},
      {WRITE_OP, "WRITE_OP"},
      {REPLACE_OP, "REPLACE_OP"},
      {UNDEFINED_INTERP, "UNDEFINED_INTERP"},
      {CLOSEST_INTERP, "CLOSEST_INTERP"},
      {PREVIOUS_INTERP, "PREVIOUS_INTERP"},
      {LINEAR_INTERP, "LINEAR_INTERP"},
      {UNDEFINED_TIME, "UNDEFINED_TIME"},
      {OPEN_PULSE, "OPEN_PULSE"},
      {FORCE_OPEN_PULSE, "FORCE_OPEN_PULSE"},
      {CREATE_PULSE, "CREATE_PULSE"},
      {FORCE_CREATE_PULSE, "FORCE_CREATE_PULSE"},
      {CLOSE_PULSE, "CLOSE_PULSE"},
      {ERASE_PULSE, "ERASE_PULSE"},
      {CHAR_DATA, "CHAR_DATA"},
      {INTEGER_DATA, "INTEGER_DATA"},
      {DOUBLE_DATA, "DOUBLE_DATA"},
      {COMPLEX_DATA, "COMPLEX_DATA"},
      {ASCII_SERIALIZER_PROTOCOL, "ASCII_SERIALIZER_PROTOCOL"}
    };
}

namespace alerror {

  const int unknown_err = UNKNOWN_ERR;
  const int context_err = CONTEXT_ERR;
  const int backend_err = BACKEND_ERR;
  const int lowlevel_err = LOWLEVEL_ERR;

  const std::map<int,std::string> errmap =
    {
      {UNKNOWN_ERR, "UNKNOWN_ERR"},
      {CONTEXT_ERR, "CONTEXT_ERR"},
      {BACKEND_ERR, "BACKEND_ERR"},
      {LOWLEVEL_ERR, "LOWLEVEL_ERR"}
    };
}

namespace plugin{

  enum OPERATION
  {
    PUT_ONLY      = 1,
    GET_ONLY      = 2,
    PUT_AND_GET   = 3
  };
} 

#endif

#ifdef __cplusplus
extern "C" {
#endif


  /**
     Returns the String of the passed constant identifier.
     @param[in] id constant ID
     @result String associated with the constant ID
  */
  LIBRARY_API const char * const2str(int id);
  
  /**
     Returns the String of the passed integer error.
     @param[in] id error ID
     @result String associated with the error ID
  */
  LIBRARY_API const char * err2str(int id);
  
  /**
     Returns the String of the AL version.
     @result String of the AL version
  */
  LIBRARY_API const char * getALVersion();
  
  /**
     Returns the String of Data Dictionary version.
     @result String of the Data Dictionary version
  */
  LIBRARY_API const char * getDDVersion();
  
  
#if defined(__cplusplus)
}
#endif

#endif // AL_CONST_H
