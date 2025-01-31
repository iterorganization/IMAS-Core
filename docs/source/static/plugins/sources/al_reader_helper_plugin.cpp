#include "al_reader_helper_plugin.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

AL_reader_helper_plugin::AL_reader_helper_plugin()
:pulseCtx(-1), ctx(-1), dataobjectname(), occurrence(-1), mode(-1), time(-1), interp(-1)
{
}

AL_reader_helper_plugin::~AL_reader_helper_plugin()
{
}

std::string AL_reader_helper_plugin::getName() {
    throw ALPluginException("Name of the plugin should be defined in user code!");
}

std::string AL_reader_helper_plugin::getDescription() {
    throw ALPluginException("Description of the plugin should be defined in user code!");
}

std::string AL_reader_helper_plugin::getVersion() {
    throw ALPluginException("Version of the plugin should be defined in user code!");
}

plugin::OPERATION AL_reader_helper_plugin::node_operation(const std::string &path) {
    throw ALPluginException("plugin node_operation(path) should be defined in user code!"); 
}

std::string AL_reader_helper_plugin::getCommit() {
    return "8f2e7cd64daf9e35a6e6c5850dd80fc198f11d86";
}

std::string AL_reader_helper_plugin::getRepository() {
    return "ssh://git@git.iter.org/imas/access-layer-plugins.git";
}
std::string AL_reader_helper_plugin::getParameters() {
    return "";
}

std::string AL_reader_helper_plugin::getReadbackName(const std::string &path, int* index) {
       return "";
}
std::string AL_reader_helper_plugin::getReadbackDescription(const std::string &path) {
   return "";
}
std::string AL_reader_helper_plugin::getReadbackCommit(const std::string &path) {
   return "";
}
std::string AL_reader_helper_plugin::getReadbackVersion(const std::string &path) {
   return "";
}
std::string AL_reader_helper_plugin::getReadbackRepository(const std::string &path) {
   return "";
}
std::string AL_reader_helper_plugin::getReadbackParameters(const std::string &path) {
   return "";
}

void AL_reader_helper_plugin::setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) {
}

void AL_reader_helper_plugin::begin_global_action(int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) {

    this->pulseCtx = pulseCtx;
    this->dataobjectname = std::string(dataobjectname);
    this->mode = mode;
    this->ctx = opCtx;
    
    std::size_t found = std::string(dataobjectname).find("/");
    if (found != std::string::npos) {
        std::string occStr = std::string(dataobjectname).substr(found+1, std::string::npos);
        this->occurrence = std::stoi(occStr);
    }
    else {
        this->occurrence = 0;
    }
}

void AL_reader_helper_plugin::begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) {
    this->time = time;
    this->interp = interp;
    begin_global_action(pulseCtx, dataobjectname, "", mode, opCtx);
}

void AL_reader_helper_plugin::begin_arraystruct_action(int ctx, int *aosctx, const char* fieldPath, const char* timeBasePath, int *arraySize) {
    LLenv lle = Lowlevel::getLLenv(ctx);
    ArraystructContext* actx = lle.create(fieldPath, timeBasePath);
    *aosctx = Lowlevel::addLLenv(lle.backend, actx); 
    lle.backend->beginArraystructAction(actx, arraySize);
}

int AL_reader_helper_plugin::read_data(int ctx, const char* fieldPath, const char* timeBasePath, 
                void **data, int datatype, int dim, int *size) {
    return 0;
}

void AL_reader_helper_plugin::write_data(int ctx, const char* fieldPath, const char* timeBasePath,
                void *data, int datatype, int dim, int *size) {
}

void AL_reader_helper_plugin::end_action(int ctx) {}
