#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <ual_lowlevel.h>

#ifndef WIN32
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
	printf("\t-b\tBackend to use: M=MDSplus (default), A=ASCII, U=UDA, H=HDF5\n");
	printf("\t-u\tUser name\n");
	printf("\t-m\tMachine name\n");
	printf("\t-o\tOpen action to do: O=Open (default), C=Create\n");
	printf("\t-c\tClose action to do: C=Close (default), E=Close and erase\n");
	printf("\t-p\tParameters to pass at UAL\n");
	printf("\t-h\tShow this help\n");
	printf("\n");
}

// Format: -x /x
bool ExtractOpt(char* pcOption, const char* szString)
{
	bool bRet = false;
	
	if (strlen(szString) == 2 && (szString[0] == '/' || szString[0] == '-'))
	{
		*pcOption = szString[1];
		bRet = true;
	}
	
	return bRet;
}

// Format: -x=123 /x:123
bool ExtractInt(char* pcOption, int* piValue, const char* szString)
{
	bool bRet = false;
	
	if (strlen(szString) >= 4 && (szString[0] == '/' || szString[0] == '-'))
	{
		bool bDigit = true;
		for (int i = 3; i < strlen(szString) && bDigit; i++)
		{
			bDigit = isdigit(szString[i]);
		}
		if (bDigit)
		{
			*pcOption = szString[1];
			*piValue = atoi(szString + 3);
			bRet = true;
		}
	}

	return bRet;
}

// Format: -x=azerty /x:azerty
bool ExtractString(char* pcOption, char* szValue, size_t sSize, const char* szString)
{
	bool bRet = false;

	if (strlen(szString) >= 4 && (szString[0] == '/' || szString[0] == '-'))
	{
		*pcOption = szString[1];
		strcpy_s(szValue, sSize, szString + 3);
		bRet = true;
	}
	
	return bRet;
}

int main(int argc, char *argv[])
{
	bool bUsage = false;
	bool bRead = true;
	bool bWrite = true;
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
		if (ExtractOpt(&cOption, argv[i]))
		{
			if (cOption == 'h')
			{
				bUsage = true;
			}
		}
		else if (ExtractInt(&cOption, &iTemp, argv[i]))
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
		else if (ExtractString(&cOption, szTemp, 1024, argv[i]))
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
				else if (_stricmp(szTemp, "A") == 0)
				{
					iBackend = ASCII_BACKEND;
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
		printf("Backend:\t%d\n", iBackend);
		printf("User name:\t%s\n", szUser);
		printf("Machine:\t%s\n", szTokamak);
		printf("Open action:\t%d\n", iOpenAction);
		printf("Close action:\t%d\n", iCloseAction);
		printf("Parameters:\t%s\n", szParams);
		printf("\n");
		
		// Low Level
		int iPulseCtx = -1;
        char* uri;
        ual_build_uri_from_legacy_parameters(iBackend, iShot, iRun, szUser, szTokamak, szVersion, &uri);
        al_status_t alStatus = ual_begin_dataentry_action(uri, iOpenAction, &iPulseCtx);
        iRet = alStatus.code;

		if (alStatus.code != 0)
		{
			printf("Error opening imas action ctx for shot %d, run %d: ual_begin_dataentry_action = %s\n", iShot, iRun, alStatus.message);
		}
		else
		{
			printf("Opening imas pulse action ctx %d OK!\n", iPulseCtx);
			
			char* szInfo = NULL;
			alStatus = ual_context_info(iPulseCtx, &szInfo);
			if (alStatus.code == 0 && szInfo)
			{
				printf("Context info:\n%s\n", szInfo);
				free(szInfo);
			}
			else
			{
				printf("Error reading context info = %s\n", alStatus.message);
			}

			printf("Opening imas pulse ctx %d OK!\n", iPulseCtx);
			
			int iGetOpCtx = -1;
			int iValue = 1;
			const char* szValue = "test";
			const char* szFieldPath = "ids_properties/homogeneous_time";
			const char* szFieldPath2 = "ids_properties/comment";
			const char* szTimeBasePath = "";
			const char* szIdsFullName = "magnetics";
			
			if (bWrite)
			{
				printf("\n============================\n");
				printf("Writing data...\n");
				
				iGetOpCtx = -1;
				alStatus = ual_begin_global_action(iPulseCtx, szIdsFullName, WRITE_OP, &iGetOpCtx);
				if (alStatus.code != 0) 
				{
					printf("Error opening imas %s for writing: ual_begin_global_action = %s\n", szIdsFullName, alStatus.message);
					iRet = alStatus.code;
				}
				else
				{
					printf("Opening imas global ctx %d OK!\n", iGetOpCtx);
					
					alStatus = ual_write_data(iGetOpCtx, szFieldPath, szTimeBasePath, (void*)(&iValue), INTEGER_DATA, 0, NULL);
					if (alStatus.code != 0)
					{
						printf("Error writing integer imas global ctx %d: ual_write_data = %s\n", iGetOpCtx, alStatus.message);
						iRet = alStatus.code;
					}
					else
					{
						printf("Wrting integer data in global ctx %d OK!\n", iGetOpCtx);
					}
					
					int arrayOfSizes[1] = { (int)strlen(szValue) };
					alStatus = ual_write_data(iGetOpCtx, szFieldPath2, szTimeBasePath, (void*)(szValue), CHAR_DATA, 1, arrayOfSizes);
					if (alStatus.code != 0)
					{
						printf("Error writing string imas global ctx %d: ual_write_data = %s\n", iGetOpCtx, alStatus.message);
						iRet = alStatus.code;
					}
					else
					{
						printf("Wrting string data in global ctx %d OK!\n", iGetOpCtx);
					}
					
					alStatus = ual_end_action(iGetOpCtx);
				}
			}
			
			if (bRead)
			{
				printf("\n============================\n");
				printf("Reading data...\n");
				
				iGetOpCtx = -1;
				alStatus = ual_begin_global_action(iPulseCtx, szIdsFullName, READ_OP, &iGetOpCtx);
				if (alStatus.code != 0) 
				{
					printf("Error opening imas %s for reading: ual_begin_global_action = %s\n", szIdsFullName, alStatus.message);
					iRet = alStatus.code;
				}
				else
				{
					printf("Opening imas global ctx %d OK!\n", iGetOpCtx);
					
					int iTemp = -1;
					void* pData = &iTemp;
					char* szTemp = NULL;
					int retSize[MAXDIM] = { 0 };
					
					alStatus = ual_read_data(iGetOpCtx, szFieldPath, szTimeBasePath, (void**)&pData, INTEGER_DATA, 0, &retSize[0]);
					if (alStatus.code != 0)
					{
						printf("Error reading integer imas global ctx %d: ual_read_data = %s\n", iGetOpCtx, alStatus.message);
						iRet = alStatus.code;
					}
					else
					{
						printf("Reading integer imas global ctx %d OK! -> %d\n", iGetOpCtx, iTemp);
					}
					
					alStatus = ual_read_data(iGetOpCtx, szFieldPath2, szTimeBasePath, (void**)&szTemp, CHAR_DATA, 1, &retSize[0]);
					if (alStatus.code != 0)
					{
						printf("Error reading string imas global ctx %d: ual_read_data = %s\n", iGetOpCtx, alStatus.message);
						iRet = alStatus.code;
					}
					else
					{
						printf("Reading string imas global ctx %d OK! -> %.*s\n", iGetOpCtx, retSize[0], szTemp);
						free(szTemp);
					}
					
					alStatus = ual_end_action(iGetOpCtx);
				}
			}
			
			alStatus = ual_close_pulse(iPulseCtx, iCloseAction, "");
			
		}
	}
	
	return iRet;
}
