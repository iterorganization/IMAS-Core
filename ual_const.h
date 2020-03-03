/*-*-c++-*-*/

/**
   \file ual_const.h
   Contains all constants used within the UAL low-levels.
*/
#ifndef UAL_CONST_H
#define UAL_CONST_H 1

#include <array>
#include <string>

#include "ual_defs.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif


const int NO_BACKEND      = BACKEND_ID_0;
const int ASCII_BACKEND   = BACKEND_ID_0+1;
const int MDSPLUS_BACKEND = BACKEND_ID_0+2;
const int HDF5_BACKEND    = BACKEND_ID_0+3;
const int MEMORY_BACKEND  = BACKEND_ID_0+4;
const int UDA_BACKEND     = BACKEND_ID_0+5;


namespace ualconst {

  const int no_backend      = NO_BACKEND;
  const int ascii_backend   = ASCII_BACKEND;
  const int mdsplus_backend = MDSPLUS_BACKEND;
  const int hdf5_backend    = HDF5_BACKEND;
  const int memory_backend  = MEMORY_BACKEND;
  const int uda_backend     = UDA_BACKEND;

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

  extern const std::array<std::string,6> backend_id_str;
  extern const std::array<std::string,2> op_range_str;
  extern const std::array<std::string,3> op_access_str;
  extern const std::array<std::string,4> op_interp_str;
  extern const std::array<std::string,6> access_pulse_str;
  extern const std::array<std::string,4> data_type_str;
}

namespace ualerror {

  const int unknown_err = UNKNOWN_ERR;
  const int context_err = CONTEXT_ERR;
  const int backend_err = BACKEND_ERR;
  const int lowlevel_err = LOWLEVEL_ERR;

  extern const std::array<std::string,4> ual_err_str;
}

LIBRARY_API const char * type2str(int type);
LIBRARY_API const char * err2str(int err);


#if defined(__cplusplus)
}
#endif

#endif // UAL_CONST_H
