#ifndef DEBUG_PLUGIN_H
#define DEBUG_PLUGIN_H 1

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>

#include "access_layer_plugin.h"


class Debug_plugin: public access_layer_plugin
{

  private:

  public:
    Debug_plugin();
    ~Debug_plugin();
    
     std::string getName() override;
     std::string getDescription() override;
     std::string getCommit() override;
     std::string getVersion() override;
     std::string getRepository() override;
     std::string getParameters() override;
     plugin::OPERATION node_operation(const std::string &path) override;
     std::string getReadbackName(const std::string &path, int* index) override;
     std::string getReadbackDescription(const std::string &path) override;
     std::string getReadbackCommit(const std::string &path) override;
     std::string getReadbackVersion(const std::string &path) override;
     std::string getReadbackRepository(const std::string &path) override;
     std::string getReadbackParameters(const std::string &path) override;
     void setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) override;    
     void begin_arraystruct_action(int ctx, int *aosctx, const char* fieldPath, const char* timeBasePath, int *arraySize) override;
     void begin_global_action(int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) override;
     void begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) override;
     void end_action(int ctx) override;
     int read_data(int ctx, const char* fieldPath, const char* timeBasePath, void **data, int datatype, int dim, int *size) override;
     void write_data(int ctx, const char* fieldPath, const char* timeBasePath, void *data, int datatype, int dim, int *size) override;
};

extern "C" access_layer_plugin* create() {
  return new Debug_plugin;
}

extern "C" void destroy (access_layer_plugin* al_plugin) {
  delete al_plugin;
}

#endif
