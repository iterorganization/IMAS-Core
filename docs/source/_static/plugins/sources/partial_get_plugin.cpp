#include "partial_get_plugin.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "simple_logger.h"

PartialGetPlugin::PartialGetPlugin()
{
}

PartialGetPlugin::~PartialGetPlugin()
{
}

std::string PartialGetPlugin::getName() {
    return "partial_get";
}

std::string PartialGetPlugin::getDescription() {
    return "";
}

std::string PartialGetPlugin::getCommit() {
    return "d92bb2f30384bc618574508b56e9542d00f0e97a";
}

std::string PartialGetPlugin::getVersion() {
    return "1.0.0";
}

std::string PartialGetPlugin::getRepository() {
    return "ssh://git@git.iter.org/imas/access-layer.git";
}
std::string PartialGetPlugin::getParameters() {
    return "";
}

plugin::OPERATION PartialGetPlugin::node_operation(const std::string &path) {
    return plugin::OPERATION::GET_ONLY;
}

std::string PartialGetPlugin::getReadbackName(const std::string &path, int* index) {
    return "";
}

std::string PartialGetPlugin::getReadbackDescription(const std::string &path) {
    return "";
}

std::string PartialGetPlugin::getReadbackCommit(const std::string &path) {
    return "";
}
std::string PartialGetPlugin::getReadbackVersion(const std::string &path) {
    return "";
}
std::string PartialGetPlugin::getReadbackRepository(const std::string &path) {
    return "";
}
std::string PartialGetPlugin::getReadbackParameters(const std::string &path) {
    return "";
}

void PartialGetPlugin::setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) {
}

void PartialGetPlugin::begin_global_action(int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) {

}

void PartialGetPlugin::begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) {
    throw ALPluginException("PartialGetPlugin: slice operation not supported", LOG);
}

void PartialGetPlugin::begin_arraystruct_action(int ctx, int *aosctx, const char* fieldPath, const char* timeBasePath, int *arraySize) {
    
  LLenv lle = Lowlevel::getLLenv(ctx);
  ArraystructContext* actx = lle.create(fieldPath, timeBasePath);
  *aosctx = Lowlevel::addLLenv(lle.backend, actx); 
  lle.backend->beginArraystructAction(actx, arraySize);
  if (std::string(fieldPath) == "flux_loop") {
     LOG_WARNING << "ignoring flux_loop";
     *arraySize = 0; 
  } 
}

int PartialGetPlugin::read_data(int ctx, const char* fieldPath, const char* timeBasePath, 
                void **data, int datatype, int dim, int *size) {
   return 0;
}


void PartialGetPlugin::write_data(int ctx, const char* fieldPath, const char* timeBasePath, void *data, int datatype, int dim, int *size) {
	throw ALPluginException("PartialGetPlugin: write_data not supported", LOG);
}

void PartialGetPlugin::end_action(int ctx) {}
