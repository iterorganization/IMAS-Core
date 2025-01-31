#ifndef PATCH_READER_PLUGIN_H
#define PATCH_READER_PLUGIN_H 1

#include "al_reader_helper_plugin.h"
#include "access_layer_plugin.h"

class PatchReaderPlugin: public AL_reader_helper_plugin
{ 
  private:
    int op;

  public:
    PatchReaderPlugin();
    ~PatchReaderPlugin();

     std::string getName() override;
     std::string getDescription() override;
     std::string getVersion() override;
     plugin::OPERATION node_operation(const std::string &path) override;

    void setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) override;
    int read_data(int ctx, const char* fieldPath, const char* timeBasePath, void **data, int datatype, int dim, int *size) override;

};

extern "C" access_layer_plugin* create() {
  return new PatchReaderPlugin;
}
extern "C" void destroy (access_layer_plugin* al_plugin) {
  delete al_plugin;
}
#endif

