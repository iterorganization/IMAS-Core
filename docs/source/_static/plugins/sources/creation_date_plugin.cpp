#include "creation_date_plugin.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

Creation_date_plugin::Creation_date_plugin()
{
}

Creation_date_plugin::~Creation_date_plugin()
{
}

/**
The following functions are used to provide metadata about the plugin to identify and manage the plugin.

getName() returns the name of the plugin, which is "creation_date" in this example.
getCommit() returns a unique identifier for the version of the plugin, which is "8f2e7cd64daf9e35a6e6c5850dd80fc198f11d86" in this example.
getVersion() returns the version number of the plugin, which is "1.0.0" in this example.
getRepository() returns the URI for the Git repository where the plugin is located, which is "ssh://git@git.iter.org/imas/access-layer-plugins.git" in this example.
getParameters() returns a string representing the parameters for the plugin.
*/

std::string Creation_date_plugin::getName() {
    return "creation_date";
}

std::string Creation_date_plugin::getDescription() {
    return "";
}

std::string Creation_date_plugin::getCommit() {
    return "8f2e7cd64daf9e35a6e6c5850dd80fc198f11d86";
}

std::string Creation_date_plugin::getVersion() {
    return "1.0.0";
}
std::string Creation_date_plugin::getRepository() {
    return "ssh://git@git.iter.org/imas/access-layer-plugins.git";
}
std::string Creation_date_plugin::getParameters() {
    return "creation_date plugin parameters";
}

/**
The following functions are used by the Access Layer to determine which readback plugin to use for a given field path.

The getReadbackName() function returns the name of the readback plugin to use for a given field path. If the field path contains the string "creation_date", it returns the string "debug". Otherwise, it returns an empty string.

The getReadbackCommit() function returns the commit hash of the readback plugin to use for a given field path. If the field path contains the string "creation_date", it returns a specific commit hash. Otherwise, it returns an empty string.

The getReadbackVersion() function returns the version of the readback plugin to use for a given field path. If the field path contains the string "creation_date", it returns the string "1.1.0". Otherwise, it returns an empty string.

The getReadbackRepository() function returns the repository location of the readback plugin to use for a given field path. If the field path contains the string "creation_date", it returns a specific repository location. Otherwise, it returns an empty string.

The getReadbackParameters() function returns the parameters to pass to the readback plugin for a given field path. If the field path contains the string "creation_date", it returns a specific string. Otherwise, it returns an empty string.
*/

std::string Creation_date_plugin::getReadbackName(const std::string &path, int* index) {
       return "";
}

std::string Creation_date_plugin::getReadbackDescription(const std::string &path) {
       return "";
}

std::string Creation_date_plugin::getReadbackCommit(const std::string &path) {
    if (path.rfind("creation_date"))
       return "d92bb2f30384bc618574508b56e9542d00f0e97a";
    return "";
}

std::string Creation_date_plugin::getReadbackVersion(const std::string &path) {
    if (path.rfind("creation_date"))
       return "1.1.0";
    return "";
}

std::string Creation_date_plugin::getReadbackRepository(const std::string &path) {
    if (path.rfind("creation_date"))
       return "ssh://git@git.iter.org/imas/access-layer-plugins.git";
    return "";
}

std::string Creation_date_plugin::getReadbackParameters(const std::string &path) {
    if (path.rfind("creation_date"))
       return "readback plugin parameters";
    return "";
}

plugin::OPERATION Creation_date_plugin::node_operation(const std::string &path) {
    return plugin::OPERATION::PUT_ONLY;
}

void Creation_date_plugin::begin_global_action(int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) {
}

void Creation_date_plugin::begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) {
}

void Creation_date_plugin::begin_arraystruct_action(int ctx, int *aosctx, const char* fieldPath, const char* timeBasePath, int *arraySize) {
     LLenv lle = Lowlevel::getLLenv(ctx);
     ArraystructContext* actx = lle.create(fieldPath, timeBasePath);
     *aosctx = Lowlevel::addLLenv(lle.backend, actx); 
     lle.backend->beginArraystructAction(actx, arraySize);
}

int Creation_date_plugin::read_data(int ctx, const char* fieldPath, const char* timeBasePath, 
                void **data, int datatype, int dim, int *size) {
    return 0;
}


/**
 This function is used to write data to the specific IDS field 'ids_properties/creation_date'.

The function takes several parameters, including an integer "ctx", which is a handle to the current low level context, a character array "fieldPath" representing the path to the field being modified, a character array "timeBasePath" representing the path to the timebase for the field, a void pointer "data" containing the data to be written, an integer "datatype" specifying the type of data being written, an integer "dim" specifying the number of dimensions of the data, and an array of integers "size" specifying the size of each dimension.

The function prints a message indicating that the "Creation_date" plugin is patching the creation date. It then uses the standard library function "std::time" to obtain the current system time and local time, and formats it using the standard library function "std::put_time" to create a string representing the current date in the format of "YYYY-MM-DD". This string is stored in a std::ostringstream object called "oss" and then converted to a C-style string using the "str()" function, and then to a void pointer using the "c_str()" function.

Finally, the function creates an array of sizes for the data being written, with a single element representing the length of the string. It then calls the "al_plugin_write_data" function to write the data to the specified field, passing in the current context, the field path, the timebase path, the void pointer to the data, the data type, the number of dimensions, and the array of sizes. After writing the data, the function prints a message indicating that the patching is complete.
*/
void Creation_date_plugin::write_data(int ctx, const char* fieldPath, const char* timeBasePath, void *data, int datatype, int dim, int *size) {
    printf("Patching creation_date from the creation_date plugin... \n");
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    auto text = oss.str(); 
    void* ptrData = (void *) (text.c_str());
    int arrayOfSizes[1] = {(int)text.size()};
    al_plugin_write_data(ctx, fieldPath, timeBasePath, ptrData, CHAR_DATA, 1, arrayOfSizes);
    printf("End of patching.\n");
}

void Creation_date_plugin::setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) {
}

void Creation_date_plugin::end_action(int ctx) {}
