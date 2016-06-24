
// IDAM interface library
// Wrappers to replace ual_low_level_idam with redirection to IDAM (plugin: imas)
// No HDF5 API calls

/*
Observations:

expIdx or idx	- reference to the open data file: index into an array of open files - idamFiles[idx]  
cpoPath	- the root group where the CPO/IDS is written
path	- the path relative to the root (cpoPath) where the data are written (must include the variable name!)
data	- the data to be written

HDF5_BASE - an environment variable defining the root directory of the HDF5 data archive
HDF5_MODEL_BASE - an environment variable defining a model HDF5 file

idamEuitmCreate - copies the HDF5 model file to a new file and opens it
idamEuitmOpen - opens a file

Concerns:

What does the HDF5 model file contain. Probably all group hierarchies for each IDS.
Is this a further limitation to versioning structures
Why should the user be concerned about what type of file to open!
There are no model files in the git repo

idamEuitmCreate may create a new file and overwrite a previous file!
TODO: Check for existing files

idamEuitmCreate calls the system() function
This is a security risk
Additional system commands may be included in the passed name strings
There are 2048 bytes to fill with code!
TODO: Need to use IDAM function that check filenames from getHdf5FileName and getHdf5ModelName are standards compliant
      use IsLegalFilePath(char *str) from IDAM TrimString utilities
      Done!

There is no mechanism for opening a previously existing file that is already open!
TODO: Add name details to the file handle log and check for open files before opening a new file
      Use the IDAM managePluginFiles utilities
      Done!
      
TODO: The source argument for idamGetAPI is empty (default server) - this should be a targeted source!  

**** THERE ARE NO UNIT TESTS FOR THE LOW LEVEL ROUTINES!!!!
**** All putSlice functions have a 'double time' argument that is not used! 

For Scalar putDataSlice, rank=0, shape=NULL
For array 		 rank=n, shape=same shape as the original
   

Planned actions:

Change over file management to the IDAM plugin system
Add additional types
Review how strings are handled
Review default values for missing data: NaN rather than the arbitrary extreme values chosen in IMAS, e.g. EMPTY_INT 
Locate all Object specific code to the IDAM plugin
Investigate the object system - is this just a local cache?


Change History

21Apr2015 dgmuir	Original version based on ual_low_level_idam from ITER git master repo
			All original source code reused where possible
			HDF5 specific code relocated to an IDAM put plugin
			Minimal code changes made: No renaming; No change to arguments
			
			Added initHdf5File to initialise array 
			Modified idamEuitmCreate to call initHdf5File on startup 
			Modified idamEuitmOpen to call initHdf5File on startup
			Added idamIMASCreate to create a new file without a model
			
			Commented out TranslateLogical as external dependence
			Modified getHdf5FileName to use getenv instead of TranslateLogical 
			Modified getHdf5ModelName to use getenv instead of TranslateLogical 
			
			bug fix: function idamGetDimension has no type - adding type int 
			
			Pointer errmsg is undefined - create locate string and assign errmsg to it.
			declared extern - changed to static
			
			Added IDAM error log to pass back errors and messages
			
			Added IDAM state variable: static unsigned short isCloseRegistered 
			
			Added function static int findHdf5Idx(int idam_id)
			
			Added functionality to create missing groups to putData
			
			Added function findIMASType and an UNKNOWN_TYPE
			
			Added function createGroup with #define MAX_TOKENS
			
			Added function findIMASIDAMType
			
			putData renamed imas_putData 
			New function putData created that calls IDAM to replace original
			
			putDataSlice renamed imas_putDataSlice 
			New function putDataSlice created that calls IDAM to replace original
			
			Added functionality to create missing groups to imas_putDataSlice, imas_replaceLastDataSlice

			getData renamed imas_getData 
			New function getData created that calls IDAM to replace original

			getDataSlices renamed imas_getDataSlices 
			New function getDataSlices created that calls IDAM to replace original

			idamGetDimension renamed imas_idamGetDimension 
			New function idamGetDimension created that calls IDAM to replace original
			
			idamDeleteData renamed imas_idamDeleteData
			New function idamDeleteData created that calls IDAM to replace original

			new function idamimasOpen wrapper to idamEuitmOpen
			idamEuitmOpen renamed imas_idamEuitmOpen
			New function idamEuitmOpen created that calls IDAM to replace original
			
			new function idamimasCreate wrapper to idamEuitmCreate
			idamEuitmCreate renamed imas_idamEuitmCreate
			New function idamEuitmCreate created that calls IDAM to replace original
			IsLegalFilePath used to test passed file name before system level api called
			
			idamIMASCreate renamed imas_idamIMASCreate
			New function idamIMASCreate created that calls IDAM to replace original


24Aug2015 DGMuir	Add calls to imas_getKeyword to enable the client application to use mdsplus on the server

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ual_low_level.h"

#include "idamclientpublic.h"
#include "ual_low_level_idam.h"
#include "ual_low_level_idam_private.h"

#define PUT_OPERATION               0
#define PUTSLICE_OPERATION          1
#define REPLACELASTSLICE_OPERATION  2

#define ERROR_RETURN_VALUE		1
#define OK_RETURN_VALUE			0
 
#define MAX_STRINGS 20000

#define STRING          1
#define STRING_VECTOR   5

#define UNKNOWN_TYPE 0

#define INT 2
#define FLOAT 3 
#define DOUBLE 4
#define DIMENSION -1    // if we need to read the dimensions only

static char ErrMsg[MAX_STRINGS];
static char *errmsg = &ErrMsg[0];

//typedef struct obj_t {
//    hid_t handle;   // HDF5 handle of the group corresponding to the object
//    int dim;        // number of elements contained in the object (not used for put)
//    int timeIdx;    // for timed objects, index of the time to be read (not used for put)
//    struct obj_t *nextObj; // a list of other objects contained inside this object (for cleaning purpose)
//} obj_t;

static char imasKeyword[128];
static char * imas_getKeyword()
{
    return imasKeyword;
}

// Low level functions that originate in low_level_mdsplus
// Required by the High Level APIs
// call IDAM IMAS mdsplus plugin

int idamimasOpenEnv(char* name, int shot, int run, int* retIdx, char* user, char* tokamak, char* version)
{
    return 0;
}

int idamimasCreateEnv(char* name, int shot, int run, int refShot, int refRun, int* retIdx, char* user, char* tokamak, char* version)
{
    return 0;
}

void reportInfo(char *str1, char *str2)
{
#ifdef DEBUG_UAL
    FILE *f = fopen("ual.txt", "a");
    fprintf(f, str1, str2);
    fclose(f);
#endif
}

int mdsCopyCpo(int fromIdx, int toIdx, char *inputcpoName, int fromCpoOccur, int toCpoOccur)
{
    return 0;
}

static void idam_void(char *directive){   
   int handle = idamGetAPI(directive, "");   
   int err = 0;   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return;
   }   
   idamFree(handle);    
}

static int idam_int(char *directive){   
   int handle = idamGetAPI(directive, "");   
   int err = 0;   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   int rc = ((int *)getIdamData(handle))[0];   
   idamFree(handle);
   return rc;    
}

static char *idam_char(char *directive){   
   int handle = idamGetAPI(directive, "");   
   int err = 0;   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return NULL;
   }
   char *rc = (char *)getIdamData(handle);   
   idamFree(handle);
   return rc;    
}

void imas_flush_mem_cache(int idx){   
   int lstr = 256;
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::cache(idx=%d, /imas_mds, /flush)", idx);      
   idam_void(directive);   
   if(directive) free(directive);       
}

void imas_discard_cpo_mem_cache(int idx, char *cpoPath){
   int lstr = 256 + strlen(cpoPath);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::cache(idx=%d, cpoPath='%s', /imas_mds, /discard)", idx, cpoPath);      
   idam_void(directive);   
   if(directive) free(directive);           
}

int imas_get_cache_level(int idx){
   int lstr = 256;
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::cache(idx=%d, /imas_mds, /getLevel)", idx);      
   int rc = idam_int(directive);   
   if(directive) free(directive);
   return rc;           
}

void imas_flush_cpo_mem_cache(int idx, char *cpoPath){
   int lstr = 256 + strlen(cpoPath);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::cache(idx=%d, cpoPath='%s', /imas_mds, /flushCPO)", idx, cpoPath);      
   idam_void(directive);   
   if(directive) free(directive);           
}

void imas_set_cache_level(int idx, int level){
   int lstr = 256;
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::cache(idx=%d, cacheLevel=%d, /imas_mds, /setLevel)", idx, level);      
   idam_void(directive);   
   if(directive) free(directive);           
}

void imas_disable_mem_cache(int idx){
   int lstr = 256;
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::cache(idx=%d, /imas_mds, /disable)", idx);      
   idam_void(directive);   
   if(directive) free(directive);           
}

void imas_discard_mem_cache(int idx){
   int lstr = 256;
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::cache(idx=%d, /imas_mds,/discard)", idx);      
   idam_void(directive);   
   if(directive) free(directive);           
}

void imas_enable_mem_cache(int idx){
   int lstr = 256;
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::cache(idx=%d, /imas_mds, /enable)", idx);      
   idam_void(directive);   
   if(directive) free(directive);           
}

char* spawnCommand(char *command, char *ipAddress){
   int lstr = 256 + strlen(command) + strlen(ipAddress);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::spawnCommand(command='%s', ipAddress='%s', /imas_mds)", command, ipAddress);      
   char *rc = idam_char(directive);   
   if(directive) free(directive);
   return rc;           
}

int getUniqueRun(int shot){
   int lstr = 256;
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::getUniqueRun(shot=%d, /imas_mds)", shot);      
   int rc = idam_int(directive);   
   if(directive) free(directive);
   return rc;           
}

void setTimeBasePath(char *timeBasePath){
   static char TimeBasePath[256];
   if(timeBasePath == NULL || !strcmp(timeBasePath, TimeBasePath)) return;
   //if(timeBasePath == NULL || timeBasePath[0] == '\0' || !strcmp(timeBasePath, TimeBasePath)) return;
   int lstr = 512 + strlen(timeBasePath);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   sprintf(directive, "imas::setTimeBasePath(path='%s', /imas_mds)", timeBasePath);   
   idam_void(directive);   
   if(directive) free(directive);
   if(strlen(timeBasePath) < 256) strcpy(TimeBasePath, timeBasePath);           
}

// dgm  Convert name to IMAS type

int findIMASType(char *typeName){
   //if(!strcasecmp(typeName, "byte"))     return TYPE_CHAR;
   //if(!strcasecmp(typeName, "char"))     return TYPE_CHAR;
   //if(!strcasecmp(typeName, "short"))    return TYPE_SHORT;
   if(!strcasecmp(typeName, "int"))      return INT;
   //if(!strcasecmp(typeName, "int64"))    return TYPE_LONG64;
   if(!strcasecmp(typeName, "float"))    return FLOAT;
   if(!strcasecmp(typeName, "double"))   return DOUBLE;
   //if(!strcasecmp(typeName, "ubyte"))    return TYPE_UNSIGNED_CHAR;
   //if(!strcasecmp(typeName, "ushort"))   return TYPE_UNSIGNED_SHORT;
   //if(!strcasecmp(typeName, "uint"))     return TYPE_UNSIGNED_INT;
   //if(!strcasecmp(typeName, "uint64"))   return TYPE_UNSIGNED_LONG64;
   //if(!strcasecmp(typeName, "text"))     return TYPE_STRING;
   if(!strcasecmp(typeName, "string"))   return STRING;
   //if(!strcasecmp(typeName, "vlen"))     return TYPE_VLEN;
   //if(!strcasecmp(typeName, "compound")) return TYPE_COMPOUND;
   //if(!strcasecmp(typeName, "opaque"))   return TYPE_OPAQUE;
   //if(!strcasecmp(typeName, "enum"))     return TYPE_ENUM;
   return(UNKNOWN_TYPE);     
}

// dgm  Convert IMAS type to IDAM type

int findIMASIDAMType(int type){
   switch(type){
      case INT:		return TYPE_INT;
      case FLOAT:	return TYPE_FLOAT;
      case DOUBLE:	return TYPE_DOUBLE;
      case STRING:	return TYPE_STRING;
      case STRING_VECTOR: return TYPE_STRING;
   }
   return TYPE_UNKNOWN;     
}

char *convertIdam2StringType(int type){
   switch(type){
      case TYPE_CHAR:	return("char"); 
      case TYPE_SHORT:	return("short");
      case TYPE_INT:	return("int");
      case TYPE_LONG64:	return("int64");
      case TYPE_FLOAT:	return("float");
      case TYPE_DOUBLE:	return("double");
      case TYPE_UNSIGNED_CHAR:	return("ubyte");		
      case TYPE_UNSIGNED_SHORT:	return("ushort");
      case TYPE_UNSIGNED_INT:	return("uint");
      case TYPE_UNSIGNED_LONG64:return("ulong64");
      //case TYPE_VLEN:		return(NC_VLEN);
      //case TYPE_COMPOUND:	return(NC_COMPOUND);
      //case TYPE_OPAQUE:		return(NC_OPAQUE);
      //case TYPE_ENUM:		return(NC_ENUM);
      case TYPE_STRING:		return("string"); 
      default:return "unknown";          
   }
   return "unknown";
}

// dgm Replacement for Original IMAS HDF5 idamEuitmCreate (renamed imas_idamEuitmCreate) that calls IDAM

int idamEuitmCreate(char *name, int shot, int run, int refShot, int refRun, int *retIdx){ 
   
   int lstr = DIRECTIVELENGTH + strlen(name);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::create(file='%s', shot=%d, run=%d, refShot=%d, refRun=%d, /CreateFromModel, %s)", name, shot, run, refShot, refRun, keyword);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error Creating File: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) *retIdx = data[0];
   
// Free heap

   idamFree(handle); 
   
   return OK_RETURN_VALUE;
}

// dgm added function idamimasCreate wrapper to original idamEuitmCreate

int idamimasCreate(char *name, int shot, int run, int refShot, int refRun, int *retIdx){ 
   return idamEuitmCreate(name, shot, run, refShot, refRun, retIdx);
}

// dgm Replacement for Original IMAS HDF5 idamIMASCreate (renamed imas_idamIMASCreate) that calls IDAM

int idamIMASCreate(char *name, int shot, int run, int refShot, int refRun, int *retIdx){
   
   int lstr = DIRECTIVELENGTH + strlen(name);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::create(file='%s', shot=%d, run=%d, refShot=%d, refRun=%d, %s)", name, shot, run, refShot, refRun, keyword);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error Creating File: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) *retIdx = data[0];
   
// Free heap

   idamFree(handle); 
   
   return OK_RETURN_VALUE;
}

// dgm Replacement for Original IMAS HDF5 idamEuitmOpen (renamed imas_idamEuitmOpen) that calls IDAM

int idamEuitmOpen(char *name, int shot, int run, int *retIdx){
   
   int lstr = DIRECTIVELENGTH + strlen(name);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::open(file='%s', shot=%d, run=%d, %s)", name, shot, run, keyword);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error Opening File: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) *retIdx = data[0];
   
// Free heap

   idamFree(handle); 
   
   return OK_RETURN_VALUE;
}

// dgm added function idamimasOpen wrapper to original idamEuitmOpen

int idamimasOpen(char *name, int shot, int run, int *retIdx){ 
   return idamEuitmOpen(name, shot, run, retIdx); 
}

// dgm Replacement for Original IMAS HDF5 idamEuitmClose (renamed imas_idamEuitmClose) that calls IDAM

int idamEuitmClose(int idx, char *name, int shot, int run){   
   int lstr = DIRECTIVELENGTH + strlen(name);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::close(idx=%d, file='%s', shot=%d, run=%d, %s)", idx, name, shot, run, keyword);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error Closing File: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL && data[0] != OK_RETURN_VALUE){ 
      sprintf(errmsg, "Error Closing File: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
// Free heap

   idamFree(handle); 
   
   return OK_RETURN_VALUE;
}

// dgm added function idamimasClose wrapper to original idamimasClose

int idamimasClose(int idx, char *name, int shot, int run){
   return idamEuitmClose(idx, name, shot, run); 
}

// dgm Replacement for Original IMAS HDF5 idamDeleteData (renamed imas_idamDeleteData) that calls IDAM 

int idamDeleteData(int idx, char *cpoPath, char *path){

#ifdef IDAM_NO_HDF5DELETEDATA
   return OK_RETURN_VALUE;		// Disable this function to monitor Performance
#endif

// Pass the Data

   int lstr = DIRECTIVELENGTH + strlen(cpoPath) + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::delete(idx=%d, group='%s', variable='%s', %s)", idx, cpoPath, path, keyword);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0 || getIdamData(handle) == NULL){      
      sprintf(errmsg, "Error DELETing Data: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
      
   idamFree(handle);   
   
   return OK_RETURN_VALUE;
}

// dgm Replacement for Original IMAS HDF5 idamGetDimension (renamed imas_idamGetDimension) that calls IDAM 

int idamGetDimension(int idx, char *cpoPath, char *path, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7){

// Pass the Data

   int i;
   
   int lstr = DIRECTIVELENGTH + strlen(cpoPath) + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::get(idx=%d, group='%s', variable='%s', /getDimension, %s)", idx, cpoPath, path, keyword);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0 || getIdamData(handle) == NULL){      
      sprintf(errmsg, "Error GETing Data: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   *numDims = getIdamDataNum(handle);
   
   int shape[7];
   
   for(i=0;i< *numDims;i++){
       switch(i){
         case 0: 
	    *dim1 = ((int *)getIdamData(handle))[i];
	    break;
         case 1: 
	    *dim2 = ((int *)getIdamData(handle))[i];
	    break;
         case 2: 
	    *dim3 = ((int *)getIdamData(handle))[i];
	    break;
         case 3: 
	    *dim4 = ((int *)getIdamData(handle))[i];
	    break;
         case 4: 
	    *dim5 = ((int *)getIdamData(handle))[i];
	    break;
         case 5: 
	    *dim6 = ((int *)getIdamData(handle))[i];
	    break;
         case 6: 
	    *dim7 = ((int *)getIdamData(handle))[i];
	    break;
	 default: {
	    *numDims = 8;
	    idamFree(handle); 
	    return ERROR_RETURN_VALUE;
	 }  
      }
   }
   
   idamFree(handle);      
   return OK_RETURN_VALUE;
/*   
   *numDims = getIdamRank(handle);
   
   for(i=0;i< *numDims;i++){
      switch(i){
         case 0: 
	    *dim1 = getIdamDimNum(handle, i);
	    break;
         case 1: 
	    *dim2 = getIdamDimNum(handle, i);
	    break;
         case 2: 
	    *dim3 = getIdamDimNum(handle, i);
	    break;
         case 3: 
	    *dim4 = getIdamDimNum(handle, i);
	    break;
         case 4: 
	    *dim5 = getIdamDimNum(handle, i);
	    break;
         case 5: 
	    *dim6 = getIdamDimNum(handle, i);
	    break;
         case 6: 
	    *dim7 = getIdamDimNum(handle, i);
	    break;
	 default: {
	    *numDims = 7;
	    idamFree(handle); 
	    return OK_RETURN_VALUE;
	 }   
      }
   }
*/      
}

// dgm Replacement for Original IMAS HDF5 putData (renamed imas_putData) that calls IDAM 
static int putData(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, int isTimed, void *data){

// Create PUTDATA variables   
    
   PUTDATA_BLOCK putData;
   initIdamPutDataBlock(&putData);
   
// Pass the Data

   int i;
   putData.data_type = findIMASIDAMType(type);
   if(dims != NULL) 
      putData.count = dims[0];
   else 
      putData.count = 1; 
   putData.rank  = nDims;
   putData.shape = dims;
   for(i=1;i<putData.rank;i++)putData.count = putData.count * dims[i]; 
   putData.data  = data;
   
   int lstr = DIRECTIVELENGTH + strlen(cpoPath) + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::put(idx=%d, group='%s', variable='%s', isTimed=%d, %s)", idx, cpoPath, path, isTimed, keyword);
   
   int handle = idamPutAPI(directive, &putData);   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error PUTing Data: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   int *rc = (int *)getIdamData(handle);
   
   if(rc == NULL || rc[0] != OK_RETURN_VALUE){
      sprintf(errmsg, "Error PUTing Data: No or Incorrrect status return code");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   idamFree(handle);
   
   return OK_RETURN_VALUE;
}

// dgm Varient of putData for use only with putDataSlice and replaceLastDataSlice
static int putDataX(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, int dataOperation, void *data, double time){

// Create PUTDATA variables   
    
   PUTDATA_BLOCK putData;
   initIdamPutDataBlock(&putData);
   
// Pass the Data

   int i, handle;
   putData.data_type = findIMASIDAMType(type);
   if(dims != NULL) 
      putData.count = dims[0];
   else 
      putData.count = 1; 
   putData.rank  = nDims;
   putData.shape = dims;
   for(i=1;i<putData.rank;i++)putData.count = putData.count * dims[i]; 
   putData.data  = data;
   
   int lstr = DIRECTIVELENGTH + strlen(cpoPath) + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   if(dataOperation == PUTSLICE_OPERATION){
      sprintf(directive, "imas::put(idx=%d, group='%s', variable='%s', /putSlice, isTimed=0, %s)", idx, cpoPath, path, keyword);

// Two PUTDATABLOCKS are passed: One for the Data and one for the Time. 
// To pass the time accurately a PUTDATA block must be used - name-value pairs are insufficiently accurate as ASCII

      PUTDATA_BLOCK timeData;
      initIdamPutDataBlock(&timeData);
      PUTDATA_BLOCK_LIST putDataBlockList;
      initIdamPutDataBlockList(&putDataBlockList);   
      
      timeData.data_type = TYPE_DOUBLE; 
      timeData.count = 1; 
      timeData.rank  = 0;
      timeData.shape = NULL;
      
      timeData.blockName = (char *)malloc(5*sizeof(char));
      timeData.blockNameLength = 5;
      strcpy(timeData.blockName, "time");

      putData.blockName = (char *)malloc(5*sizeof(char));
      putData.blockNameLength = 5;
      strcpy(putData.blockName, "data");
      
      double *data = (double *)malloc(sizeof(double));
      data[0] = time;
      timeData.data = (void *)data;

      addIdamPutDataBlockList(&putData,  &putDataBlockList);
      addIdamPutDataBlockList(&timeData, &putDataBlockList);

      handle = idamPutListAPI(directive, &putDataBlockList);
      
      freeIdamClientPutDataBlockList(&putDataBlockList);
      free(data);
      free(timeData.blockName);

   } else 
   if(dataOperation == REPLACELASTSLICE_OPERATION){
// The time passed is set to Zero as it's the Last Time Slice that is to be replaced. Only the data is passed by a PUTDATABLOCK. 
      sprintf(directive, "imas::put(idx=%d, group='%s', variable='%s', /replaceLastSlice, isTimed=0, %s)", idx, cpoPath, path, keyword);   
      handle = idamPutAPI(directive, &putData);   
   }
   
   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error PUTing Data: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   int *rc = (int *)getIdamData(handle);
   
   if(rc == NULL || rc[0] != OK_RETURN_VALUE){
      sprintf(errmsg, "Error PUTing Data: No or Incorrrect status return code");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   idamFree(handle);
   
   return OK_RETURN_VALUE;
}

static int putDataSlice(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, void *data, double time){
   return putDataX(idx, cpoPath, path, type, nDims, dims, PUTSLICE_OPERATION, data, time);
}

static int replaceLastDataSlice(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, void *data){
   return putDataX(idx, cpoPath, path, type, nDims, dims, REPLACELASTSLICE_OPERATION, data, 0.0);
}

// dgm Replacement for Original IMAS HDF5 getData (renamed imas_getData) that calls IDAM 

static int getData(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, char  **data)
{
   
// Pass the Data

   int i;
   char *data_type = convertIdam2StringType(findIMASIDAMType(type));
   int rank   = nDims;
   int *shape = dims;
   
   int lstr = DIRECTIVELENGTH + strlen(cpoPath) + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   int source = ual_get_shot(idx);
   
   sprintf(directive, "imas::get(idx=%d, group='%s', variable='%s', type=%s, rank=%d, shot=%d, %s)", idx, cpoPath, path, data_type, rank, source, keyword);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error GETing Data: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   if(rank != getIdamRank(handle)){
      sprintf(errmsg, "Error GETing Data: Inconsistent Data Rank");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   if(shape != NULL){
      shape[0] = 1;	// Scalar if rank == 0
      for(i=0;i<rank;i++) shape[i] = getIdamDimNum(handle, i);
   }
   
   if(data != NULL) *data = getIdamData(handle);
   if (*data == NULL && type == STRING) {
      *data = malloc(sizeof(char));
      (*data)[0] = '\0';
   }
   
// Detach heap from the IDAM structure then free the IDAM structure (avoid data leaks within IDAM)

   getIdamDataBlock(handle)->data = NULL;      
   idamFree(handle);	// Application is responsible for freeing data  
   
   return OK_RETURN_VALUE;
}

static int sliceIdx1, sliceIdx2;
static double sliceTime1, sliceTime2;

/*
static void setSliceIdx(int index1, int index2){
   sliceIdx1 = index1;   
   sliceIdx2 = index2;   
}
static void setSliceTime(double time1, double time2){
   sliceTime1 = time1;   
   sliceTime2 = time2;   
}
*/
static int getSliceIdxs(int expIdx, char *cpoPath, double time)
{
    double *times;
    int status, dim, i;
    
    status = idamGetVect1DDouble(expIdx, cpoPath, "time", &times, &dim);
    if(status) return status;
    if(dim == 1 || time <= times[0])
    {
        sliceIdx1 = 0;
        sliceTime1 = times[0];
        sliceIdx2 = -1;
    }
    else if(time >= times[dim - 1])
    {
        sliceIdx1 = dim-1;
        sliceTime1 = times[dim-1];
        sliceIdx2 = -1;
    }
    else //dim > 1
    {
        for(i = 0; i < dim-1; i++)
            if(times[i] <= time && times[i+1] >= time)
                break;
        sliceIdx1 = i;
        sliceTime1 = times[i];
        sliceIdx2 = i+1;
        sliceTime2 = times[i+1];
    }
    free(times);
    return OK_RETURN_VALUE;
}

// dgm Replacement for Original IMAS HDF5 getDataSLices (renamed imas_getDataSlices) that calls IDAM
// dgm Added additional arguments to push slicing concerns to the server
// dgm Correctd a bug - sliceIdx and sliceTime not defined. Function getSliceIdxs never called
// dgm dataIdx argument now redundant so removed
// dgm numSlices argument also redundant so removed
// dgm All times passed to server in a PUTDATA BLOCK for precision

//static int getDataSlices(int idx, char *cpoPath, char *path, int type, int nDims, int *dims, int dataIdx, int numSlices, char **data){
  static int getDataSlices(int idx, char *cpoPath, char *path, int type, int nDims, int *dims,                             char **data, double time, double *retTime, int interpolMode){

// Locate the times slices in the time vector

   getSliceIdxs(idx, cpoPath, time);	// *** Very inefficient to do this for every request!

// Pass the Data

   int i;
   char *data_type = convertIdam2StringType(findIMASIDAMType(type));
   int rank   = nDims;
   int *shape = dims;
   
   int lstr = DIRECTIVELENGTH + strlen(cpoPath) + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::get(idx=%d, group='%s', variable='%s', type=%s, rank=%d, index1=%d, index2=%d, interpolMode=%d, /getSlice, %s)", 
           idx, cpoPath, path, data_type, rank, sliceIdx1, sliceIdx2, interpolMode, keyword);   

// Create PUTDATA variables   
    
   PUTDATA_BLOCK putData;
   initIdamPutDataBlock(&putData);
   
// Pass the Data

   putData.data_type = TYPE_DOUBLE;
   putData.count = 3; 
   putData.rank  = 1;
   putData.shape = NULL;
   double *dp    = (double *)malloc(putData.count*sizeof(double));
   putData.data  = (void *)dp;   
   
   dp[0] = time;
   dp[1] = sliceTime1;
   dp[2] = sliceTime2;
   
   int handle = idamPutAPI(directive, &putData);   

   if(directive) free(directive);
   if(dp) free(dp);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error GETing Data: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

// Returned time point (Additional coordinate of length 1 attached)

   *retTime = ((double *)getIdamDimData(handle, getIdamOrder(handle)))[0];      
   
   rank = getIdamRank(handle)-1;	// Drop the additional coordinate returned (retTime)
      
   if(shape != NULL && rank > 0){
      shape[0] = 1;	// Scalar if rank == 0
      for(i=0;i<rank;i++) shape[i] = getIdamDimNum(handle, i);
   }
   
   if(data != NULL) *data = getIdamData(handle);
   
// Detach heap from the IDAM structure then free the IDAM structure (avoid data leaks within IDAM)

   getIdamDataBlock(handle)->data = NULL;      
   idamFree(handle);	// Application is responsible for freeing data  
      
   return OK_RETURN_VALUE;
}

//Low level function prototypes
int idamPutString(int expIdx, char *cpoPath, char *path, char *data, int strlen)
{
// dgm: Move this concern to putData
// dgm: Not done in idamPutStringSlice or idamPutStringInObject!
    //int dims = 1; 
    //dims = (strlen / 132) +1;
    //return putData(expIdx, cpoPath, path, STRING, 1, &dims, 0, data);
    
    int rank=0, dims=strlen;    
    return putData(expIdx, cpoPath, path, STRING, rank, &dims, 0, data);

}

int idamPutInt(int expIdx, char *cpoPath, char *path, int data)
{
    int dims = 1; 
    return putData(expIdx, cpoPath, path, INT, 0, &dims, 0, &data);

}

int idamPutFloat(int expIdx, char *cpoPath, char *path, float data)
{
    int dims = 1; 
    return putData(expIdx, cpoPath, path, FLOAT, 0, &dims, 0, &data);

}

int idamPutDouble(int expIdx, char *cpoPath, char *path, double data)
{
    int dims = 1; 
    return putData(expIdx, cpoPath, path, DOUBLE, 0, &dims, 0, &data);

}

int idamPutVect1DString(int expIdx, char *cpoPath, char *path, char **data, int dim, int isTimed)
{
    return putData(expIdx, cpoPath, path, STRING, 1, &dim, isTimed, data);
}

int idamPutVect1DInt(int expIdx, char *cpoPath, char *path, int *data, int dim, int isTimed)
{
    return putData(expIdx, cpoPath, path, INT, 1, &dim, isTimed, data);
}

int idamPutVect1DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim, int isTimed)
{
    return putData(expIdx, cpoPath, path, FLOAT, 1, &dim, isTimed, data);
}

int idamPutVect1DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim, int isTimed)
{
    return putData(expIdx, cpoPath, path, DOUBLE, 1, &dim, isTimed, data);
}

int idamPutVect2DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int isTimed)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putData(expIdx, cpoPath, path, INT, 2, dims, isTimed, data);
}

int idamPutVect2DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int isTimed)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putData(expIdx, cpoPath, path, FLOAT, 2, dims, isTimed, data);
}

int idamPutVect2DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int isTimed)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putData(expIdx, cpoPath, path, DOUBLE, 2, dims, isTimed, data);
}

int idamPutVect3DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int isTimed)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return putData(expIdx, cpoPath, path, INT, 3, dims, isTimed, data);
}

int idamPutVect3DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int isTimed)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return putData(expIdx, cpoPath, path, FLOAT, 3, dims, isTimed, data);
}

int idamPutVect3DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int isTimed)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    
    return putData(expIdx, cpoPath, path, DOUBLE, 3, dims, isTimed, data);
}

int idamPutVect4DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putData(expIdx, cpoPath, path, INT, 4, dims, isTimed, data);
}

int idamPutVect4DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putData(expIdx, cpoPath, path, FLOAT, 4, dims, isTimed, data);
}

int idamPutVect4DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putData(expIdx, cpoPath, path, DOUBLE, 4, dims, isTimed, data);
}

int idamPutVect5DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putData(expIdx, cpoPath, path, INT, 5, dims, isTimed, data);
}

int idamPutVect5DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putData(expIdx, cpoPath, path, FLOAT, 5, dims, isTimed, data);
}

int idamPutVect5DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putData(expIdx, cpoPath, path, DOUBLE, 5, dims, isTimed, data);
}

int idamPutVect6DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, 
    int dim5, int dim6, int isTimed)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return putData(expIdx, cpoPath, path, INT, 6, dims, isTimed, data);
}

int idamPutVect6DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, 
    int dim5, int dim6, int isTimed)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return putData(expIdx, cpoPath, path, FLOAT, 6, dims, isTimed, data);
}

int idamPutVect6DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, 
    int dim5, int dim6, int isTimed)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return putData(expIdx, cpoPath, path, DOUBLE, 6, dims, isTimed, data);
}

int idamPutVect7DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, 
    int dim5, int dim6, int dim7, int isTimed)
{
    int dims[7];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    dims[6] = dim7;
    return putData(expIdx, cpoPath, path, INT, 7, dims, isTimed, data);
}

int idamPutVect7DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, 
    int dim5, int dim6, int dim7, int isTimed)
{
    int dims[7];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    dims[6] = dim7;
    return putData(expIdx, cpoPath, path, FLOAT, 7, dims, isTimed, data);
}

int idamPutVect7DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, 
    int dim5, int dim6, int dim7, int isTimed)
{
    int dims[7];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    dims[6] = dim7;
    return putData(expIdx, cpoPath, path, DOUBLE, 7, dims, isTimed, data);
}

int idamPutIntSlice(int expIdx, char *cpoPath, char *path, int data, double time)
{
    return putDataSlice(expIdx, cpoPath, path, INT, 0, NULL, &data, time);
}

int idamPutFloatSlice(int expIdx, char *cpoPath, char *path, float data, double time)
{
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 0, NULL, &data, time);
}

int idamPutDoubleSlice(int expIdx, char *cpoPath, char *path, double data, double time)
{
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 0, NULL, &data, time);
}

int idamPutStringSlice(int expIdx, char *cpoPath, char *path, char *data, double time)
{
    return putDataSlice(expIdx, cpoPath, path, STRING, 0, NULL, &data, time);
}

int idamPutVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim, double time)
{
    return putDataSlice(expIdx, cpoPath, path, INT, 1, &dim, data, time);
}

int idamPutVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim, double time)
{
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 1, &dim, data, time);
}

int idamPutVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim, double time)
{
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 1, &dim, data, time);
}

int idamPutVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, double time)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putDataSlice(expIdx, cpoPath, path, INT, 2, dims, data, time);
}

int idamPutVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, double time)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 2, dims, data, time);
}

int idamPutVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, double time)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 2, dims, data, time);
}

int idamPutVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, double time)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return putDataSlice(expIdx, cpoPath, path, INT, 3, dims, data, time);
}

int idamPutVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, double time)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 3, dims, data, time);
}

int idamPutVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, double time)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 3, dims, data, time);
}

int idamPutVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, double time)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putDataSlice(expIdx, cpoPath, path, INT, 4, dims, data, time);
}

int idamPutVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, double time)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 4, dims, data, time);
}

int idamPutVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, double time)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 4, dims, data, time);
}

int idamPutVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, 
    int dim4, int dim5, double time)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putDataSlice(expIdx, cpoPath, path, INT, 5, dims, data, time);
}

int idamPutVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, 
    int dim4, int dim5, double time)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 5, dims, data, time);
}

int idamPutVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, 
    int dim4, int dim5, double time)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 5, dims, data, time);
}

int idamPutVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, 
    int dim4, int dim5, int dim6, double time)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return putDataSlice(expIdx, cpoPath, path, INT, 6, dims, data, time);
}

int idamPutVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, 
    int dim4, int dim5, int dim6, double time)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim5;
    return putDataSlice(expIdx, cpoPath, path, FLOAT, 6, dims, data, time);
}

int idamPutVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, 
    int dim4, int dim5, int dim6, double time)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return putDataSlice(expIdx, cpoPath, path, DOUBLE, 6, dims, data, time);
}

int idamReplaceLastIntSlice(int expIdx, char *cpoPath, char *path, int data)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 0, NULL, &data);
}

int idamReplaceLastFloatSlice(int expIdx, char *cpoPath, char *path, float data)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 0, NULL, &data);
}

int idamReplaceLastDoubleSlice(int expIdx, char *cpoPath, char *path, double data)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 0, NULL, &data);
}

int idamReplaceLastStringSlice(int expIdx, char *cpoPath, char *path, char *data)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, STRING, 0, NULL, &data);
}

int idamReplaceLastVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 1, &dim, data);
}

int idamReplaceLastVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 1, &dim, data);
}

int idamReplaceLastVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim)
{
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 1, &dim, data);
}

int idamReplaceLastVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 2, dims, data);
}

int idamReplaceLastVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 2, dims, data);
}

int idamReplaceLastVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2)
{
    int dims[2];
    dims[0] = dim1;
    dims[1] = dim2;
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 2, dims, data);
}

int idamReplaceLastVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 3, dims, data);
}

int idamReplaceLastVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 3, dims, data);
}

int idamReplaceLastVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3)
{
    int dims[3];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 3, dims, data);
}

int idamReplaceLastVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 4, dims, data);
}

int idamReplaceLastVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 4, dims, data);
}

int idamReplaceLastVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4)
{
    int dims[4];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 4, dims, data);
}

int idamReplaceLastVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, 
    int dim4, int dim5)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 5, dims, data);
}

int idamReplaceLastVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, 
    int dim4, int dim5)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 5, dims, data);
}

int idamReplaceLastVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5)
{
    int dims[5];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 5, dims, data);
}

int idamReplaceLastVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, 
    int dim4, int dim5, int dim6)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return replaceLastDataSlice(expIdx, cpoPath, path, INT, 6, dims, data);
}

int idamReplaceLastVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, 
    int dim4, int dim5, int dim6)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
    return replaceLastDataSlice(expIdx, cpoPath, path, FLOAT, 6, dims, data);
}

int idamReplaceLastVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6)
{
    int dims[6];
    dims[0] = dim1;
    dims[1] = dim2;
    dims[2] = dim3;
    dims[3] = dim4;
    dims[4] = dim5;
    dims[5] = dim6;
   return replaceLastDataSlice(expIdx, cpoPath, path, DOUBLE, 6, dims, data);
}

int idamGetString(int expIdx, char *cpoPath, char *path, char **data)
{
    int dims[1];
    return getData(expIdx, cpoPath, path, STRING, 0, dims, data);	// Scalar String
}

int idamGetFloat(int expIdx, char *cpoPath, char *path, float *data)
{
    int dims[1];
    int status;
    float *tmp; //we cannot use data directly for scalars because getData returns a pointer on pointer
    status = getData(expIdx, cpoPath, path, FLOAT, 0, dims, (char **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int idamGetInt(int expIdx, char *cpoPath, char *path, int *data)
{
    int dims[1];
    int status;
    int *tmp; //we cannot use data directly for scalars because getData returns a pointer on pointer
    status = getData(expIdx, cpoPath, path, INT, 0, dims, (char **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int idamGetDouble(int expIdx, char *cpoPath, char *path, double *data)
{
    int dims[1];
    int status;
    double *tmp; //we cannot use data directly for scalars because getData returns a pointer on pointer
    status = getData(expIdx, cpoPath, path, DOUBLE, 0, dims, (char **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int idamGetVect1DString(int expIdx, char *cpoPath, char *path, char  ***data, int *dim)
{
    return getData(expIdx, cpoPath, path, STRING_VECTOR, 1, dim, (char **)data);
}

int idamGetVect1DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim)
{
    return getData(expIdx, cpoPath, path, INT, 1, dim, (char **)data);
}

int idamGetVect1DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim)
{
    return getData(expIdx, cpoPath, path, FLOAT, 1, dim, (char **)data);
}

int idamGetVect1DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim)
{
    return getData(expIdx, cpoPath, path, DOUBLE, 1, dim, (char **)data);
}

int idamGetVect2DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2)
{
    int dims[2];
    int status;
    status =  getData(expIdx, cpoPath, path, INT, 2, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    return status;
}

int idamGetVect2DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2)
{
    int dims[2];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 2, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    return status;
}

int idamGetVect2DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2)
{
    int dims[2];
    int status;
    status =  getData(expIdx, cpoPath, path, DOUBLE, 2, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    return status;
}

int idamGetVect3DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3)
{
    int dims[3];
    int status;
    status =  getData(expIdx, cpoPath, path, INT, 3, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return status;
}

int idamGetVect3DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3)
{
    int dims[3];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 3, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return status;
}

int idamGetVect3DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3)
{
    int dims[3];
    int status;
    status = getData(expIdx, cpoPath, path, DOUBLE, 3, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return status;
}

int idamGetVect4DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dims[4];
    int status;
    status = getData(expIdx, cpoPath, path, INT, 4, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return status;
}

int idamGetVect4DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dims[4];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 4, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return status;
}

int idamGetVect4DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dims[4];
    int status;
    status = getData(expIdx, cpoPath, path, DOUBLE, 4, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return status;
}

int idamGetVect5DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int dims[5];
    int status;
    status = getData(expIdx, cpoPath, path, INT, 5, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return status;
}

int idamGetVect5DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5)
{
    int dims[5];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 5, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return status;
}

int idamGetVect5DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5)
{
    int dims[5];
    int status;
    status = getData(expIdx, cpoPath, path, DOUBLE, 5, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return status;
}

int idamGetVect6DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, 
    int *dim5, int *dim6)
{
    int dims[6];
    int status;
    status = getData(expIdx, cpoPath, path, INT, 6, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return status;
}

int idamGetVect6DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5, int *dim6)
{
    int dims[6];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 6, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return status;
}

int idamGetVect6DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5, int *dim6)
{
    int dims[6];
    int status;
    status = getData(expIdx, cpoPath, path, DOUBLE, 6, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return status;
}

int idamGetVect7DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, 
    int *dim5, int *dim6, int *dim7)
{
    int dims[7];
    int status;
    status = getData(expIdx, cpoPath, path, INT, 7, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    *dim7 = dims[6];
    return status;
}

int idamGetVect7DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dims[7];
    int status;
    status = getData(expIdx, cpoPath, path, FLOAT, 7, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    *dim7 = dims[6];
    return status;
}

int idamGetVect7DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dims[7];
    int status;
    status = getData(expIdx, cpoPath, path, DOUBLE, 7, dims, (char **)data);
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    *dim7 = dims[6];
    return status;
}

int idamBeginCPOGetSlice(int expIdx, char *cpoPath, double time)
{
    return getSliceIdxs(expIdx, cpoPath, time);

}

int idamGetStringSlice(int expIdx, char *cpoPath, char *path, char **data, double time, double *retTime, int interpolMode)
{
    sprintf(errmsg, "getStringSlice not supported\n");
    return ERROR_RETURN_VALUE;

}

int idamGetIntSlice(int expIdx, char *cpoPath, char *path, int *data, double time, double *retTime, int interpolMode)
{
   int dims[7];
   int *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, INT, 0, dims, (char **)&currData, time, retTime, interpolMode);
   *data = *currData;
   free((char *)currData);
   return rc;

/* 
    int y1, y2;
    int status;
    int dims[16];
    int *currData;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 0, dims, sliceIdx1, 1, (char **)&currData);
        if (!status) {
            *data = *currData;
            free(currData);
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 0, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData[0];
    y2 = currData[1];
    if(status) return status;
    	switch(interpolMode) {
		case INTERPOLATION: 
			*data = y1 + (y2 - y1)*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
			*retTime = time;
			break;
		case CLOSEST_SAMPLE: 
			if(time - sliceTime1 < sliceTime2 - time)
			{
				*data = y1;
				*retTime = sliceTime1;
			}
			else
			{
				*data = y2;
				*retTime = sliceTime2;
			}
			break;
		case PREVIOUS_SAMPLE:
			*data = y1;
			*retTime = sliceTime1;
			break;
	}
        free((char *)currData);
        return 0;
*/
}

int idamGetFloatSlice(int expIdx, char *cpoPath, char *path, float *data, double time, double *retTime, int interpolMode)
{
   int dims[7];
   float *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, FLOAT, 0, dims, (char **)&currData, time, retTime, interpolMode);
   *data = *currData;
   free((char *)currData);
   return rc;
   
/*
    float y1, y2;
    int status;
    int dims[16];
    float *currData;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 0, dims, sliceIdx1, 1, (char **)&currData);
        if (!status) {
            *data = *currData;
            free(currData);
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 0, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData[0];
    y2 = currData[1];
    if(status) return status;
    	switch(interpolMode) {
		case INTERPOLATION: 
			*data = y1 + (y2 - y1)*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
			*retTime = time;
			break;
		case CLOSEST_SAMPLE: 
			if(time - sliceTime1 < sliceTime2 - time)
			{
				*data = y1;
				*retTime = sliceTime1;
			}
			else
			{
				*data = y2;
				*retTime = sliceTime2;
			}
			break;
		case PREVIOUS_SAMPLE:
			*data = y1;
			*retTime = sliceTime1;
			break;
	}
        free((char *)currData);
        return 0;
*/
}

int idamGetDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, double time, double *retTime, int interpolMode)
{
   int dims[7];
   double *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, DOUBLE, 0, dims, (char **)&currData, time, retTime, interpolMode);
   *data = *currData;
   free((char *)currData);
   return rc;
   
/*
    double y1, y2;
    int status;
    int dims[16];
    double *currData;

    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 0, dims, sliceIdx1, 1, (char **)&currData);
        if (!status) {
            *data = *currData;
            free(currData);
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 0, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData[0];
    y2 = currData[1];
    if(status) return status;
    	switch(interpolMode) {
		case INTERPOLATION: 
			*data = y1 + (y2 - y1)*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
			*retTime = time;
			break;
		case CLOSEST_SAMPLE: 
			if(time - sliceTime1 < sliceTime2 - time)
			{
				*data = y1;
				*retTime = sliceTime1;
			}
			else
			{
				*data = y2;
				*retTime = sliceTime2;
			}
			break;
		case PREVIOUS_SAMPLE:
			*data = y1;
			*retTime = sliceTime1;
			break;
	}
        free((char *)currData);
        return 0;
*/
}

int idamGetVect1DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim, double time, double *retTime, int interpolMode)
{
   int dims[7];
   int *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, INT, 1, dims, (char **)&currData, time, retTime, interpolMode);
   *dim = dims[0];
   *data = currData;
   return rc;

/*       
    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 1, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim = dims[0];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 1, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim = dims[0];
    return 0;
*/
}

int idamGetVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim, double time, double *retTime, int interpolMode)
{
   int dims[7];
   float *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, FLOAT, 1, dims, (char **)&currData, time, retTime, interpolMode);
   *dim = dims[0];
   *data = currData;
   return rc;

/*    
    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 1, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim = dims[0];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 1, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim = dims[0];
    return 0;
*/
}

int idamGetVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim, double time, double *retTime, int interpolMode)
{ 
   int dims[7];
   double *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, DOUBLE, 1, dims, (char **)&currData, time, retTime, interpolMode);
   *dim = dims[0];
   *data = currData;
   return rc;

/*   
    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 1, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim = dims[0];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 1, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim = dims[0];
    return 0;
*/
}

int idamGetVect2DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{
   int dims[7];
   int *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, INT, 2, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *data = currData;
   return rc;

/*    
    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 2, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 2, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    return 0;
*/
}

int idamGetVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{
   int dims[7];
   float *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, FLOAT, 2, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *data = currData;
   return rc;

/*    
    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 2, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 2, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    return 0;
*/
}

int idamGetVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{
   int dims[7];
   double *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, DOUBLE, 2, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *data = currData;
   return rc;

/*     
    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 2, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 2, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    return 0;
*/
}

int idamGetVect3DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{
   int dims[7];
   int *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, INT, 3, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *data = currData;
   return rc;

/*     
    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 3, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 3, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return 0;
*/
}

int idamGetVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{
   int dims[7];
   float *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, FLOAT, 3, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *data = currData;
   return rc;

/*    
    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 3, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 3, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2&i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return 0;
*/}

int idamGetVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{
   int dims[7];
   double *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, DOUBLE, 3, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *data = currData;
   return rc;

/*    
    
    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 3, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 3, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    return 0;
*/}
//////////////////
int idamGetVect4DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, double time, double *retTime, int interpolMode)
{
   int dims[7];
   int *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, INT, 4, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *dim4 = dims[3];
   *data = currData;
   return rc;

/*       
    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 4, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 4, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2]* dims[3];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return 0;
*/
}

int idamGetVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, double time, double *retTime, int interpolMode)
{
   int dims[7];
   float *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, FLOAT, 4, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *dim4 = dims[3];
   *data = currData;
   return rc;

/*    
    
    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 4, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 4, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2]*dims[3];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2&i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return 0;
*/
}

int idamGetVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, double time, double *retTime, int interpolMode)
{
   int dims[7];
   double *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, DOUBLE, 4, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *dim4 = dims[3];
   *data = currData;
   return rc;

/*    
    
    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 4, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 4, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2]*dims[3];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    return 0;
*/
}
///////////////
int idamGetVect5DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{
   int dims[7];
   int *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, INT, 5, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *dim4 = dims[3];
   *dim5 = dims[4];
   *data = currData;
   return rc;

/*        
    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 5, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 5, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2]* dims[3]*dims[4];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return 0;
*/
}

int idamGetVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{
   int dims[7];
   float *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, FLOAT, 5, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *dim4 = dims[3];
   *dim5 = dims[4];
   *data = currData;
   return rc;

/*    
    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 5, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 5, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2]*dims[3]*dims[4];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2&i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return 0;
*/
}

int idamGetVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{
   int dims[7];
   double *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, DOUBLE, 5, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *dim4 = dims[3];
   *dim5 = dims[4];
   *data = currData;
   return rc;

/*    
    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 5, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 5, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2]*dims[3]*dims[4];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    return 0;
*/
}


int idamGetVect6DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{
   int dims[7];
   int *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, INT, 6, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *dim4 = dims[3];
   *dim5 = dims[4];
   *dim6 = dims[5];
   *data = currData;
   return rc;

/*    
    int *y1, *y2;
    int status;
    int dims[16];
    int *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, INT, 6, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
            *dim6 = dims[5];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, INT, 6, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2]* dims[3]*dims[4]*dims[5];
    retData = (int *)malloc(sizeof(int) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return 0;
*/
}

int idamGetVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{
   int dims[7];
   float *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, FLOAT, 6, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *dim4 = dims[3];
   *dim5 = dims[4];
   *dim6 = dims[5];
   *data = currData;
   return rc;

/*    
    
    float *y1, *y2;
    int status;
    int dims[16];
    float *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, FLOAT, 6, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
            *dim6 = dims[5];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, FLOAT, 6, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2]*dims[3]*dims[4]*dims[5];
    retData = (float *)malloc(sizeof(float) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2&i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return 0;
*/
}

int idamGetVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, 
    int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{
   int dims[7];
   double *currData;
   int rc = getDataSlices(expIdx, cpoPath, path, DOUBLE, 6, dims, (char **)&currData, time, retTime, interpolMode);
   *dim1 = dims[0];
   *dim2 = dims[1];
   *dim3 = dims[2];
   *dim4 = dims[3];
   *dim5 = dims[4];
   *dim6 = dims[5];
   *data = currData;
   return rc;

/*        
    double *y1, *y2;
    int status;
    int dims[16];
    double *currData, *retData;
    int nItems, i;
    
    if(sliceIdx2 == -1) //Only a single sample
    {
        status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 6, dims, sliceIdx1, 1, (char **)data);
        if (!status) {
            *dim1 = dims[0];
            *dim2 = dims[1];
            *dim3 = dims[2];
            *dim4 = dims[3];
            *dim5 = dims[4];
            *dim6 = dims[5];
        }
        *retTime = sliceTime1;
        return status;
    }
    status = getDataSlices(expIdx, cpoPath, path, DOUBLE, 6, dims, sliceIdx1, 2, (char **)&currData);
    if(status) return status;
    y1 = currData;
    y2 = &currData[1];
    
    nItems = dims[0]*dims[1]*dims[2]*dims[3]*dims[4]*dims[5];
    retData = (double *)malloc(sizeof(double) * nItems);

    for(i = 0; i < nItems; i++)
    {
            switch(interpolMode) {
                    case INTERPOLATION: 
                            retData[i] = y1[2*i] + (y2[2*i] - y1[2*i])*(time - sliceTime1)/(sliceTime2 - sliceTime1); 
                            *retTime = time;
                            break;
                    case CLOSEST_SAMPLE: 
                            if(time - sliceTime1 < sliceTime2 - time)
                            {
                                    retData[i] = y1[2*i];
                                    *retTime = sliceTime1;
                            }
                            else
                            {
                                    retData[i] = y2[2*i];
                                    *retTime = sliceTime2;
                            }
                            break;
                    case PREVIOUS_SAMPLE:
                            retData[i] = y1[2*i];
                            *retTime = sliceTime1;
                            break;
            }
    }
    free((char *)currData);
    *data = retData;
    *dim1 = dims[0];
    *dim2 = dims[1];
    *dim3 = dims[2];
    *dim4 = dims[3];
    *dim5 = dims[4];
    *dim6 = dims[5];
    return 0;
*/
}



int idamBeginCPOGet(int expIdx, char *path, int isTimed, int *retSamples) 
{
    int status;
    double *times;
    int dim;
    
    if(!isTimed)
            *retSamples = 1;
    else
    {
        status = idamGetVect1DDouble(expIdx, path, "time", &times, &dim);
        if(status) return status;
        *retSamples = dim;
        free((char *)times);
    }
    return OK_RETURN_VALUE;
}



int idamEndCPOGet(int expIdx, char *path) {return 0;}
int idamEndCPOGetSlice(int expIdx, char *path) {return 0;}
int idamBeginCPOPut(int expIdx, char *path) {return 0;}
int idamEndCPOPut(int expIdx, char *path) {return 0;}
int idamBeginCPOPutTimed(int expIdx, char *path, int samples, double *inTimes) {return 0;}
int idamEndCPOPutTimed(int expIdx, char *path) {return 0;}
int idamBeginCPOPutNonTimed(int expIdx, char *path) {return 0;}
int idamEndCPOPutNonTimed(int expIdx, char *path) {return 0;}
int idamBeginCPOPutSlice(int expIdx, char *path)
{
    int nDims,dim1;
    
    // get the current number of time slices.
    // This will be used when by leaves of slice objects to know whether some times have been skipped
    // (because they were empty and therefore not written to the database)
    if (idamGetDimension(expIdx,path, "time",&nDims,&dim1,NULL,NULL,NULL,NULL,NULL,NULL)<0) {
        sliceIdx1 = 0;      // there is no slice yet
    } else {
        sliceIdx1 = dim1;   //we are going to write right after the last slice
    }
    return OK_RETURN_VALUE;
}

int idamEndCPOPutSlice(int expIdx, char *path) {return 0;}
int idamBeginCPOReplaceLastSlice(int expIdx, char *path)
{
    int nDims,dim1;
    
    // get the current number of time slices.
    // This will be used when by leaves of slice objects to know whether some times have been skipped
    // (because they were empty and therefore not written to the database)
    if (idamGetDimension(expIdx,path, "time",&nDims,&dim1,NULL,NULL,NULL,NULL,NULL,NULL)<0 || dim1==0) {
        sliceIdx1 = 0;      // there is no slice yet
    } else {
        sliceIdx1 = dim1-1;  // we are going to overwrite last slice
    }
    return OK_RETURN_VALUE;

}

int idamEndCPOReplaceLastSlice(int expIdx, char *path) {return 0;}


/* ---------------------- *
 *  Arrays of structures  *
 * ---------------------- */

/** Append data to a field
 * If we are dealing with non timed data, the field is assumed to have
 * been cleared previously
 * The size of the field can vary between slices.
 * The size of the field is therefore stored in an associated "_size" field
 * nDims should be 0 for scalars
 * If the specified slice is not the next slice to be filled
 * then we insert empty slices */
 
// dgm Replacement for Original IMAS HDF5 putDataSliceInObject (renamed imas_putDataSliceInObject) that calls IDAM 
// The obj is a memory address

static void *putDataSliceInObject(void *obj, char *path, int index, int type, int nDims, int *dims, void *data){

// Create PUTDATA variables   
    
   PUTDATA_BLOCK putData[2];
   initIdamPutDataBlock(&putData[0]);
   initIdamPutDataBlock(&putData[1]);
   
   PUTDATA_BLOCK_LIST putDataBlockList;
   initIdamPutDataBlockList(&putDataBlockList);   
   
// Pass the Data

   int i;
   putData[0].data_type = TYPE_INT; 
   putData[0].count = 1;
   putData[0].rank  = 0;
   putData[0].shape = NULL;
   putData[0].data  = obj;

   putData[1].data_type = findIMASIDAMType(type);	// Data Slice to be inserted
   putData[1].data  = data;
   putData[1].rank  = nDims;
   if(dims != NULL){
      putData[1].count = dims[0];
      putData[1].shape = dims;
      for(i=1;i<putData[1].rank;i++)putData[1].count *= dims[i]; 
   } else {
      putData[1].count = 1;
   }   
   
   addIdamPutDataBlockList(&putData[0], &putDataBlockList);
   addIdamPutDataBlockList(&putData[1], &putDataBlockList);
   
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::putObject(path='%s', index=%d, %s)", path, index, keyword);
   
   int handle = idamPutListAPI(directive, &putDataBlockList);   

   if(directive) free(directive);
   freeIdamClientPutDataBlockList(&putDataBlockList);  
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error PUTing Data Slice in Object: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return NULL;
   }

   int *refId = (int *)getIdamData(handle);	// Returned Object Reference   
   if(refId == NULL){
      sprintf(errmsg, "Error PUTing Data Slice in Object: Object not returned");
      idamFree(handle);
      return NULL;
   }      

// Detach heap from the IDAM structure then free the IDAM structure (avoid data leaks within IDAM)

   getIdamDataBlock(handle)->data = NULL;      
   idamFree(handle);	 
   
   return refId;
}

/** Read a single slice from a field
 *  The size of the field is read in the associated "_size" field
 *  nDims should be 0 for scalars
 *  If the required slice does not exist (is higher than the last slice)
 *  then we return an empty result ("empty" value for scalars, size 0 for arrays)
 *  It is possible to read only the size (not the data) by setting type to DIMENSION */
 
// dgm Replacement for Original IMAS HDF5 gettDataSliceInObject (renamed imas_gettDataSliceInObject) that calls IDAM 
// The obj is a memory address
static int getDataSliceFromObject(void *obj, char *path, int index, int type, int nDims, int *dims, void **data){
   
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   if( type == DIMENSION)
      sprintf(directive, "imas::getObject(path='%s', index=%d, objectId=%d, /getDimension, %s)", path, index, *((int *)obj), keyword);
   else
      sprintf(directive, "imas::getObject(path='%s', index=%d, objectId=%d, rank=%d, type='%s', %s)", path, index, *((int *)obj), nDims, convertIdam2StringType(findIMASIDAMType(type)), keyword);
   
       
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error GETing Data Slice in Object: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   if(type != DIMENSION && nDims != getIdamRank(handle)){
      sprintf(errmsg, "Error GETing Data: Inconsistent Data Rank");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
      
   if(type != DIMENSION && dims == NULL){
      sprintf(errmsg, "Error GETing Data: Unable to return Object Dimensions - NULL array allocation!");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
// Object Data
   
   if(data != NULL) *data = (void *)getIdamData(handle);

// Dimension Lengths

   if(type != DIMENSION){
      if(nDims > 0){
         if(getIdamDataNum(handle) > 0 && getIdamDimBlock(handle, 0) != NULL){
            int i;
            for(i=0;i<nDims;i++) dims[i] = getIdamDimNum(handle, i);
         } else
            dims[0] = 0;
      } else
         dims[0] = getIdamDataNum(handle);

// Detach heap from the IDAM structure then free the IDAM structure (avoid data leaks within IDAM)

      getIdamDataBlock(handle)->data = NULL;      

   } else {
      int i;
      for(i=0;i<nDims;i++) dims[i] = ((int *) *data)[i];
      
      int *dataArg = (int *)malloc(sizeof(int));
      dataArg[0] = nDims;
      *data = dataArg;
   }
   
         
   idamFree(handle);	// Application is responsible for freeing data  
   
   return OK_RETURN_VALUE;
}

static int getDataSliceFromObjectXXX(void *obj, char *path, int index, int type, int nDims, int *dims, void **data){

// get PUTDATA variables   
    
   PUTDATA_BLOCK putData[2];
   initIdamPutDataBlock(&putData[0]);
   initIdamPutDataBlock(&putData[1]);
   
   PUTDATA_BLOCK_LIST putDataBlockList;
   initIdamPutDataBlockList(&putDataBlockList);   
   
// Pass the Data

   putData[0].data_type = TYPE_INT; 
   putData[0].count = 1;
   putData[0].rank  = 0;
   putData[0].shape = NULL;
   putData[0].data  = obj;

   putData[1].data_type = TYPE_INT;	 
   putData[1].count = nDims;
   putData[1].rank  = 1;
   putData[1].shape = NULL;
   putData[1].data = (void *)dims;   

   int scalar[1] = {1};
   if(dims == NULL){
      putData[1].rank = 0;
      putData[1].data = (void *)scalar; 
   }
   
   addIdamPutDataBlockList(&putData[0], &putDataBlockList);
   addIdamPutDataBlockList(&putData[1], &putDataBlockList);
   
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::getObject(path='%s', index=%d, type='%s', %s)", path, index, convertIdam2StringType(findIMASIDAMType(type)), keyword);
   
   int handle = idamPutListAPI(directive, &putDataBlockList);   

   if(directive) free(directive);
   freeIdamClientPutDataBlockList(&putDataBlockList);  
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error GETing Data Slice in Object: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   if(nDims != getIdamRank(handle)){
      sprintf(errmsg, "Error GETing Data: Inconsistent Data Rank");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   if(data != NULL) *data = (void *)getIdamData(handle);
   
// Detach heap from the IDAM structure then free the IDAM structure (avoid data leaks within IDAM)

   getIdamDataBlock(handle)->data = NULL;      
   idamFree(handle);	// Application is responsible for freeing data  
   
   return OK_RETURN_VALUE;
}


// dgm Replacement for Original IMAS HDF5 idamBeginObject (renamed imas_idamBeginObject) that calls IDAM 
// dgm isTimed is Not a BOOL 

void *idamBeginObject(int expIdx, void *obj, int index, const char *relPath, int isTimed){

// PUTDATA variable   
    
   int i;
   int *stdObjRef = NULL;
   PUTDATA_BLOCK putData[2];
   for(i=0;i<2;i++) initIdamPutDataBlock(&putData[i]);
   
   PUTDATA_BLOCK_LIST putDataBlockList;
   initIdamPutDataBlockList(&putDataBlockList);   

// Pass the Data

   putData[0].data_type = TYPE_INT;		// reference to a serverside Address - 32 or 64 bit 
   putData[0].count = 1;	 
   putData[0].rank  = 0;
   putData[0].shape = NULL;
   if(obj == NULL){				// Standard Object ID 0			 
      stdObjRef = (int *)malloc(sizeof(int));
      stdObjRef[0] = 0;
      putData[0].data = (void *)stdObjRef;
   } else
   if(obj == (void *)-1){			// Standard Object ID 1
      stdObjRef = (int *)malloc(sizeof(int));
      stdObjRef[0] = 1;
      putData[0].data = (void *)stdObjRef;
   } else   
      putData[0].data = obj;

   putData[1].data_type = TYPE_INT;	 
   putData[1].rank  = 1;
   putData[1].count = 2;
   putData[1].shape = NULL;
   int *sliceIdx = (int *)malloc(putData[1].count*sizeof(int));			// Pass static variables from the application to the server
   sliceIdx[0] = sliceIdx1;
   sliceIdx[1] = sliceIdx2;
   putData[1].data = (void *)sliceIdx;

   for(i=0;i<2;i++) addIdamPutDataBlockList(&putData[i], &putDataBlockList);
   
   int lstr = DIRECTIVELENGTH + strlen(relPath);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::beginObject(idx=%d, index=%d, relPath='%s', isTimed=%d, %s)", expIdx, index, relPath, isTimed, keyword);
         
   int handle = idamPutListAPI(directive, &putDataBlockList);  

   if(directive) free(directive);
   if(sliceIdx)  free(sliceIdx);
   freeIdamClientPutDataBlockList(&putDataBlockList);
   if(stdObjRef != NULL)free((void *)stdObjRef);  
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error BeginObject: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return NULL;
   }
   
   int *refId = (int *)getIdamData(handle);	// Returned Object reference
   if(refId == NULL){
      sprintf(errmsg, "Error BeginObject: No returned Object");
      idamFree(handle);
      return NULL;
   }
   
// Detach heap from the IDAM structure then free the IDAM structure (avoid data leaks within IDAM)

   getIdamDataBlock(handle)->data = NULL;      
   idamFree(handle);	// Application is responsible for freeing data  
   
   return refId;
}


// dgm Replacement for Original IMAS HDF5 idamGetObject (renamed imas_idamGetObject) that calls IDAM  

int idamGetObject(int expIdx, char *path, char *cpoPath, void **obj, int isTimed){
   
   int lstr = DIRECTIVELENGTH + strlen(path) + strlen(cpoPath);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::getObjectGroup(idx=%d, path='%s', cpoPath='%s', isTimed=%d, %s)", expIdx, path, cpoPath, isTimed, keyword);
      
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error getObjectGroup: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   void *data = (void *)getIdamData(handle);
   if(data == NULL){
      sprintf(errmsg, "Error getObjectGroup: No returned Data");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   *obj = (void *) data;	// Returned Object reference

// Detach heap from the IDAM structure then free the IDAM structure (avoid data leaks within IDAM)

   getIdamDataBlock(handle)->data = NULL;      
   idamFree(handle);	// Application is responsible for freeing data  
   
   return OK_RETURN_VALUE;
}


// dgm Replacement for Original IMAS HDF5 idamGetObjectSlice (renamed imas_idamGetObjectSlice) that calls IDAM  
// Pass static variables sliceIdx1 etc. to the server 
//int idamGetObjectSlice(int expIdx, char *idamPath, char *cpoPath, double time, void **obj){
int idamGetObjectSlice(int expIdx, char *cpoPath, char *path, double time, void **obj){

// PUTDATA variable   
    
   int i;
   PUTDATA_BLOCK putData[3];
   for(i=0;i<3;i++) initIdamPutDataBlock(&putData[i]);
   
   PUTDATA_BLOCK_LIST putDataBlockList;
   initIdamPutDataBlockList(&putDataBlockList);   


// Pass the Data

   putData[0].data_type = TYPE_DOUBLE;	 
   putData[0].rank  = 0;
   putData[0].count = 1;
   putData[0].shape = NULL;
   putData[0].data  = (void *) &time;	// Do not Free!

   putData[1].data_type = TYPE_INT;	 
   putData[1].rank  = 1;
   putData[1].count = 2;
   putData[1].shape = NULL;
   int *sliceIdx = (int *)malloc(putData[1].count*sizeof(int));			// Pass static variables from the application to the server
   sliceIdx[0] = sliceIdx1;
   sliceIdx[1] = sliceIdx2;
   putData[1].data = (void *)sliceIdx;

   putData[2].data_type = TYPE_DOUBLE;	 
   putData[2].rank  = 1;
   putData[2].count = 2;
   putData[2].shape = NULL;
   double *sliceTime = (double *)malloc(putData[1].count*sizeof(double));	// Pass static variables from the application to the server
   sliceTime[0] = sliceTime1;
   sliceTime[1] = sliceTime2;
   putData[2].data = (void *)sliceTime;
   
   for(i=0;i<3;i++) addIdamPutDataBlockList(&putData[i], &putDataBlockList);
   
   int lstr = DIRECTIVELENGTH + strlen(path) + strlen(cpoPath);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::getObjectSlice(idx=%d, cpoPath='%s', path='%s', %s)", expIdx, cpoPath, path, keyword);   
      
   int handle = idamPutListAPI(directive, &putDataBlockList);  

   if(directive) free(directive);
   if(sliceIdx)  free(sliceIdx);
   if(sliceTime) free(sliceTime);
   freeIdamClientPutDataBlockList(&putDataBlockList);  
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error getObjectSlice: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   *obj = (void *)getIdamData(handle);	// Returned Object reference
     
   if(*obj == NULL){
      sprintf(errmsg, "Error getObjectSlice: No returned Data");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

// Detach heap from the IDAM structure then free the IDAM structure (avoid data leaks within IDAM)

   getIdamDataBlock(handle)->data = NULL;      
   idamFree(handle);	// Application is responsible for freeing data  
   
   return OK_RETURN_VALUE;
}


// dgm Replacement for Original IMAS HDF5 idamGetObjectFromObject (renamed imas_idamGetObjectFromObject) that calls IDAM  

int idamGetObjectFromObject(void *obj, char *path, int index, void **dataObj){

// PUTDATA variable   
    
   PUTDATA_BLOCK putData;
   initIdamPutDataBlock(&putData);

// Pass the Data

   putData.data_type = TYPE_INT; 
   putData.rank  = 0;
   putData.count = 1;
   putData.shape = NULL;
   putData.data  = obj;
   
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::getObjectObject(index=%d, path='%s', %s)", index, path, keyword);   
      
   int handle = idamPutAPI(directive, &putData);  

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error getObjectObject: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   *dataObj = (void *)getIdamData(handle);		// Returned Object reference
   
   if(*dataObj == NULL){
      sprintf(errmsg, "Error getObjectObject: No returned Data");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

// Detach heap from the IDAM structure then free the IDAM structure (avoid data leaks within IDAM)

   getIdamDataBlock(handle)->data = NULL;      
   idamFree(handle);	// Application is responsible for freeing data  
   
   return OK_RETURN_VALUE;
}


// dgm Replacement for Original IMAS HDF5 idamReleaseObject (renamed imas_idamReleaseObject) that calls IDAM  

void idamReleaseObject(void *obj){

// PUTDATA variable   
    
   PUTDATA_BLOCK putData;
   initIdamPutDataBlock(&putData);

// Pass the Data

   putData.data_type = TYPE_INT; 
   putData.rank  = 0;
   putData.count = 1;
   putData.shape = NULL;
   putData.data  = obj;
   
   int lstr = DIRECTIVELENGTH;
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::releaseObject(%s)", keyword);   
      
   int handle = idamPutAPI(directive, &putData);  

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error releaseObject: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return;
   }
   
   int *data = (int *)getIdamData(handle);
   if(data == NULL){
      sprintf(errmsg, "Error releaseObject: No returned status code");
      idamFree(handle);
      return;
   }
   
   int rc = *data;
   
   if(rc != OK_RETURN_VALUE){
      sprintf(errmsg, "Error releaseObject: release failed");
      idamFree(handle);
      return;
   }

   idamFree(handle);	 
   
   return;
}


int idamPutObject(int currIdx, char *cpoPath, char *path, void *obj, int isTimed)
{
    // Since all data has been written along the way by
    // the idamPut...InObject functions, then there is nothing left
    // to do except freeing the memory
//  idamReleaseObject(obj);
    
// int mdsPutObject(int expIdx, char *cpoPath, char *path, void *obj, int isTimed);

   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::putObjectGroup(idx=%d, cpoPath='%s', path='%s', objectId=%d, isTimed=%d, %s)", currIdx, cpoPath, path, ((int *)obj)[0], isTimed, keyword);
       
   int handle = idamGetAPI(directive, "");  

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error releaseObject: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   int *data = (int *)getIdamData(handle);
   if(data == NULL){
      sprintf(errmsg, "Error releaseObject: No returned status code");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   int rc = *data;
   
   if(rc != OK_RETURN_VALUE){
      sprintf(errmsg, "Error releaseObject: release failed");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

   idamFree(handle);	 
   
   return OK_RETURN_VALUE;
}

void *idamPutIntInObject(void *obj, char *path, int idx, int data)
{
    return putDataSliceInObject(obj, path, idx, INT, 0, NULL, (void *)&data);
    
    //if (status)
    //    obj = NULL;
    //return obj;
}

void *idamPutFloatInObject(void *obj, char *path, int idx, float data)
{
    return putDataSliceInObject(obj, path, idx, FLOAT, 0, NULL, (void *)&data);
}

void *idamPutDoubleInObject(void *obj, char *path, int idx, double data)
{
    return putDataSliceInObject(obj, path, idx, DOUBLE, 0, NULL, (void *)&data);
}

void *idamPutStringInObject(void *obj, char *path, int idx, char *data)
{

// dgm    putDataSliceInObject(obj, path, idx, STRING, 0, NULL, (void *)&data);
    return putDataSliceInObject(obj, path, idx, STRING, 0, NULL, (void *)data);
}

void *idamPutVect1DStringInObject(void *obj, char *path, int idx, char **data, int dim)
{
    return putDataSliceInObject(obj, path, idx, STRING, 1, &dim, (void *)data);
}

void *idamPutVect1DIntInObject(void *obj, char *path, int idx, int *data, int dim)
{
    return putDataSliceInObject(obj, path, idx, INT, 1, &dim, (void *)data);
}

void *idamPutVect1DFloatInObject(void *obj, char *path, int idx, float *data, int dim)
{
    return putDataSliceInObject(obj, path, idx, FLOAT, 1, &dim, (void *)data);
}

void *idamPutVect1DDoubleInObject(void *obj, char *path, int idx, double *data, int dim)
{
    return putDataSliceInObject(obj, path, idx, DOUBLE, 1, &dim, (void *)data);
}

void *idamPutVect2DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2)
{
    int dim[2] = {dim1,dim2};
    return putDataSliceInObject(obj, path, idx, INT, 2, dim, (void *)data);
}

void *idamPutVect2DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2)
{
    int dim[2] = {dim1,dim2};
    return putDataSliceInObject(obj, path, idx, FLOAT, 2, dim, (void *)data);
}

void *idamPutVect2DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2)
{
    int dim[2] = {dim1,dim2};
    return putDataSliceInObject(obj, path, idx, DOUBLE, 2, dim, (void *)data);
}

void *idamPutVect3DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3)
{
    int dim[3] = {dim1,dim2,dim3};
    return putDataSliceInObject(obj, path, idx, INT, 3, dim, (void *)data);
}

void *idamPutVect3DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3)
{
    int dim[3] = {dim1,dim2,dim3};
    return putDataSliceInObject(obj, path, idx, FLOAT, 3, dim, (void *)data);
}

void *idamPutVect3DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3)
{
    int dim[3] = {dim1,dim2,dim3};
    return putDataSliceInObject(obj, path, idx, DOUBLE, 3, dim, (void *)data);
}

void *idamPutVect4DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4)
{
    int dim[4] = {dim1,dim2,dim3,dim4};
    return putDataSliceInObject(obj, path, idx, INT, 4, dim, (void *)data);
}

void *idamPutVect4DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4)
{
    int dim[4] = {dim1,dim2,dim3,dim4};
    return putDataSliceInObject(obj, path, idx, FLOAT, 4, dim, (void *)data);
}

void *idamPutVect4DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4)
{
    int dim[4] = {dim1,dim2,dim3,dim4};
    return putDataSliceInObject(obj, path, idx, DOUBLE, 4, dim, (void *)data);
}

void *idamPutVect5DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5)
{
    int dim[5] = {dim1,dim2,dim3,dim4,dim5};
    return putDataSliceInObject(obj, path, idx, INT, 5, dim, (void *)data);
}

void *idamPutVect5DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4, int dim5)
{
    int dim[5] = {dim1,dim2,dim3,dim4,dim5};
    return putDataSliceInObject(obj, path, idx, FLOAT, 5, dim, (void *)data);
}

void *idamPutVect5DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4, int dim5)
{
    int dim[5] = {dim1,dim2,dim3,dim4,dim5};
    return putDataSliceInObject(obj, path, idx, DOUBLE, 5, dim, (void *)data);
}

void *idamPutVect6DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6)
{
    int dim[6] = {dim1,dim2,dim3,dim4,dim5,dim6};
    return putDataSliceInObject(obj, path, idx, INT, 6, dim, (void *)data);
}

void *idamPutVect6DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6)
{
    int dim[6] = {dim1,dim2,dim3,dim4,dim5,dim6};
    return putDataSliceInObject(obj, path, idx, FLOAT, 6, dim, (void *)data);
}

void *idamPutVect6DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6)
{
    int dim[6] = {dim1,dim2,dim3,dim4,dim5,dim6};
    return putDataSliceInObject(obj, path, idx, DOUBLE, 6, dim, (void *)data);
}

void *idamPutVect7DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7)
{
    int dim[7] = {dim1,dim2,dim3,dim4,dim5,dim6,dim7};
    return putDataSliceInObject(obj, path, idx, INT, 7, dim, (void *)data);
}

void *idamPutVect7DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7)
{
    int dim[7] = {dim1,dim2,dim3,dim4,dim5,dim6,dim7};
    return putDataSliceInObject(obj, path, idx, FLOAT, 7, dim, (void *)data);
}

void *idamPutVect7DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7)
{
    int dim[7] = {dim1,dim2,dim3,dim4,dim5,dim6,dim7};
    return putDataSliceInObject(obj, path, idx, DOUBLE, 7, dim, (void *)data);
}

void *idamPutObjectInObject(void *obj, char *path, int idx, void *data)
{
// void *mdsPutObjectInObject(void *obj, char *path, int idx, void *dataObj)
//    return obj;	

   PUTDATA_BLOCK putData;
   initIdamPutDataBlock(&putData);
   
// Pass the Data

   int *objId = (int *)malloc(2*sizeof(int));	// Pass two object references   
   objId[0] = ((int *)obj)[0];
   objId[1] = ((int *)data)[0];

   putData.data_type = TYPE_INT; 
   putData.count = 2;
   putData.rank  = 1;
   putData.shape = NULL;   
   putData.data  = (void *)objId;  
   
   int lstr = DIRECTIVELENGTH;
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::putObjectInObject(index=%d, path='%s', %s)", idx, path, keyword); 
   
   int handle = idamPutAPI(directive, &putData);   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error putObjectInObject: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return NULL;
   }
   
   int *refId = (int *)getIdamData(handle);	// Returned Object Reference   
   if(refId == NULL){
      sprintf(errmsg, "Error putObjectInObject: Object not returned");
      idamFree(handle);
      return NULL;
   }      

// Detach heap from the IDAM structure then free the IDAM structure (avoid data leaks within IDAM)

   getIdamDataBlock(handle)->data = NULL;      
   idamFree(handle);	 
   
   return refId;
}

int idamGetObjectDim(void *obj)
{
// int mdsGetObjectDim(void *obj)
// *** error - obj is a reference on the client. The obj only exists on the server. 
//  return ((obj_t *)obj)->dim;
        
   PUTDATA_BLOCK putData;
   initIdamPutDataBlock(&putData);
   
// Pass the Data

   int *objId = (int *)malloc(sizeof(int));   
   objId[0] = ((int *)obj)[0];

   putData.data_type = TYPE_INT; 
   putData.count = 1;
   putData.rank  = 0;
   putData.shape = NULL;   
   putData.data  = (void *)objId;  
   
   int lstr = DIRECTIVELENGTH;
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::getObjectDim(%s)", keyword);
   
   int handle = idamPutAPI(directive, &putData);   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error GETing Object Dim: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   int data = -1;
   
   if(getIdamData(handle) != NULL && getIdamDataType(handle) == TYPE_INT) data = *((int *)getIdamData(handle));
   
   idamFree(handle);	// Application is responsible for freeing data  
   
   return data;
}
    

/** Get the dimensions of a field by calling the regular object read function
 *  but limiting it to the size part (thanks to the type DIMENSION) */
int idamGetDimensionFromObject(int expIdx, void *obj, char *path, int idx, int *nDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dims[7];
    int status;
    int *tmp; //we cannot use data directly for scalars because getDataSliceFromObject returns a pointer on pointer
    status = getDataSliceFromObject(obj,path,idx,DIMENSION,7,dims,(void **)&tmp);
    if (!status) {
        if (tmp==NULL)  // the field could not be read
            *nDims = 0;
        else {
            *nDims = *tmp;
            free(tmp);
        }
        *dim1 = dims[0];
        *dim2 = dims[1];
        *dim3 = dims[2];
        *dim4 = dims[3];
        *dim5 = dims[4];
        *dim6 = dims[5];
        *dim7 = dims[6];
    }
    return status;
}

int idamGetStringFromObject(void *obj, char *path, int idx, char **data)
{
    int dims[1];
    return getDataSliceFromObject(obj,path,idx,STRING,0,dims,(void **)data);

/*
    int dims[1];
    int status;
    char **tmp; //we cannot use data directly because getDataSliceFromObject returns a pointer on pointer
    status = getDataSliceFromObject(obj,idamPath,idx,STRING,0,dims,(void **)&tmp);
    if (!status) {
        *data = *tmp;
        // free(tmp);		// dgm: Freed in ids get 
    }
    return status;
*/    
}

int idamGetFloatFromObject(void *obj, char *path, int idx, float *data)
{
    int dims[1];
    int status;
    float *tmp; //we cannot use data directly for scalars because getDataSliceFromObject returns a pointer on pointer
    status = getDataSliceFromObject(obj,path,idx,FLOAT,0,dims,(void **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int idamGetIntFromObject(void *obj, char *path, int idx, int *data)
{
    int dims[1];
    int status;
    int *tmp; //we cannot use data directly for scalars because getDataSliceFromObject returns a pointer on pointer
    status = getDataSliceFromObject(obj,path,idx,INT,0,dims,(void **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int idamGetDoubleFromObject(void *obj, char *path, int idx, double *data)
{
    int dims[1];
    int status;
    double *tmp; //we cannot use data directly for scalars because getDataSliceFromObject returns a pointer on pointer
    status = getDataSliceFromObject(obj,path,idx,DOUBLE,0,dims,(void **)&tmp);
    if (!status) {
        *data = *tmp;
        free(tmp);
    }
    return status;
}

int idamGetVect1DStringFromObject(void *obj, char *idamPath, int idx, char  ***data, int *dim)
{
    return getDataSliceFromObject(obj,idamPath,idx,STRING,1,dim,(void **)data);
}

int idamGetVect1DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim)
{
    return getDataSliceFromObject(obj,idamPath,idx,INT,1,dim,(void **)data);
}

int idamGetVect1DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim)
{
    return getDataSliceFromObject(obj,idamPath,idx,FLOAT,1,dim,(void **)data);
}

int idamGetVect1DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim)
{
    return getDataSliceFromObject(obj,idamPath,idx,DOUBLE,1,dim,(void **)data);
}

int idamGetVect2DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2)
{
    int dim[2];
    int status = getDataSliceFromObject(obj,idamPath,idx,INT,2,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    return status;
}

int idamGetVect2DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2)
{
    int dim[2];
    int status = getDataSliceFromObject(obj,idamPath,idx,FLOAT,2,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    return status;
}

int idamGetVect2DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2)
{
    int dim[2];
    int status = getDataSliceFromObject(obj,idamPath,idx,DOUBLE,2,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    return status;
}

int idamGetVect3DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3)
{
    int dim[3];
    int status = getDataSliceFromObject(obj,idamPath,idx,INT,3,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    return status;
}

int idamGetVect3DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3)
{
    int dim[3];
    int status = getDataSliceFromObject(obj,idamPath,idx,FLOAT,3,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    return status;
}

int idamGetVect3DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3)
{
    int dim[3];
    int status = getDataSliceFromObject(obj,idamPath,idx,DOUBLE,3,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    return status;
}

int idamGetVect4DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dim[4];
    int status = getDataSliceFromObject(obj,idamPath,idx,INT,4,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    return status;
}

int idamGetVect4DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dim[4];
    int status = getDataSliceFromObject(obj,idamPath,idx,FLOAT,4,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    return status;
}

int idamGetVect4DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
    int dim[4];
    int status = getDataSliceFromObject(obj,idamPath,idx,DOUBLE,4,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    return status;
}

int idamGetVect5DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int dim[5];
    int status = getDataSliceFromObject(obj,idamPath,idx,INT,5,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    return status;
}

int idamGetVect5DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int dim[5];
    int status = getDataSliceFromObject(obj,idamPath,idx,FLOAT,5,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    return status;
}

int idamGetVect5DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
    int dim[5];
    int status = getDataSliceFromObject(obj,idamPath,idx,DOUBLE,5,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    return status;
}

int idamGetVect6DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
    int dim[6];
    int status = getDataSliceFromObject(obj,idamPath,idx,INT,6,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    return status;
}

int idamGetVect6DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
    int dim[6];
    int status = getDataSliceFromObject(obj,idamPath,idx,FLOAT,6,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    return status;
}

int idamGetVect6DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
    int dim[6];
    int status = getDataSliceFromObject(obj,idamPath,idx,DOUBLE,6,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    return status;
}

int idamGetVect7DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dim[7];
    int status = getDataSliceFromObject(obj,idamPath,idx,INT,7,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    *dim7 = dim[6];
    return status;
}

int idamGetVect7DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dim[7];
    int status = getDataSliceFromObject(obj,idamPath,idx,FLOAT,7,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    *dim7 = dim[6];
    return status;
}

int idamGetVect7DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int dim[7];
    int status = getDataSliceFromObject(obj,idamPath,idx,DOUBLE,7,dim,(void **)data);
    *dim1 = dim[0];
    *dim2 = dim[1];
    *dim3 = dim[2];
    *dim4 = dim[3];
    *dim5 = dim[4];
    *dim6 = dim[5];
    *dim7 = dim[6];
    return status;
}

int idamPutObjectSlice(int expIdx, char *cpoPath, char *path, double time, void *obj)
{
    // Since all data has been written along the way by
    // the idamPut...InObject functions, then there is nothing left
    // to do except freeing the memory
//    idamReleaseObject(obj);    
// int mdsPutObjectSlice(int expIdx, char *cpoPath, char *path, double time, void *obj)    

// PUTDATA variable   
    
   PUTDATA_BLOCK putData;
   initIdamPutDataBlock(&putData);

// Pass the Data

   putData.data_type = TYPE_DOUBLE; 
   putData.rank  = 0;
   putData.count = 1;
   putData.shape = NULL;
   putData.data  = (void *) &time;
       
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::putObjectSlice(idx=%d, cpoPath='%s', path='%s', objectId=%d, %s)", expIdx, cpoPath, path, ((int *)obj)[0], keyword);
       
   int handle = idamPutAPI(directive, &putData);  

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error putObjectSlice: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   int *data = (int *)getIdamData(handle);	// Returned status code
   
   if(data == NULL){
      sprintf(errmsg, "Error putObjectSlice: No returned status code");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   int rc = *data;
   
   if(rc != OK_RETURN_VALUE){
      sprintf(errmsg, "Error putObjectSlice: release failed");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

   idamFree(handle);	 
   
   return OK_RETURN_VALUE;
}

int idamReplaceLastObjectSlice(int expIdx, char *cpoPath, char *path, void *obj)
{
    // Since all data has been written along the way by
    // the idamPut...InObject functions, then there is nothing left
    // to do except freeing the memory
//    idamReleaseObject(obj);
    
// int mdsReplaceLastObjectSlice(int expIdx, char *cpoPath, char *path, void *obj)    
       
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::replaceLastObjectSlice(idx=%d, cpoPath='%s', path='%s', objectId=%d, %s)", expIdx, cpoPath, path, ((int *)obj)[0], keyword);
       
   int handle = idamGetAPI(directive, "");  

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error putObjectSlice: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   int *data = (int *)getIdamData(handle);	// Returned status code
   
   if(data == NULL){
      sprintf(errmsg, "Error putObjectSlice: No returned status code");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }
   
   int rc = *data;
   
   if(rc != OK_RETURN_VALUE){
      sprintf(errmsg, "Error putObjectSlice: release failed");
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

   idamFree(handle);	 
   
   return OK_RETURN_VALUE;
}


// dgm =================== MISSING - undefined references!!! ================================

int idambeginIdsGet(int expIdx, char *path, int isTimed, int *retSamples) {
   
   *retSamples = 0;
   
// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

// status = mdsbeginIdsGet(currIdx, path, isTimed, retSamples);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::beginIdsGet(idx=%d, path=%s, isTimed=%d, /imas_mds)", expIdx, path, isTimed);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error beginIdsGet: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return ERROR_RETURN_VALUE;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL && getIdamDataType(handle) == TYPE_INT && getIdamDataNum(handle) == 1){
      *retSamples = data[0];
      status = OK_RETURN_VALUE;
   }   
   
// Free heap

   idamFree(handle); 
 
   return status;
}
int idamendIdsGet(int expIdx, char *path) {

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

// int mdsendIdsGet(int expIdx, char *path)
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::endIdsGet(idx=%d, path=%s, /imas_mds)", expIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error endIdsGet: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}

int idambeginIdsPut(int expIdx, char *path) {
// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

// status = mdsbeginIdsPut(currIdx, path);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::beginIdsPut(idx=%d, path=%s, /imas_mds)", expIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error beginIdsPut: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}

int idamendIdsPut(int expIdx, char *path) {

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;
// status = mdsendIdsPut(currIdx, path);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::endIdsPut(idx=%d, path=%s, /imas_mds)", expIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error beginIdsPut: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}

int idambeginIdsGetSlice(int expIdx, char *path, double time) {

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

//status = mdsbeginIdsGetSlice(currIdx, path, time);

// Create PUTDATA variable for the time argument   
    
   PUTDATA_BLOCK putData;
   initIdamPutDataBlock(&putData);
   
// Pass the Data

   int status = ERROR_RETURN_VALUE;
   
   putData.data_type = TYPE_DOUBLE;
   putData.count = 1; 
   putData.data  = (void *) &time;
   
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::beginIdsGetSlice(idx=%d, path='%s', /imas_mds)", expIdx, path);
   
   int handle = idamPutAPI(directive, &putData);   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error beginIdsGetSlice: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}
int idamendIdsGetSlice(int currIdx, char *path){

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

//status = mdsendIdsGetSlice(currIdx, path);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::endIdsGetSlice(idx=%d, path=%s, /imas_mds)", currIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error endIdsGetSlice: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}

int idambeginIdsPutSlice(int expIdx, char *path){

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

// status = mdsbeginIdsPutSlice(currIdx, path);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::beginIdsPutSlice(idx=%d, path=%s, /imas_mds)", expIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error beginIdsPutSlice: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}
int idamendIdsPutSlice(int expIdx, char *path){

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

// status = mdsendIdsPutSlice(currIdx, path);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::endIdsPutSlice(idx=%d, path=%s, /imas_mds)", expIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error endIdsPutSlice: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}

int idamendIdsPutNonTimed(int expIdx, char *path){

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

// status = mdsendIdsPutNonTimed(currIdx, path);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::endIdsPutNonTimed(idx=%d, path=%s, /imas_mds)", expIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error endIdsPutNonTimed: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}
int idambeginIdsPutNonTimed(int expIdx, char *path){

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

// status = mdsbeginIdsPutNonTimed(currIdx, path);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::beginIdsPutNonTimed(idx=%d, path=%s, /imas_mds)", expIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error beginIdsPutNonTimed: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}

int idamendIdsReplaceLastSlice(int expIdx, char *path){

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

// status = mdsendIdsReplaceLastSlice(currIdx, path);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::endIdsReplaceLastSlice(idx=%d, path=%s, /imas_mds)", expIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error endIdsReplaceLastSlice: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}
int idambeginIdsReplaceLastSlice(int expIdx, char *path){

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

// status = mdsbeginIdsReplaceLastSlice(currIdx, path);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::beginIdsReplaceLastSlice(idx=%d, path=%s, /imas_mds)", expIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error beginIdsReplaceLastSlice: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}

int idambeginIdsPutTimed(int expIdx, char *path, int samples, double *inTimes){

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

// status = mdsbeginIdsPutTimed(currIdx, path, samples, inTimes);

// Create PUTDATA variable for the time argument   
    
   PUTDATA_BLOCK putData;
   initIdamPutDataBlock(&putData);
   
// Pass the Data

   int status = ERROR_RETURN_VALUE;
   
   putData.data_type = TYPE_DOUBLE;
   putData.count = samples;
   putData.rank  = 1; 
   putData.data  = (void *) inTimes;
   
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));
   char *keyword = imas_getKeyword();
   
   sprintf(directive, "imas::beginIdsPutTimed(idx=%d, path='%s', /imas_mds)", expIdx, path);
   
   int handle = idamPutAPI(directive, &putData);   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error beginIdsPutTimed: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}
int idamendIdsPutTimed(int expIdx, char *path){

// Only proceed if known to be mdsplus
   if(getenv("IDAM_NOHDF5_IMAS_PLUGIN") == NULL) return 1;

//status = mdsendIdsPutTimed(currIdx, path);
   
   int status = ERROR_RETURN_VALUE;
   int lstr = DIRECTIVELENGTH + strlen(path);
   char *directive = (char *)malloc(lstr*sizeof(char));   
   
   sprintf(directive, "imas::endIdsPutTimed(idx=%d, path=%s, /imas_mds)", expIdx, path);   
   
   int handle = idamGetAPI(directive, "");   

   if(directive) free(directive);
   
   int err = 0;
   
   if(handle < 0 || (err=getIdamErrorCode(handle)) != 0){      
      sprintf(errmsg, "Error endIdsPutTimed: %s", getIdamServerErrorMsg(handle));
      idamFree(handle);
      return status;
   }

   int *data = (int *)getIdamData(handle);
   if(data != NULL) status = data[0];
   
// Free heap

   idamFree(handle); 
 
   return status;
}
