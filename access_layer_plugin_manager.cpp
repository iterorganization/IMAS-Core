#include "access_layer_plugin_manager.h"

#include <string>
#include "access_layer_plugin.h"

#define PLUGIN_MANAGER_NAME "AccessLayerPluginManager"
#define PLUGIN_MANAGER_VERSION "1.0.0"
#define PLUGIN_MANAGER_REPOSITORY "ssh://git@git.iter.org/imas/access-layer.git"


   AccessLayerPluginManager::AccessLayerPluginManager() {}
   AccessLayerPluginManager::~AccessLayerPluginManager() {}

    std::string AccessLayerPluginManager::getName()
    {
        return std::string(PLUGIN_MANAGER_NAME);
    }

    std::string AccessLayerPluginManager::getCommit()
    {
        return "0";
    }

    std::string AccessLayerPluginManager::getVersion()
    {
        return std::string(PLUGIN_MANAGER_VERSION);
    }

    std::string AccessLayerPluginManager::getRepository()
    {
        return std::string(PLUGIN_MANAGER_REPOSITORY);
    }

    std::string AccessLayerPluginManager::getParameters()
    {
        return "";
    }

    void AccessLayerPluginManager::bind_readback_plugins(int ctxID) // function called before a get()
    { 
        int actxID = -1;
        printf("AccessLayerPluginManager::bind_readback_plugins is called\n");
        int size = 0; // number of nodes bound to a plugin
        ual_begin_arraystruct_action(ctxID, "ids_properties/plugins/node", "", &size, &actxID);
        assert(actxID >= 0);
        al_status_t status;
        //std::map<std::string, std::set<std::string>> plugins; // key = readback plugin name, value = set of nodes path
        int data_shape[MAXDIM];
        void *ptrData = NULL;
        for (int i = 0; i < size; i++)
        {
            status = ual_read_data(actxID, "path", "", &ptrData, CHAR_DATA, 1, &data_shape[0]);
            assert(status.code == 0);
            std::string path = std::string((char *)ptrData, data_shape[0]);
            int plugins_count = 0;
            int nested_actxID = -1;
            ual_begin_arraystruct_action(actxID, "readback", "", &plugins_count, &nested_actxID);
            assert(nested_actxID >= 0);

            std::vector<std::string> applied_plugins;
            std::vector<std::string> plugins_parameters;
            
            //getting the readback plugins
            for (int j = 0; j < plugins_count; j++)
            {
                status = ual_read_data(nested_actxID, "name", "", &ptrData, CHAR_DATA, 1, &data_shape[0]);
                assert(status.code == 0);
                std::string plugin_name = std::string((char *)ptrData, data_shape[0]);

                if (plugin_name.empty())
                   continue;

                status = ual_read_data(nested_actxID, "parameters", "", &ptrData, CHAR_DATA, 1, &data_shape[0]);
                assert(status.code == 0);
                std::string parameters = std::string((char *)ptrData, data_shape[0]);

                applied_plugins.push_back(plugin_name);
                plugins_parameters.push_back(parameters);
                ual_iterate_over_arraystruct(nested_actxID, 1);
            }

            //binding the readback plugins
            for (int j = applied_plugins.size() - 1; j>=0; j--) //order of application
            {
                const std::string &plugin_name = applied_plugins[j];
                printf("applied plugin = %s\n ", plugin_name.c_str());
                if (!LLplugin::isPluginRegistered(plugin_name.c_str()))
                   LLplugin::register_plugin(plugin_name.c_str());

                if (!LLplugin::isPluginBound(path.c_str(), plugin_name.c_str()))
                   LLplugin::bindPlugin(path.c_str(), plugin_name.c_str());

                LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
                access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;

                const std::string &parameters = plugins_parameters[j]; 
                al_plugin->setParameters(parameters);
            }

            ual_end_action(nested_actxID);
            ual_iterate_over_arraystruct(actxID, 1);
        }
        ual_end_action(actxID);


        //binding all plugins to the the AOS 'ids_properties/plugins/node' and the nodes below
        std::string aos_path_node;
        LLplugin::getFullPath(ctxID, "ids_properties/plugins/node",  aos_path_node);

        //std::string aos_path_node = "ids_properties/plugins/node";
        std::string aos_path_get;
        LLplugin::getFullPath(ctxID, "ids_properties/plugins/node/get_operation",  aos_path_get);

        std::string path_get_name = aos_path_get + "/name";
        std::string path_get_commit = aos_path_get + "/commit";
        std::string path_get_version = aos_path_get + "/version";
        std::string path_get_repository = aos_path_get + "/repository";
        std::string path_get_parameters = aos_path_get + "/parameters";

        for (auto it = LLplugin::boundPlugins.begin(); it != LLplugin::boundPlugins.end(); it++)
        {
            std::string path = it->first;
            std::vector<std::string> &plugins = it->second;
            for (int i = (int) plugins.size() - 1; i >= 0; i--) //reverse order of application
            {
                std::string &plugin_name = plugins[i];
                
                if (!LLplugin::isPluginBound(aos_path_node.c_str(), plugin_name.c_str()))
                    hli_bind_plugin(aos_path_node.c_str(), plugin_name.c_str());

                printf("binding plugin %s to %s\n ",  plugin_name.c_str(), aos_path_get.c_str());

                if (!LLplugin::isPluginBound(aos_path_get.c_str(), plugin_name.c_str()))
                    hli_bind_plugin(aos_path_get.c_str(), plugin_name.c_str());

                if (!LLplugin::isPluginBound(path_get_name.c_str(), plugin_name.c_str()))
                    hli_bind_plugin(path_get_name.c_str(), plugin_name.c_str());

                if (!LLplugin::isPluginBound(path_get_version.c_str(), plugin_name.c_str()))
                    hli_bind_plugin(path_get_version.c_str(), plugin_name.c_str());

                if (!LLplugin::isPluginBound(path_get_repository.c_str(), plugin_name.c_str()))
                    hli_bind_plugin(path_get_repository.c_str(), plugin_name.c_str());

                if (!LLplugin::isPluginBound(path_get_parameters.c_str(), plugin_name.c_str()))
                    hli_bind_plugin(path_get_parameters.c_str(), plugin_name.c_str());

            } 
        } 

        
    }


void AccessLayerPluginManager::write_plugins_metadata(int ctxID) { //function called at the end of a put()

    printf("AccessLayerPluginManager::write_plugins_metadata is called\n");
    int actxID = -1;
    int size = LLplugin::boundPlugins.size(); //number of nodes bound to plugins
    if (size == 0){
       return;
    } 

    ual_begin_arraystruct_action(ctxID, "ids_properties/plugins/node", "", &size, &actxID);
    assert(actxID >=0);

    for (auto it = LLplugin::boundPlugins.begin(); it != LLplugin::boundPlugins.end(); it++){
      std::string path = it->first;
      //remove dataObjectName from the path
      std::size_t found = path.find("/" , 0);
      assert(found != std::string::npos);
      path = path.substr(found + 1, std::string::npos);
     
      printf("write_plugins_metadata::path = %s\n ", path.c_str());
      std::string name = "path";
      write_field(actxID, name, path);
      int nested_actxID = -1;
      std::vector<std::string> &plugins = it->second;
      size = plugins.size();
      ual_begin_arraystruct_action(actxID, "put_operation", "", &size, &nested_actxID);
      assert(nested_actxID >=0);

      for (int i = 0; i < size; i++){ //stored in order of application
          std::string &plugin_name = plugins[i];
          LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
          access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
          write_field(nested_actxID, std::string("name"), al_plugin->getName());
	      write_field(nested_actxID, std::string("commit"), al_plugin->getCommit());
	      write_field(nested_actxID, std::string("version"), al_plugin->getVersion());
	      write_field(nested_actxID, std::string("repository"), al_plugin->getRepository());
	      write_field(nested_actxID, std::string("parameters"), al_plugin->getParameters());

          ual_iterate_over_arraystruct(nested_actxID, 1);
      } 
      ual_end_action(nested_actxID);
      nested_actxID = -1;
      ual_begin_arraystruct_action(actxID, "readback", "", &size, &nested_actxID);
      assert(nested_actxID >=0);

      for (int i = size - 1; i >= 0; i--){ //stored in reverse order of application
          std::string &plugin_name = plugins[i];
          LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
          access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
          write_field(nested_actxID, std::string("name"), al_plugin->getReadbackName(path));
          write_field(nested_actxID, std::string("commit"), al_plugin->getReadbackCommit(path));
          write_field(nested_actxID, std::string("version"), al_plugin->getReadbackVersion(path));
          write_field(nested_actxID, std::string("repository"), al_plugin->getReadbackRepository(path));
	      write_field(nested_actxID, std::string("parameters"), al_plugin->getReadbackParameters(path));

          ual_iterate_over_arraystruct(nested_actxID, 1);
      }
      ual_end_action(nested_actxID);
      ual_iterate_over_arraystruct(actxID, 1);
    }

    write_plugins_infrastructure_infos(ctxID);
}

void AccessLayerPluginManager::write_plugins_infrastructure_infos(int ctxID) {
    //printf("calling write_plugins_infrastructure_infos...\n");
    std::string name_put = "ids_properties/plugins/infrastructure_put/name";
    write_field(ctxID, name_put, getName());
    std::string commit_put = "ids_properties/plugins/infrastructure_put/commit";
    write_field(ctxID, commit_put, getCommit());
    std::string version_put = "ids_properties/plugins/infrastructure_put/version";
    write_field(ctxID, version_put, getVersion());
    std::string repository_put = "ids_properties/plugins/infrastructure_put/repository";
    write_field(ctxID, repository_put, getRepository());
    std::string name_get = "ids_properties/plugins/infrastructure_get/name";
    write_field(ctxID, name_get, getName());
    std::string commit_get = "ids_properties/plugins/infrastructure_get/commit";
    write_field(ctxID, commit_get, getCommit());
    std::string version_get = "ids_properties/plugins/infrastructure_get/version";
    write_field(ctxID, version_get, getVersion());
    std::string repository_get = "ids_properties/plugins/infrastructure_get/repository";
    write_field(ctxID, repository_get, getRepository());
}


void AccessLayerPluginManager::write_field(int ctxID, const std::string &field, const std::string &value) {
    void *ptrData = malloc(value.size() + 1);
    strcpy((char*) ptrData, value.c_str());
    int field_shape[1] = {(int)value.size()};
    al_status_t status = ual_write_data(ctxID, field.c_str(), "", ptrData, CHAR_DATA, 1, field_shape);
    free(ptrData);
    assert(status.code == 0);
}



