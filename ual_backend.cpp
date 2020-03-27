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
#include <map>
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

int extractOptions(const std::string& strOptions, std::map<std::string, std::string>& mapOptions)
{
	int iRet = 0;
	
	mapOptions.clear();
	
	std::smatch rxResults;
	std::regex rxOptions("\\s*([-/]?(\\w+))\\s*([=:]\\s*(\\w+))?\\s*");
	for (std::sregex_iterator i = std::sregex_iterator(strOptions.begin(), strOptions.end(), rxOptions); i != std::sregex_iterator(); ++i)
	{
		rxResults = *i;
		if (rxResults.size() >= 4)
		{
			mapOptions[rxResults.str(2)] = rxResults.str(4);
			iRet++;
		}
	}
	
	return iRet;
}

bool isOptionExist(const std::string& strOption, const std::map<std::string, std::string>& mapOptions, std::string& strValue)
{
	bool bRet = false;
	
	std::map<std::string, std::string>::const_iterator it = mapOptions.begin();
	while (it != mapOptions.end() && !bRet)
	{
		if (strcasecmp(strOption.c_str(), it->first.c_str()) == 0)
		{
			strValue = it->second;
			bRet = true;
		}
		else
		{
			it++;
		}
	}
	
	return bRet;
}

int compareVersion(const std::string& strV1, const std::string& strV2)
{
	int iRet = 0;
	
	// Search for x.y or x.y.z with different separators
	std::cmatch rxResults;
	std::regex rxVersion("([_\\d]+)[.\\-_]([\\d]+)[.\\-_]?([\\d]*)");
	
	if (std::regex_match(strV1.c_str(), rxResults, rxVersion) && rxResults.size() == 4)
	{
		int iX1 = atoi(rxResults[1].str().c_str());
		int iY1 = atoi(rxResults[2].str().c_str());
		int iZ1 = atoi(rxResults[3].str().c_str());
		
		if (std::regex_match(strV2.c_str(), rxResults, rxVersion) && rxResults.size() == 4)
		{
			int iX2 = atoi(rxResults[1].str().c_str());
			int iY2 = atoi(rxResults[2].str().c_str());
			int iZ2 = atoi(rxResults[3].str().c_str());
			
			if (iX1 == iX2)
			{
				if (iY1 == iY2)
				{
					if (iZ1 == iZ2)
					{
						iRet = 0;
					}
					else
					{
						iRet = iZ1 > iZ2 ? -1 : 1;
					}
				}
				else
				{
					iRet = iY1 > iY2 ? -1 : 1;
				}
			}
			else
			{
				iRet = iX1 > iX2 ? -1 : 1;
			}
		}
	}
	
	return iRet;
}
