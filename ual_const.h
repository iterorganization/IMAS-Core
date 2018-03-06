/*-*-c++-*-*/

/**
   \file ual_const.h
   Contains all constants used within the UAL low-levels.
*/
#ifndef UAL_CONST_H
#define UAL_CONST_H 1


/* c++ only part */
#if defined(__cplusplus)
#include <array>
#include <string>
#endif


/* C defs */

extern const int NO_BACKEND;
extern const int ASCII_BACKEND;
extern const int MDSPLUS_BACKEND;
extern const int HDF5_BACKEND;
extern const int MEMORY_BACKEND;

#include "ual_defs.h"



/* c++ only part */
#if defined(__cplusplus)
namespace ualconst {

  extern const int no_backend;
  extern const int ascii_backend;
  extern const int mdsplus_backend;
  extern const int hdf5_backend;
  extern const int memory_backend;
  extern const int uda_backend;

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
  const int nodata_err = NODATA_ERR;

  extern const std::array<std::string,5> ual_err_str;
}
#endif


#if defined(__cplusplus)
extern "C"
{
#endif

  const char * err2str(int err);

  /* c++ only part */
#if defined(__cplusplus)
}
#endif



#endif
