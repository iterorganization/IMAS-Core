#include "ual_const.h"


const char * const2str(int id)
{
  auto it = ualconst::constmap.find(id);
  if (it == ualconst::constmap.end())
    return "";
  else
    return it->second.c_str();
}

const char * err2str(int id)
{
  auto it = ualerror::errmap.find(id);
  if (it == ualerror::errmap.end())
    return "";
  else
    return it->second.c_str();
}

const char * getUALVersion()
{
  return UAL_VERSION;
}

const char * getDDVersion()
{
  return DD_VERSION;
}
