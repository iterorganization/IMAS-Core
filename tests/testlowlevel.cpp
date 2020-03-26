#include <stdio.h>
#include <string.h>
#include <regex>
#include <ual_lowlevel.h>

#ifndef WIN32
#define _countof(x) sizeof(x)
#define _stricmp(x, y) strcasecmp(x, y)
#define strcpy_s(a, b, c) strcpy(a, c)

const char* strupr(char* s)
{
	char* p = s;
	while (*p = toupper(*p))
	{
		p++;
	}
	return s;
}
#endif


void usage()
{
	printf("testlowlevel\n");
	printf("\n");
	printf("usage: testlowlevel [-s=shot] [-r=run] [-b=backend] [-u=user] [-m=machine] [-o=action] [-c=action] [-p=param] [-h]\n");
	printf("\n");
	printf("with:\n");
	printf("\t-s\tShot number\n");
	printf("\t-r\tRun number\n");
	printf("\t-b\tBackend to use: M=MDSplus (default), U=UDA, H=HDF5\n");
	printf("\t-u\tUser name\n");
	printf("\t-m\tMachine name\n");
	printf("\t-o\tOpen action to do: O=Open (default), C=Create\n");
	printf("\t-c\tClose action to do: C=Close (default), E=Close and erase\n");
	printf("\t-p\tParameters to pass at UAL\n");
	printf("\t-h\tShow this help\n");
	printf("\n");
}

// Format: -x /x
bool ExtractOpt(char& cOption, const char* szString)
{
	bool bRet = false;
	std::cmatch rxResults;
	std::regex rxInt("([\\s\\r\\n]*)([-/][a-zA-Z])([\\s\\r\\n]*)");
	
	if (std::regex_match(szString, rxResults, rxInt) && rxResults.size() == 4)
	{
		cOption = rxResults[2].str().c_str()[1];
		bRet = true;
	}

	return bRet;
}

// Format: -x=123 /x:123
bool ExtractInt(char& cOption, int& iValue, const char* szString)
{
	bool bRet = false;
	std::cmatch rxResults;
	std::regex rxInt("([\\s\\r\\n]*)([-/][a-zA-Z][=:])([+-]?[\\d]+)([\\s\\r\\n]*)");
	
	if (std::regex_match(szString, rxResults, rxInt) && rxResults.size() == 5)
	{
		cOption = rxResults[2].str().c_str()[1];
		iValue = atoi(rxResults[3].str().c_str());
		bRet = true;
	}

	return bRet;
}

// Format: -x=azerty /x:azerty
bool ExtractString(char& cOption, char* szValue, size_t sSize, const char* szString)
{
	bool bRet = false;
	std::cmatch rxResults;
	std::regex rxStr("([\\s\\r\\n]*)([-/][a-zA-Z][=:])(.+)([\\s\\r\\n]*)");

	if (std::regex_match(szString, rxResults, rxStr) && rxResults.size() == 5)
	{
		cOption = rxResults[2].str().c_str()[1];
		strcpy_s(szValue, sSize, rxResults[3].str().c_str());
		bRet = true;
	}
	
	return bRet;
}

int main(int argc, char *argv[])
{
	bool bUsage = false;
	bool bRead = false;
	bool bWrite = false;
	int iRet = 0;
	int iShot = 12;
	int iRun = 0;
	int iBackend = MDSPLUS_BACKEND;
	int iOpenAction = FORCE_OPEN_PULSE;
	int iCloseAction = CLOSE_PULSE;
	int iTemp = 0;
	char cOption = 0;
	char szTemp[1024] = { 0 };
	char szUser[1024] = "imas_public";
	char szTokamak[1024] = "west";
	char szVersion[1024] = "3";
	char szParams[1024] = "";
	
	printf("UAL version:\t%s\n", getUALVersion());
	printf("DD version:\t%s\n", getDDVersion());
	printf("\n");
	
	for (int i = 1; i < argc; i++)
	{
		if (ExtractOpt(cOption, argv[i]))
		{
			if (cOption == 'h')
			{
				bUsage = true;
			}
		}
		else if (ExtractInt(cOption, iTemp, argv[i]))
		{
			if (cOption == 's')
			{
				iShot = iTemp;
			}
			else if (cOption == 'r')
			{
				iRun = iTemp;
			}
		}
		else if (ExtractString(cOption, szTemp, _countof(szTemp), argv[i]))
		{
			if (cOption == 'b')
			{
				if (_stricmp(szTemp, "M") == 0)
				{
					iBackend = MDSPLUS_BACKEND;
					// Read/write
					bRead = true;
					bWrite = true;
				}
				else if (_stricmp(szTemp, "U") == 0)
				{
					iBackend = UDA_BACKEND;
					// Read only
					bRead = true;
					// Machine in CAPITAL
					strupr(szTokamak);
				}
				else if (_stricmp(szTemp, "H") == 0)
				{
					iBackend = HDF5_BACKEND;
					// Read/write
					bRead = true;
					bWrite = true;
				}
				else
				{
					bUsage = true;
				}
			}
			else if (cOption == 'u')
			{
				strcpy(szUser, szTemp);
			}
			else if (cOption == 'm')
			{
				strcpy(szTokamak, szTemp);
			}
			else if (cOption == 'o')
			{
				if (_stricmp(szTemp, "O") == 0)
				{
					iOpenAction = FORCE_OPEN_PULSE;
				}
				else if (_stricmp(szTemp, "C") == 0)
				{
					iOpenAction = FORCE_CREATE_PULSE;
				}
				else
				{
					bUsage = true;
				}
			}
			else if (cOption == 'c')
			{
				if (_stricmp(szTemp, "C") == 0)
				{
					iCloseAction = CLOSE_PULSE;
				}
				else if (_stricmp(szTemp, "E") == 0)
				{
					iCloseAction = ERASE_PULSE;
				}
				else
				{
					bUsage = true;
				}
			}
			else if (cOption == 'p')
			{
				strcpy(szParams, szTemp);
			}
		}
	}
	
	if (bUsage)
	{
		usage();
	}
	else
	{		
		printf("Shot number:\t%d\n", iShot);
		printf("Run number:\t%d\n", iRun);
		printf("Backend:\t%s\n", ualconst::backend_id_str[iBackend - BACKEND_ID_0].c_str());
		printf("User name:\t%s\n", szUser);
		printf("Machine:\t%s\n", szTokamak);
		printf("Open action:\t%s\n", ualconst::access_pulse_str[iOpenAction - ACCESS_PULSE_0].c_str());
		printf("Close action:\t%s\n", ualconst::access_pulse_str[iCloseAction - ACCESS_PULSE_0].c_str());
		printf("Parameters:\t%s\n", szParams);
		printf("\n");
		
		// Low Level
		int iPulseCtx = -1;
		al_status_t alStatus = ual_begin_pulse_action(iBackend, iShot, iRun, szUser, szTokamak, szVersion, &iPulseCtx);
		if (alStatus.code != 0)
		{
			printf("Error opening imas action ctx for shot %d, run %d: ual_begin_pulse_action\n", iShot, iRun);
			iRet = alStatus.code;
		}
		else
		{
			printf("Opening imas pulse action ctx %d OK!\n", iPulseCtx);
			
			char* szInfo = NULL;
			alStatus = ual_context_info(iPulseCtx, &szInfo);
			if (alStatus.code == 0)
			{
				printf("Context info:\n%s\n", szInfo);
				free(szInfo);
			}
			
			alStatus = ual_open_pulse(iPulseCtx, iOpenAction, szParams);
			if (alStatus.code != 0)
			{
				printf("Error opening imas pulse ctx %d: ual_open_pulse\n", iPulseCtx);
				iRet = alStatus.code;
			}
			else
			{
				printf("Opening imas pulse ctx %d OK!\n", iPulseCtx);
				
				int iGetOpCtx = -1;
				int iValue = 123456789;
				const char* szFieldPath = "ids_properties/homogeneous_time";
				const char* szTimeBasePath = "";
				const char* szIdsFullName = "magnetics";
				
				if (bWrite)
				{
					printf("\r============================\n");
					printf("Writing data...\n");
					
					alStatus = ual_begin_global_action(iPulseCtx, szIdsFullName, WRITE_OP, &iGetOpCtx);
					if (alStatus.code != 0) 
					{
						printf("Error opening imas %s for writing: ual_begin_global_action\n", szIdsFullName);
						iRet = alStatus.code;
					}
					else
					{
						printf("Opening imas global ctx %d OK!\n", iGetOpCtx);
						
						alStatus = ual_write_data(iGetOpCtx, szFieldPath, szTimeBasePath, (void*)(&iValue), INTEGER_DATA, 0, NULL);
						if (alStatus.code != 0)
						{
							printf("Error writing imas global ctx %d: ual_write_data\n", iGetOpCtx);
							iRet = alStatus.code;
						}
						else
						{
							printf("Wrting data in global ctx %d OK!\n", iGetOpCtx);
						}
						
						alStatus = ual_end_action(iGetOpCtx);
					}
				}
				
				if (bRead)
				{
					printf("\r============================\n");
					printf("Reading data...\n");
					
					alStatus = ual_begin_global_action(iPulseCtx, szIdsFullName, READ_OP, &iGetOpCtx);
					if (alStatus.code != 0) 
					{
						printf("Error opening imas %s for reading: ual_begin_global_action\n", szIdsFullName);
						iRet = alStatus.code;
					}
					else
					{
						printf("Opening imas global ctx %d OK!\n", iGetOpCtx);
						
						int iTemp = -1;
						void* pData = &iTemp;
						int retSize[MAXDIM] = { 0 };
						
						alStatus = ual_read_data(iGetOpCtx, szFieldPath, szTimeBasePath, (void**)&pData, INTEGER_DATA, 0, &retSize[0]);
						if (alStatus.code != 0)
						{
							printf("Error reading imas global ctx %d: ual_read_data\n", iGetOpCtx);
							iRet = alStatus.code;
						}
						else
						{
							printf("Reading imas global ctx %d OK! -> %d\n", iGetOpCtx, iTemp);
						}
						
						alStatus = ual_end_action(iGetOpCtx);
					}
				}
				
				alStatus = ual_close_pulse(iPulseCtx, iCloseAction, "");
			}
		}
	}
	
	return iRet;
}
