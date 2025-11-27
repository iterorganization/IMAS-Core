#ifndef PARAMETER_PLUGIN_H
#define PARAMETER_PLUGIN_H 1

#include "al_reader_helper_plugin.h"
#include "access_layer_plugin.h"

class ParameterPlugin: public AL_reader_helper_plugin
{ 
  private:
    int op;

  public:
    ParameterPlugin();
    ~ParameterPlugin();

    std::string getName() override;
    std::string getDescription() override;
    std::string getVersion() override;
    plugin::OPERATION node_operation(const std::string &path) override;

    void setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) override;

};

extern "C" access_layer_plugin* create() {
  return new ParameterPlugin;
}
extern "C" void destroy (access_layer_plugin* al_plugin) {
  delete al_plugin;
}
#endif

