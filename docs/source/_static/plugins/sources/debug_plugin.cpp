#include "debug_plugin.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "simple_logger.h"

Debug_plugin::Debug_plugin()
{
}

Debug_plugin::~Debug_plugin()
{
}

std::string Debug_plugin::getName() {
    return "debug";
}

std::string Debug_plugin::getDescription() {
    return "";
}

std::string Debug_plugin::getCommit() {
    return "d92bb2f30384bc618574508b56e9542d00f0e97a";
}

std::string Debug_plugin::getVersion() {
    return "1.1.0";
}
std::string Debug_plugin::getRepository() {
    return "ssh://git@git.iter.org/imas/access-layer-plugins.git";
}
std::string Debug_plugin::getParameters() {
    return "";
}

std::string Debug_plugin::getReadbackName(const std::string &path, int* index) {
    return "";
}

std::string Debug_plugin::getReadbackDescription(const std::string &path) {
    return "";
}

std::string Debug_plugin::getReadbackCommit(const std::string &path) {
    return "";
}

std::string Debug_plugin::getReadbackVersion(const std::string &path) {
    return "";
}

std::string Debug_plugin::getReadbackRepository(const std::string &path) {
    return "";
}

std::string Debug_plugin::getReadbackParameters(const std::string &path) {
    return "";
}

plugin::OPERATION Debug_plugin::node_operation(const std::string &path) {
    return plugin::OPERATION::GET_ONLY;
}

void Debug_plugin::setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) {
}

void Debug_plugin::begin_global_action(int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) {
}

void Debug_plugin::begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) {
}

void Debug_plugin::begin_arraystruct_action(int ctx, int *aosctx, const char* fieldPath, const char* timeBasePath, int *arraySize) {
    LLenv lle = Lowlevel::getLLenv(ctx);
    ArraystructContext* actx = lle.create(fieldPath, timeBasePath);
    *aosctx = Lowlevel::addLLenv(lle.backend, actx); 
    lle.backend->beginArraystructAction(actx, arraySize); //calling backend
    if (*arraySize != 0)
       printf("visiting AOS=%s with size=%d\n", fieldPath, *arraySize);  
}

/**
This is an implementation of the read_data function for the Debug_plugin class.

The function takes in a ctx integer representing the LL context of the data read, fieldPath and timeBasePath character arrays representing the path to the data field and the time base path, respectively. It also takes in a void **data pointer to a pointer to the data to be read, an integer datatype representing the type of data being read, an integer dim representing the number of dimensions of the data, and an array of integers size representing the size of the data in each dimension.

The function first calls the al_plugin_read_data function to read the data from the given field path and time base path. If the datatype is CHAR_DATA, it logs the field path and the string data to the debug log using the LOG_DEBUG macro.

Finally, the function returns 1, indicating that the read_data operation has modified the data pointer.
*/

int Debug_plugin::read_data(int ctx, const char* fieldPath, const char* timeBasePath, 
                void **data, int datatype, int dim, int *size) {
	al_status_t status = al_plugin_read_data(ctx, fieldPath, timeBasePath, data, datatype, dim, size);
	if (status.code != 0) {
            throw ALPluginException("Error calling al_plugin_read_data in debug plugin.");
        }
	if (datatype == CHAR_DATA) {
		LOG_DEBUG << "reading data for field path= " << fieldPath;
		char buff[100];
		snprintf(buff, sizeof(buff), "%s", (char*) *data);
		std::string buffAsStdStr = buff;
		LOG_DEBUG << "ids_properties/version_put/access_layer= " << buffAsStdStr;
	}
	else {
		LOG_DEBUG << "debug plugin prints only STRING data" << fieldPath;
	}
    return 1; 
}


void Debug_plugin::write_data(int ctx, const char* fieldPath, const char* timeBasePath, void *data, int datatype, int dim, int *size) {
}

void Debug_plugin::end_action(int ctx) {}

