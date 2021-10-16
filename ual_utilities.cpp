#include "ual_utilities.h"

#ifdef _MSC_VER 
//not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

int extractOptions(const std::string& strOptions, std::map<std::string, std::string>& mapOptions)
{
    int iRet = 0;
    
    mapOptions.clear();
    
#if HAVE_WORKING_REGEX
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
#else // HAVE_WORKING_REGEX
    const char* szDelim1 = " \t";
    const char* szDelim2 = "=:";
    char* szSavePtr1 = NULL;
    char* szSavePtr2 = NULL;
    const char* szOptions = strOptions.c_str();
    char* szPtr1Loc = NULL;
    char* szPtr2 = NULL;
    char* szPtr1 = strtok_r((char*)szOptions, szDelim1, &szSavePtr1);
    while (szPtr1)
    {
        szPtr1Loc = szPtr1;
        szPtr2 = strtok_r(szPtr1Loc, szDelim2, &szSavePtr2);
        if (szPtr2)
        {
            szPtr2 = strtok_r(NULL, szDelim2, &szSavePtr2);
            if (szPtr2)
            {
                mapOptions[std::string(szPtr1Loc)] = std::string(szPtr2);
            }
            else
            {
                mapOptions[std::string(szPtr1Loc)] = std::string("");
            }
        }
        else
        {
            mapOptions[std::string(szPtr1Loc)] = std::string("");
        }
        
        iRet++;
        szPtr1 = strtok_r(NULL, szDelim1, &szSavePtr1);
    }
#endif // HAVE_WORKING_REGEX
    
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
    int iX1 = 0;
    int iY1 = 0;
    int iZ1 = 0;
    int iX2 = 0;
    int iY2 = 0;
    int iZ2 = 0;
    
    if (strV1.length() > 0 && strV2.length() > 0)
    {
        // Search for x.y or x.y.z with different separators
#if HAVE_WORKING_REGEX
        std::cmatch rxResults;
        std::regex rxVersion("([\\d]+)[.\\-_]([\\d]+)[.\\-_]?([\\d]*)");
        
        if (std::regex_match(strV1.c_str(), rxResults, rxVersion) && rxResults.size() == 4)
        {
            iX1 = atoi(rxResults[1].str().c_str());
            iY1 = atoi(rxResults[2].str().c_str());
            iZ1 = atoi(rxResults[3].str().c_str());
            
            if (std::regex_match(strV2.c_str(), rxResults, rxVersion) && rxResults.size() == 4)
            {
                iX2 = atoi(rxResults[1].str().c_str());
                iY2 = atoi(rxResults[2].str().c_str());
                iZ2 = atoi(rxResults[3].str().c_str());
            }
        }
#else // HAVE_WORKING_REGEX
        int iPos = 0;
        const char* szDelim = ".-_";
        char* szSavePtr = NULL;
        const char* szVersion = strV1.c_str();
        char* szPtr = strtok_r((char*)szVersion, szDelim, &szSavePtr);
        while (szPtr)
        {
            if (iPos == 0)
            {
                iX1 = atoi(szPtr);
            }
            else if (iPos == 1)
            {
                iY1 = atoi(szPtr);
            }
            else if (iPos == 2)
            {
                iZ1 = atoi(szPtr);
            }
            
            iPos++;
            szPtr = strtok_r(NULL, szDelim, &szSavePtr);
        }
        
        iPos = 0;
        szSavePtr = NULL;
        szVersion = strV2.c_str();
        szPtr = strtok_r((char*)szVersion, szDelim, &szSavePtr);
        while (szPtr)
        {
            if (iPos == 0)
            {
                iX2 = atoi(szPtr);
            }
            else if (iPos == 1)
            {
                iY2 = atoi(szPtr);
            }
            else if (iPos == 2)
            {
                iZ2 = atoi(szPtr);
            }
            
            iPos++;
            szPtr = strtok_r(NULL, szDelim, &szSavePtr);
        }
#endif // HAVE_WORKING_REGEX
    }
    
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
    
    return iRet;
}