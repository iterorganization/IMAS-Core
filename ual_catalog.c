#include <stdio.h>
#include <string.h>
#ifdef USE_ITM_CATALOG
#include "mysql/mysql.h"
#include "common/itm_common.h"
#include "simulations/entry_table.h"
#include "simulations/cpo_finder_table.h"
#include "simulations/entry_summary_table.h"
#include "simulations/children_table.h"
#include "simulations/database_api.h"

//UAL Interface to simulations catalog
static MYSQL *initCatalog()
{
    static int connected;
    static MYSQL *mysql;

    if(connected)
        return mysql;

    mysql=mysql_init(NULL);
    if(!mysql)
    {
    	printf("Error initializing mysql\n");
	return 0;
    }
    //NOTE: DB Server, user and password are cabled in the code
    if(!mysql_real_connect(mysql,"itmmysql1","ual_user",0,"itm_catalog",0,NULL,0))
    
    {
        fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(mysql));
    }
    else    
        printf("Connected!\n");
 
    printf("MySQL Server Version is %s\n",mysql_get_server_info(mysql));
    connected = 1;
    return mysql;
}

int ual_get_entry_id(char *user, char *machine, int shot, int run, 
	long *retId, char *retDataV)
{
    ITM_UBIGINT id;
    int status;
    MYSQL *mysql = initCatalog();
    if(!mysql)
        return -1;
    status = itm_get_entry_id(mysql, machine, shot, run, user,  &id, retDataV);
   if(!status)
        *retId = id;
    return status;
}


int ual_create_new_run(char *user, char *machine, int shot, char *dataV, int *retRun)
{
    int status;
    unsigned ITM_INT run;
    MYSQL *mysql = initCatalog();
    if(!mysql)
        return -1;
    status = itm_find_free_run_number( mysql, machine, shot, user, &run);
    if(!status)
    {
    	*retRun = run;
        status = itm_insert_entry(mysql, machine, shot, run, user, dataV);
    }
    return status;
}

int ual_create_new_run_parent(char *user, char *machine, int shot, char *dataV, int *retRun, 
	char *parentUser, char *parentMachine, int parentShot, int parentRun)
{
    int status;
    unsigned ITM_INT run;
    ITM_UBIGINT id, parentId;
    MYSQL *mysql = initCatalog();
    char retDataV[512];
    if(!mysql)
        return -1;
    status = itm_find_free_run_number( mysql, machine, shot, user, &run);
    if(!status)
    {
    	*retRun = run;
        status = itm_insert_entry(mysql, machine, shot, run, user, dataV);
    }
    if(!status)
    	status = itm_get_entry_id(mysql, machine, shot, run, user,  &id, retDataV);
    if(!status)
        status = itm_get_entry_id(mysql, parentMachine, parentShot, parentRun, parentUser,  &parentId, retDataV);
     if(!status)
    	status = itm_insert_children_info_by_value(mysql, id, parentId);
    return status;
}

int ual_create_specified_run(char *user, char *machine, int shot, int run, char *dataV)
// creates or overwrite a catalog entry with a specified run number
{
	int status;
	ITM_UBIGINT id;
	MYSQL *mysql = initCatalog();	// open database
	if(!mysql)
		return -1;
	// check if this run already exists
	status = itm_get_entry_id(mysql, machine, shot, run, user,  &id, dataV);
	// if it exists then delete it
	if(!status)
	{
		status = itm_delete_entry(mysql,id);
		if (status) return status;
	}
	// write the new entry
	status = itm_insert_entry(mysql, machine, shot, run, user, dataV);
	return status;
}

int ual_create_specified_run_parent(char *user, char *machine, int shot, int run, char *dataV,
	char *parentUser, char *parentMachine, int parentShot, int parentRun)
// creates or overwrite a catalog entry with a specified run number (with parent)
{
	int status;
	ITM_UBIGINT id, parentId;
	MYSQL *mysql = initCatalog();	// open database
	char retDataV[512];
	if(!mysql)
		return -1;
	// check if this run already exists
	status = itm_get_entry_id(mysql, machine, shot, run, user,  &id, dataV);
	// if it exists then delete it
	if(!status)
	{
		status = itm_delete_entry(mysql,id);
	}
	// write the new entry
	if(!status)
		status = itm_insert_entry(mysql, machine, shot, run, user, dataV);
	// get id of this new entry
	if(!status)
		status = itm_get_entry_id(mysql, machine, shot, run, user,  &id, retDataV);
	// get id of parent entry
	if(!status)
		status = itm_get_entry_id(mysql, parentMachine, parentShot, parentRun, parentUser,  &parentId, retDataV);
	// link parent and child
	if(!status)
		status = itm_insert_children_info_by_value(mysql, id, parentId);
	return status;
}

int ual_put_cpo(char *user, char *machine, int shot, int run, char *cpoName, int cpoOccurrence, int isRef,
   char *refUser, char *refMachine, int refShot, int refRun, int refOccurrence)
{
    ITM_UBIGINT id;
    int status;
    char retDataV[512];
    int runId;
    int i;
    char *pulseDir;
    char envVar[64];
    char *redDir;
    MYSQL *mysql = initCatalog();
    if(!mysql)
        return -1;
    char runStr[64], shotStr[64], occurrenceStr[64];

    status = itm_get_entry_id(mysql, machine, shot, run, user,  &id, retDataV);
    if(status)
	return status;
	
    if(isRef == 1) //explicit storage
    {
    	runId = run/10000;
	sprintf(envVar, "MDSPLUS_TREE_BASE_%d", runId);
	pulseDir = getenv(envVar);
	if(!pulseDir)
	  pulseDir = "";
	redDir = malloc(strlen(pulseDir) + 1);
	strcpy(redDir, pulseDir);
	for(i = 0; i < strlen(pulseDir); i++)
	{
	    if(redDir[i] == ';')
	    { 
	        redDir[i] = 0;
		break;
	    }
	}
	status = itm_insert_cpo_finder_by_value(mysql, id, cpoName, "isref", "1",cpoOccurrence);
	if(!status) 
	    status = itm_insert_cpo_finder_by_value(mysql, id, cpoName, "putinfo/putlocation", redDir,cpoOccurrence);
	if(!status) 
	    status = itm_insert_cpo_finder_by_value(mysql, id, cpoName, "putinfo/putmethod", "mds+",cpoOccurrence);
    }
    else //isRef ==2, reference to existing storage  
    {
        sprintf(runStr, "%d", refRun);
        sprintf(shotStr, "%d", refShot);
        sprintf(occurrenceStr, "%d", refOccurrence);
	status = itm_insert_cpo_finder_by_value(mysql, id, cpoName, "isref", "2",cpoOccurrence);
	if(!status ) 
	    status = itm_insert_cpo_finder_by_value(mysql, id, cpoName, "whatref/user", refUser,cpoOccurrence);
	if(!status) 
	    status = itm_insert_cpo_finder_by_value(mysql, id, cpoName, "whatref/machine", refMachine, cpoOccurrence);
	if(!status) 
	    status = itm_insert_cpo_finder_by_value(mysql, id, cpoName, "whatref/shot", shotStr, cpoOccurrence);
	if(!status) 
	    status = itm_insert_cpo_finder_by_value(mysql, id, cpoName, "whatref/run", runStr, cpoOccurrence);
	if(!status) 
	    status = itm_insert_cpo_finder_by_value(mysql, id, cpoName, "whatref/occurrence", occurrenceStr, cpoOccurrence);
    }
    return status;
}                 

int ual_get_cpo_ref(char *user, char *machine, int shot, int run, char *cpoName, int cpoOccurrence, 
  int *isRef, char *refUser, char *refMachine, int *refShot, int *refRun, int *refOccurrence)
{
    char retDataV[512];
    ITM_UBIGINT id;
    int status;
    MYSQL *mysql = initCatalog();
    if(!mysql)
        return -1;
    char runStr[64], shotStr[64], occurrenceStr[64];
    char fieldVal[512];

    status = itm_get_entry_id(mysql, machine, shot, run, user,  &id, retDataV);
    if(status)
	return status;
     
    status = itm_get_cpo_finder_field_value( mysql, id, cpoName, "isref", fieldVal, cpoOccurrence);
    if(status)
      return status;
    *isRef = atoi(fieldVal);
    if(*isRef == 1) //Not a reference 
    {
        strcpy(refUser, user);
	strcpy(refMachine, machine);
	*refShot = shot;
	*refRun = run;
	*refOccurrence = cpoOccurrence;
    }
    else //*isRef == 2, reference to another CPO
    {
         status = itm_get_cpo_finder_field_value( mysql, id, cpoName, "whatref/user", fieldVal, cpoOccurrence);
	 if(!status)
	     strcpy(refUser, fieldVal);
         status = itm_get_cpo_finder_field_value( mysql, id, cpoName, "whatref/machine", fieldVal, cpoOccurrence);
	 if(!status)
	     strcpy(refMachine, fieldVal);
         status = itm_get_cpo_finder_field_value( mysql, id, cpoName, "whatref/shot", fieldVal, cpoOccurrence);
	 if(!status)
	     *refShot = atoi(fieldVal);
         status = itm_get_cpo_finder_field_value( mysql, id, cpoName, "whatref/run", fieldVal, cpoOccurrence);
	 if(!status)
	     *refRun = atoi(fieldVal);
         status = itm_get_cpo_finder_field_value( mysql, id, cpoName, "whatref/occurrence", fieldVal, cpoOccurrence);
	 if(!status)
	     *refOccurrence = atoi(fieldVal);
     }
     return status;
 }

#else
int ual_get_entry_id(char *user, char *machine, int shot, int run, 
	long *retId, char *retDataV)
{
    return 0;
}

int ual_create_new_run(char *user, char *machine, int shot, char *dataV, int *retRun)
{
    return 0;
}

int ual_create_new_run_parent(char *user, char *machine, int shot, char *dataV, int *retRun, 
	char *parentUser, char *parentMachine, int parentShot, int parentRun)
{
    return 0;
}

int ual_create_specified_run(char *user, char *machine, int shot, int run, char *dataV)
{
    return 0;
}

int ual_create_specified_run_parent(char *user, char *machine, int shot, int run, char *dataV,
	char *parentUser, char *parentMachine, int parentShot, int parentRun)
{
    return 0;
}

int ual_put_cpo(char *user, char *machine, int shot, int run, char *cpoName, int cpoOccurrence, int isRef,
   char *refUser, char *refMachine, int refShot, int refRun, int refOccurrence)
{
    return 0;
}


int ual_get_cpo_ref(char *user, char *machine, int shot, int run, char *cpoName, int cpoOccurrence, 
  int *isRef, char *refUser, char *refMachine, int *refShot, int *refRun, int *refOccurrence)
{
    return 0;
}


#endif

