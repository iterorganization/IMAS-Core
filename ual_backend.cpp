#include "ual_backend.h"
#include "no_backend.h"
#include "memory_backend.h"
#ifdef ASCII
#include "ascii_backend.h"
#endif
#ifdef MDSPLUS
#include "mdsplus_backend.h"
#endif
#ifdef HDF5
#include "hdf5_backend.h"
#endif
#ifdef UDA
#include "uda_backend.h"
#endif

#include <regex>
#include <vector>
#include <string>

#if !defined(__GNUC__) && !defined(__clang__)
#define strcasecmp _stricmp
#endif


Backend* Backend::initBackend(int id)
{
  Backend *be = NULL;

  if (id==ualconst::no_backend)
    {
      NoBackend* tbe = new NoBackend();
      be = tbe;
    }
  else if (id==ualconst::ascii_backend)
    {
#ifdef ASCII
      AsciiBackend* tbe = new AsciiBackend();
      be = tbe;
#else
      throw UALBackendException("ASCII backend is not available within current install",LOG);
#endif
    }
  else if (id==ualconst::mdsplus_backend)
    {
#ifdef MDSPLUS
      MDSplusBackend* tbe = new MDSplusBackend();
      be = tbe;
#else
      throw UALBackendException("MDSplus backend is not available within current install",LOG);
#endif
    }
  else if (id==ualconst::hdf5_backend)
    {
#ifdef HDF5
      HDF5Backend* tbe = new HDF5Backend();
      be = tbe;
#else
      throw UALBackendException("HDF5 backend is not available within current install",LOG);
#endif
    }
  else if (id==ualconst::memory_backend)
    {
      MemoryBackend* tbe = new MemoryBackend();
      be = tbe;
    }
  else if (id==ualconst::uda_backend)
    {
#ifdef UDA
      UDABackend* tbe = new UDABackend(true);
      be = tbe;
#else
      throw UALBackendException("UDA backend is not available within current install",LOG);
#endif
    }
  else
    {
      std::cerr << "Non-identified backend ID (" << id << ")\n";
      throw UALBackendException("Wrong backend identifier "+std::to_string(id),LOG);
    }

  return be;
}


int extractOptions(const std::string& strOptions, std::vector<std::string>& vecOptions)
{
	int iRet = 0;
	
	std::smatch rxResults;
	std::regex rxOptions("\\s*([-/]?(\\w+))\\s*([=:]\\s*(\\w+))?\\s*");
	
	for (std::sregex_iterator i = std::sregex_iterator(strOptions.begin(), strOptions.end(), rxOptions); i != std::sregex_iterator(); ++i)
	{
		rxResults = *i;
		if (rxResults.size() >= 4)
		{
			vecOptions.push_back(rxResults.str(2));
			// For the future...
			//vecValues.push_back(rxResults.str(4));
			iRet++;
		}
	}
	
	return iRet;
}

bool isOptionExist(const std::string& strOption, const std::vector<std::string>& vecOptions)
{
	bool bRet = false;
	
	std::vector<std::string>::const_iterator it = vecOptions.begin();
	while (it != vecOptions.end() && !bRet)
	{
		if (strcasecmp(strOption.c_str(), it->c_str()) == 0)
		{
			bRet = true;
		}
		else
		{
			it++;
		}
	}
	
	return bRet;
}
