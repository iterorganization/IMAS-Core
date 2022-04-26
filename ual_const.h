/*-*-c++-*-*/

/**
   \file ual_const.h
   Contains all constants used within the UAL low-levels.
*/
#ifndef UAL_CONST_H
#define UAL_CONST_H 1

#include "ual_defs.h"

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
#include <string>

namespace ualconst {

  const int no_backend      = BACKEND::NO_BACKEND;
  const int ascii_backend   = BACKEND::ASCII_BACKEND;
  const int mdsplus_backend = BACKEND::MDSPLUS_BACKEND;
  const int hdf5_backend    = BACKEND::HDF5_BACKEND;
  const int memory_backend  = BACKEND::MEMORY_BACKEND;
  const int uda_backend     = BACKEND::UDA_BACKEND;

  const int timed = TIMED;
  const int non_timed = NON_TIMED;
  
  const int global_op = GLOBAL_OP;
  const int slice_op = SLICE_OP;

  const int read_op = READ_OP;
  const int write_op = WRITE_OP;
  const int replace_op = REPLACE_OP;

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

  const std::array<std::string,6> backend_id_str = {{"NO_BACKEND","ASCII_BACKEND","MDSPLUS_BACKEND","HDF5_BACKEND","MEMORY_BACKEND","UDA_BACKEND"}};
  const std::array<std::string,2> op_range_str = {{"GLOBAL_OP","SLICE_OP"}};
  const std::array<std::string,3> op_access_str = {{"READ_OP","WRITE_OP","REPLACE_OP"}};
  const std::array<std::string,4> op_interp_str = {{"UNDEFINED_INTERP","CLOSEST_INTERP","PREVIOUS_INTERP","LINEAR_INTERP"}};
  const std::array<std::string,6> access_pulse_str = {{"OPEN_PULSE","FORCE_OPEN_PULSE","CREATE_PULSE","FORCE_CREATE_PULSE","CLOSE_PULSE","ERASE_PULSE"}};
  const std::array<std::string,4> data_type_str = {{"CHAR_DATA","INTEGER_DATA","DOUBLE_DATA","COMPLEX_DATA"}};
  const std::array<std::string,1> serializer_protocol_str = {{"ASCII_SERIALIZER_PROTOCOL"}};
}

namespace ualerror {

  const int unknown_err = UNKNOWN_ERR;
  const int context_err = CONTEXT_ERR;
  const int backend_err = BACKEND_ERR;
  const int lowlevel_err = LOWLEVEL_ERR;

  const std::array<std::string,4> ual_err_str = {{"UNKNOWN_ERR","CONTEXT_ERR","BACKEND_ERR","LOWLEVEL_ERR"}};
}

#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
   Returns the String of the passed integer type.
   @result String of the type
*/
LIBRARY_API const char * type2str(int type);
/**
   Returns the String of the passed integer error.
   @result String of the error
*/
LIBRARY_API const char * err2str(int err);

/**
   Returns the String of the UAL version.
   @result String of the UAL version
*/
LIBRARY_API const char * getUALVersion();
/**
   Returns the String of Data Dictionary version.
   @result String of the Data Dictionary version
*/
LIBRARY_API const char * getDDVersion();


#if defined(__cplusplus)
}
#endif

#endif // UAL_CONST_H
