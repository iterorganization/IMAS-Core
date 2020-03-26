#include "ual_const.h"


const char * type2str(int type)
{
  return (ualconst::data_type_str.at(type-DATA_TYPE_0)).c_str();
}

const char * err2str(int err)
{
  return (ualerror::ual_err_str.at(ERR_0-err)).c_str();
}

const char * getUALVersion()
{
  return UAL_VERSION;
}

const char * getDDVersion()
{
  return DD_VERSION;
}
