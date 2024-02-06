#include "al_const.h"


const char * const2str(int id)
{
  auto it = alconst::constmap.find(id);
  if (it == alconst::constmap.end())
    return "";
  else
    return it->second.c_str();
}

const char * err2str(int id)
{
  auto it = alerror::errmap.find(id);
  if (it == alerror::errmap.end())
    return "";
  else
    return it->second.c_str();
}

const char * getALVersion()
{
  return AL_VERSION;
}

const char * getDDVersion()
{
  return DD_VERSION;
}
