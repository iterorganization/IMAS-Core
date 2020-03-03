#include "ual_const.h"

const std::array<std::string,6> ualconst::backend_id_str =
  {{"NO_BACKEND","ASCII_BACKEND","MDSPLUS_BACKEND","HDF5_BACKEND","MEMORY_BACKEND","UDA_BACKEND"}};

const std::array<std::string,2> ualconst::op_range_str =
  {{"GLOBAL_OP","SLICE_OP"}};

const std::array<std::string,3> ualconst::op_access_str =
  {{"READ_OP","WRITE_OP","REPLACE_OP"}};

const std::array<std::string,4> ualconst::op_interp_str =
  {{"UNDEFINED_INTERP","CLOSEST_INTERP","PREVIOUS_INTERP","LINEAR_INTERP"}};

const std::array<std::string,6> ualconst::access_pulse_str = 
  {{"OPEN_PULSE","FORCE_OPEN_PULSE","CREATE_PULSE","FORCE_CREATE_PULSE",
    "CLOSE_PULSE","ERASE_PULSE"}};

const std::array<std::string,4> ualconst::data_type_str = 
  {{"CHAR_DATA","INTEGER_DATA","DOUBLE_DATA","COMPLEX_DATA"}};


const std::array<std::string,4> ualerror::ual_err_str = 
  {{"UNKNOWN_ERR","CONTEXT_ERR","BACKEND_ERR","LOWLEVEL_ERR"}};


const char * type2str(int type)
{
  return (ualconst::data_type_str.at(type-DATA_TYPE_0)).c_str();
}

const char * err2str(int err)
{
  return (ualerror::ual_err_str.at(ERR_0-err)).c_str();
}


