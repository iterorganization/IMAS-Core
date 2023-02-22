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
  };

class AccessLayerPluginManager: public provenance_plugin_feature {

private:
    //std::map<std::string, std::vector<struct plugin_info>> get_operations;
    void write_field(int ctxID, const std::string &field, const std::string &value);

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
    //void update_get_operations(int ctxID, const std::string &field);

};

#endif
