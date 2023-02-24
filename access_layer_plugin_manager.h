#ifndef AL_PLUGIN_MANAGER_H
#define AL_PLUGIN_MANAGER_H 1

#include <string>
#include <map>

#include "provenance_plugin_feature.h"

   struct plugin_info{
      std::string name;
      std::string commit;
      std::string version;
      std::string repository;
      std::string parameters;
      int application_index;
  };

class AccessLayerPluginManager: public provenance_plugin_feature {

private:
    void write_field(int ctxID, const std::string &field, const std::string &value);
    void addReadbackPlugin(const std::string &plugin_name, const std::string &path);
    static bool sortPlugins (const plugin_info &p, const plugin_info &q);

protected:
    virtual void write_plugins_infrastructure_infos(int ctxID);

public:
    AccessLayerPluginManager();
    ~AccessLayerPluginManager();

    virtual std::string getName();
    virtual std::string getCommit();
    virtual std::string getVersion();
    virtual std::string getRepository();
    virtual std::string getParameters();

    virtual void write_plugins_metadata(int ctxID);
    virtual void bind_readback_plugins(int ctxID);
    virtual void unbind_readback_plugins(int ctxID);

    virtual void begin_arraystruct_action_handler(const std::string &plugin_name, int ctxID, int *actxID, 
        const char* fieldPath, const char* timeBasePath, int *arraySize);
    virtual int read_data_plugin_handler(const std::string &plugin_name, int ctxID, const char* fieldPath, const char* timeBasePath, 
        void **data, int datatype, int dim, int *size);
    virtual void write_data_plugin_handler(const std::string &plugin_name, int ctxID, const char *field, const char *timebase, 
              void *data, int datatype, int dim, int *size);
    virtual void end_action_plugin_handler(int ctxID);

    int getAccessmode(int ctxID);
    bool skipWriteAccess(int ctxID, const char* fieldPath);

};

#endif
