#include "access_layer_plugin_manager.h"

#include <string>
#include <algorithm>
#include "access_layer_plugin.h"

#define PLUGIN_MANAGER_NAME "AccessLayerPluginManager"
#define PLUGIN_MANAGER_VERSION "1.0.0"
#define PLUGIN_MANAGER_REPOSITORY "ssh://git@git.iter.org/imas/access-layer.git"

#define PLUGINS_NODE_PATH "ids_properties/plugins/node"
#define PLUGINS_READBACK_PATH "readback"

#define GET_OPERATION_NODE_PATH "ids_properties/plugins/node/get_operation"
#define GET_OPERATION "get_operation"

#define PUT_OPERATION "put_operation"

AccessLayerPluginManager::AccessLayerPluginManager() {}
AccessLayerPluginManager::~AccessLayerPluginManager() {}

std::string AccessLayerPluginManager::getName()
{
    return std::string(PLUGIN_MANAGER_NAME);
}

std::string AccessLayerPluginManager::getDescription()
{
    return "The plugin manager binds readback plugins and manage the plugins provenance.";
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

    al_status_t status;
    int data_shape[MAXDIM];
    void *ptrData = NULL;

    int actxID = -1;
    // printf("AccessLayerPluginManager::bind_readback_plugins is called\n");
    int size = 0; // number of nodes bound to a plugin
    int h = -1;
    ptrData = &h;
    status = al_plugin_read_data(ctxID, "ids_properties/homogeneous_time", "", &ptrData, INTEGER_DATA, 0, &data_shape[0]); //ASCII backend patch
    al_plugin_begin_arraystruct_action(ctxID, PLUGINS_NODE_PATH, "", &size, &actxID);
    assert(actxID >= 0);

    for (int i = 0; i < size; i++)
    {
        status = al_plugin_read_data(actxID, "path", "", &ptrData, CHAR_DATA, 1, &data_shape[0]);
        assert(status.code == 0);
        std::string path = std::string((char *)ptrData, data_shape[0]);
        int plugins_count = 0;
        int nested_actxID = -1;
        al_plugin_begin_arraystruct_action(actxID, "readback", "", &plugins_count, &nested_actxID);
        assert(nested_actxID >= 0);

        std::vector<std::string> applied_plugins;
        std::vector<std::string> plugins_parameters;
        std::vector<std::string> plugins_versions;

        // getting the readback plugins from the backend
        for (int j = 0; j < plugins_count; j++)
        {
            status = al_plugin_read_data(nested_actxID, "name", "", &ptrData, CHAR_DATA, 1, &data_shape[0]);
            assert(status.code == 0);
            std::string plugin_name = std::string((char *)ptrData, data_shape[0]);

            if (plugin_name.empty())
                continue;

            status = al_plugin_read_data(nested_actxID, "version", "", &ptrData, CHAR_DATA, 1, &data_shape[0]);
            assert(status.code == 0);
            std::string version = std::string((char *)ptrData, data_shape[0]);

            status = al_plugin_read_data(nested_actxID, "parameters", "", &ptrData, CHAR_DATA, 1, &data_shape[0]);
            assert(status.code == 0);
            std::string parameters = std::string((char *)ptrData, data_shape[0]);

            applied_plugins.push_back(plugin_name);
            plugins_parameters.push_back(parameters);
            plugins_versions.push_back(version);
            al_iterate_over_arraystruct(nested_actxID, 1);
        }

        // binding the readback plugins
        for (int j = applied_plugins.size() - 1; j >= 0; j--) // order of application
        {
            const std::string &plugin_name = applied_plugins[j];
            // printf("applied plugin = %s\n ", plugin_name.c_str());
            if (!LLplugin::isPluginRegistered(plugin_name.c_str()))
            {
                if (LLplugin::registerPlugin(plugin_name.c_str()))
                    LLplugin::readbackPlugins.push_back(plugin_name);
                else
                    continue;
            }

            std::string full_path;
            std::string dataObjectName;
            LLplugin::getFullPath(ctxID, path.c_str(), full_path, dataObjectName);

            // printf("binding readback plugin %s to %s\n ",  plugin_name.c_str(), full_path.c_str());

            if (!LLplugin::isPluginBound(full_path.c_str(), plugin_name.c_str()))
            {
                LLplugin::bindPlugin(full_path.c_str(), plugin_name.c_str());
                //check if the plugin present in the store has the expected version and node_operation type (GET)
                LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
                access_layer_plugin *al_plugin = (access_layer_plugin *)llp.al_plugin;
                if (al_plugin->node_operation(full_path) == plugin::OPERATION::PUT_ONLY)
                {
                    char error_message[200];
                    sprintf(error_message, "Readback plugin %s is declared PUT_ONLY for path=%s.\n", plugin_name.c_str(), full_path.c_str());
                    throw ALBackendException(error_message, LOG);
                }
                if (al_plugin->getVersion() != plugins_versions[j])
                {
                    char error_message[200];
                    sprintf(error_message, "Readback plugin %s has not the expected version, found=%s, expected=%s.\n", plugin_name.c_str(),
                            al_plugin->getVersion().c_str(), plugins_versions[j].c_str());
                    throw ALBackendException(error_message, LOG);
                }
                addReadbackPlugin(plugin_name, full_path);
            }

            LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
            access_layer_plugin *al_plugin = (access_layer_plugin *)llp.al_plugin;

            const std::string &parameters = plugins_parameters[j];
            al_plugin->setParameters(parameters);
        }

        al_plugin_end_action(nested_actxID);
        al_iterate_over_arraystruct(actxID, 1);
    }
    al_plugin_end_action(actxID);

    // binding all plugins to the AOS 'ids_properties/plugins/node' and the nodes below
    std::string aos_path_node;
    LLplugin::getFullPath(ctxID, PLUGINS_NODE_PATH, aos_path_node);
    std::string path_get_path = aos_path_node + "/path";

    // std::string aos_path_node = "ids_properties/plugins/node";
    std::string aos_path_get;
    LLplugin::getFullPath(ctxID, GET_OPERATION_NODE_PATH, aos_path_get);

    std::string path_get_name = aos_path_get + "/name";
    std::string path_description_name = aos_path_get + "/description";
    std::string path_get_commit = aos_path_get + "/commit";
    std::string path_get_version = aos_path_get + "/version";
    std::string path_get_repository = aos_path_get + "/repository";
    std::string path_get_parameters = aos_path_get + "/parameters";

    // now, we bind the plugins which contributes to get()/get_slice() operations to the data structure 'get_operation'

    std::map<std::string, std::vector<std::string>> get_plugins; // searching for plugins contributing to get()/get_slice() operations
    findGetOperationPlugins(get_plugins);
    size = get_plugins.size();
    if (size == 0)
    {
        return;
    }

    for (auto it = get_plugins.begin(); it != get_plugins.end(); it++)
    {
        // const std::string &path = it->first;
        std::vector<std::string> &plugins = it->second;
        for (int i = (int)plugins.size() - 1; i >= 0; i--) // reverse order of application
        {
            std::string &plugin_name = plugins[i];

            if (!LLplugin::isPluginBound(aos_path_node.c_str(), plugin_name.c_str()))
            {
                al_bind_plugin(aos_path_node.c_str(), plugin_name.c_str());
                // printf("binding plugin %s to %s\n ",  plugin_name.c_str(), aos_path_node.c_str());
                addReadbackPlugin(plugin_name, aos_path_node);
            }

            if (!LLplugin::isPluginBound(aos_path_get.c_str(), plugin_name.c_str()))
            {
                al_bind_plugin(aos_path_get.c_str(), plugin_name.c_str());
                addReadbackPlugin(plugin_name, aos_path_get);
            }

            if (!LLplugin::isPluginBound(path_get_path.c_str(), plugin_name.c_str()))
            {
                al_bind_plugin(path_get_path.c_str(), plugin_name.c_str());
                addReadbackPlugin(plugin_name, path_get_path);
            }

            if (!LLplugin::isPluginBound(path_get_name.c_str(), plugin_name.c_str()))
            {
                al_bind_plugin(path_get_name.c_str(), plugin_name.c_str());
                addReadbackPlugin(plugin_name, path_get_name);
            }

            if (!LLplugin::isPluginBound(path_description_name.c_str(), plugin_name.c_str()))
            {
                al_bind_plugin(path_description_name.c_str(), plugin_name.c_str());
                addReadbackPlugin(plugin_name, path_description_name);
            }

            if (!LLplugin::isPluginBound(path_get_commit.c_str(), plugin_name.c_str()))
            {
                al_bind_plugin(path_get_commit.c_str(), plugin_name.c_str());
                addReadbackPlugin(plugin_name, path_get_commit);
            }

            if (!LLplugin::isPluginBound(path_get_version.c_str(), plugin_name.c_str()))
            {
                al_bind_plugin(path_get_version.c_str(), plugin_name.c_str());
                addReadbackPlugin(plugin_name, path_get_version);
            }

            if (!LLplugin::isPluginBound(path_get_repository.c_str(), plugin_name.c_str()))
            {
                al_bind_plugin(path_get_repository.c_str(), plugin_name.c_str());
                addReadbackPlugin(plugin_name, path_get_repository);
            }

            if (!LLplugin::isPluginBound(path_get_parameters.c_str(), plugin_name.c_str()))
            {
                al_bind_plugin(path_get_parameters.c_str(), plugin_name.c_str());
                addReadbackPlugin(plugin_name, path_get_parameters);
            }
        }
    }
}

void AccessLayerPluginManager::addReadbackPlugin(const std::string &plugin_name, const std::string &path)
{
    auto got = LLplugin::boundReadbackPlugins.find(path);
    if (got != LLplugin::boundReadbackPlugins.end())
    {
        auto &plugin_names = got->second;
        plugin_names.push_back(plugin_name);
    }
    else
    {
        std::vector<std::string> plugin_names = std::vector<std::string>();
        plugin_names.push_back(plugin_name);
        LLplugin::boundReadbackPlugins[path] = plugin_names;
    }
}

void AccessLayerPluginManager::unbind_readback_plugins(int ctxID) // function called after a get() to unbind the readback plugins
{
    LLplugin::boundReadbackPlugins.clear();
    for (int i = 0; i < (int)LLplugin::readbackPlugins.size(); i++)
    {
        LLplugin::unregisterPlugin(LLplugin::readbackPlugins[i].c_str());
    }
    LLplugin::readbackPlugins.clear();
}

bool AccessLayerPluginManager::sortPlugins(const plugin_info &p, const plugin_info &q) { return (p.application_index > q.application_index); }

void AccessLayerPluginManager::findPlugins(const plugin::OPERATION &node_operation, const std::string &path, std::vector<std::string> &plugins)
{
    LLplugin::getBoundPlugins(path.c_str(), plugins);
    std::vector<std::string> plugins_requested;
    for (const std::string &plugin_name : plugins)
    {
        LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
        access_layer_plugin *al_plugin = (access_layer_plugin *)llp.al_plugin;
        if (al_plugin->node_operation(path) == node_operation)
            plugins_requested.push_back(plugin_name);
    }
}

void AccessLayerPluginManager::findGetOperationPlugins(std::map<std::string, std::vector<std::string>> &plugins)
{
    for (auto it = LLplugin::boundPlugins.begin(); it != LLplugin::boundPlugins.end(); it++)
    {
        const std::string &path = it->first;
        std::string node_plugins(PLUGINS_NODE_PATH);
        if (path.find(node_plugins.c_str(), 0, node_plugins.length()) != std::string::npos)
        {
            //printf("ignoring path=%s\n", path.c_str());
            continue;
        }
        std::vector<std::string> &plugins_vector = it->second;
        for (auto &plugin_name : plugins_vector)
        {
            LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
            access_layer_plugin *al_plugin = (access_layer_plugin *)llp.al_plugin;
            if (al_plugin->node_operation(path) == plugin::OPERATION::PUT_AND_GET ||
                al_plugin->node_operation(path) == plugin::OPERATION::GET_ONLY)
            {
                auto got = plugins.find(path);
                if (got == plugins.end())
                {
                    std::vector<std::string> get_plugins;
                    get_plugins.push_back(plugin_name);
                    plugins[path] = get_plugins;
                }
                else
                {
                    std::vector<std::string> &get_plugins = got->second;
                    get_plugins.push_back(plugin_name);
                }
            }
        }
    }
}

void AccessLayerPluginManager::findPutOperationPlugins(std::map<std::string, std::vector<std::string>> &plugins)
{
    for (auto it = LLplugin::boundPlugins.begin(); it != LLplugin::boundPlugins.end(); it++)
    {
        const std::string &path = it->first;
        std::vector<std::string> &plugins_vector = it->second;
        for (auto &plugin_name : plugins_vector)
        {
            LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
            access_layer_plugin *al_plugin = (access_layer_plugin *)llp.al_plugin;
            if (al_plugin->node_operation(path) == plugin::OPERATION::PUT_AND_GET ||
                al_plugin->node_operation(path) == plugin::OPERATION::PUT_ONLY)
            {
                auto got = plugins.find(path);
                if (got == plugins.end())
                {
                    std::vector<std::string> put_plugins;
                    put_plugins.push_back(plugin_name);
                    plugins[path] = put_plugins;
                }
                else
                {
                    std::vector<std::string> &put_plugins = got->second;
                    put_plugins.push_back(plugin_name);
                }
            }
        }
    }
}

void AccessLayerPluginManager::write_plugins_metadata(int ctxID) // function called at the end of a put()
{

    // printf("AccessLayerPluginManager::write_plugins_metadata is called\n");
    int actxID = -1;
    int size = LLplugin::boundPlugins.size(); // number of nodes bound to plugins
    if (size == 0)
    {
        return;
    }

    std::map<std::string, std::vector<std::string>> put_plugins; // searching for plugins contributing to put()/put_slice() operations
    findPutOperationPlugins(put_plugins);
    size = put_plugins.size();
    if (size == 0)
    {
        return;
    }

    al_plugin_begin_arraystruct_action(ctxID, PLUGINS_NODE_PATH, "", &size, &actxID);
    assert(actxID >= 0);

    for (auto it = put_plugins.begin(); it != put_plugins.end(); it++)
    {
        std::string path = it->first;
        // remove dataObjectName from the path
        std::size_t found = path.find("/", 0);
        assert(found != std::string::npos);
        path = path.substr(found + 1, std::string::npos);

        // printf("write_plugins_metadata::path = %s\n ", path.c_str());
        std::string name = "path";
        write_field(actxID, name, path);
        int nested_actxID = -1;
        std::vector<std::string> &plugins = it->second;
        size = plugins.size();
        al_plugin_begin_arraystruct_action(actxID, PUT_OPERATION, "", &size, &nested_actxID);
        assert(nested_actxID >= 0);

        for (int i = 0; i < size; i++)
        { // stored in order of application
            std::string &plugin_name = plugins[i];
            LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
            access_layer_plugin *al_plugin = (access_layer_plugin *)llp.al_plugin;
            write_field(nested_actxID, std::string("name"), al_plugin->getName());
            write_field(nested_actxID, std::string("description"), al_plugin->getDescription());
            write_field(nested_actxID, std::string("commit"), al_plugin->getCommit());
            write_field(nested_actxID, std::string("version"), al_plugin->getVersion());
            write_field(nested_actxID, std::string("repository"), al_plugin->getRepository());
            write_field(nested_actxID, std::string("parameters"), al_plugin->getParameters());

            al_iterate_over_arraystruct(nested_actxID, 1);
        }
        al_plugin_end_action(nested_actxID);

        // we first collect all readback plugins and the positions at which they must be applied to the data
        std::vector<struct plugin_info> plugins_to_apply;

        for (int i = 0; i < size; i++)
        {
            std::string &plugin_name = plugins[i];
            LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
            access_layer_plugin *al_plugin = (access_layer_plugin *)llp.al_plugin;
            struct plugin_info p_info;
            int application_index;
            p_info.name = al_plugin->getReadbackName(path, &application_index);
            if (p_info.name != "")
            {
                p_info.commit = al_plugin->getReadbackCommit(path);
                p_info.description = al_plugin->getReadbackDescription(path);
                p_info.version = al_plugin->getReadbackVersion(path);
                p_info.repository = al_plugin->getReadbackRepository(path);
                p_info.parameters = al_plugin->getReadbackParameters(path);
                p_info.application_index = application_index;
                plugins_to_apply.push_back(p_info);
            }
            else
            {
                // no readback plugin defined
            }
        }
        // Checking application index of each plugin
        int nb_applications = (int)plugins_to_apply.size();
        std::set<int> application_indices;
        for (int i = 0; i < nb_applications; i++)
        {
            struct plugin_info &p_info = plugins_to_apply[i];
            int application_index = p_info.application_index;

            // check if index is valid
            // An index of application equals to 0 means that the plugin should be the first to be applied to the data
            if (application_index >= size || application_index < 0)
            {
                char error_message[200];
                sprintf(error_message, "Plugin index (%d) is out of bounds (must be < %d and >=0) for plugin %s at node path=%s.\n",
                        application_index, nb_applications, p_info.name.c_str(), path.c_str());
                throw ALBackendException(error_message, LOG);
            }
            if (application_indices.find(application_index) != application_indices.end())
            {
                char error_message[200];
                sprintf(error_message, "Plugin index (%d) is already defined for plugin %s at node path=%s.\n", application_index, p_info.name.c_str(), path.c_str());
                throw ALBackendException(error_message, LOG);
            }
            application_indices.insert(application_index);
        }

        std::sort(plugins_to_apply.begin(), plugins_to_apply.end(), sortPlugins); // sorted in reverse order of application

        nested_actxID = -1;
        al_plugin_begin_arraystruct_action(actxID, "readback", "", &nb_applications, &nested_actxID);
        assert(nested_actxID >= 0);

        for (int i = 0; i < nb_applications; i++)
        {
            struct plugin_info &p_info = plugins_to_apply[i];
            write_field(nested_actxID, std::string("name"), p_info.name);
            write_field(nested_actxID, std::string("description"), p_info.description);
            write_field(nested_actxID, std::string("commit"), p_info.commit);
            write_field(nested_actxID, std::string("version"), p_info.version);
            write_field(nested_actxID, std::string("repository"), p_info.repository);
            write_field(nested_actxID, std::string("parameters"), p_info.parameters);

            al_iterate_over_arraystruct(nested_actxID, 1);
        }
        al_plugin_end_action(nested_actxID);
        al_iterate_over_arraystruct(actxID, 1);
    }

    write_plugins_infrastructure_infos(ctxID);
}

void AccessLayerPluginManager::write_plugins_infrastructure_infos(int ctxID)
{
    // printf("calling write_plugins_infrastructure_infos...\n");
    std::string name_put = "ids_properties/plugins/infrastructure_put/name";
    write_field(ctxID, name_put, getName());
    std::string description_put = "ids_properties/plugins/infrastructure_put/description";
    write_field(ctxID, description_put, getDescription());
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

void AccessLayerPluginManager::write_field(int ctxID, const std::string &field, const std::string &value)
{
    void *ptrData = malloc(value.size() + 1);
    strcpy((char *)ptrData, value.c_str());
    int field_shape[1] = {(int)value.size()};
    al_status_t status = al_plugin_write_data(ctxID, field.c_str(), "", ptrData, CHAR_DATA, 1, field_shape);
    free(ptrData);
    assert(status.code == 0);
}

bool AccessLayerPluginManager::skipWriteAccess(int ctxID, const char *fieldPath)
{
    if (!LLplugin::pluginsFrameworkEnabled())
        return false;
    std::string field = fieldPath;
    if (field == GET_OPERATION && getAccessmode(ctxID) == WRITE_OP)
        return true;
    return false;
}

int AccessLayerPluginManager::getAccessmode(int ctxID)
{
    LLenv lle = Lowlevel::getLLenv(ctxID);
    OperationContext *ctx = NULL;
    if (lle.context->getType() == CTX_ARRAYSTRUCT_TYPE)
    {
        ArraystructContext *arctx = dynamic_cast<ArraystructContext *>(lle.context);
        ctx = arctx->getOperationContext();
    }
    else if (lle.context->getType() == CTX_OPERATION_TYPE)
    {
        ctx = dynamic_cast<OperationContext *>(lle.context);
    }
    else
    {
        throw ALLowlevelException("Unexpected context in access_layer_plugin::begin_arraystruct_action_handler");
    }
    return ctx->getAccessmode();
}

void AccessLayerPluginManager::begin_arraystruct_action_handler(const std::string &plugin_name, int ctxID, int *actxID,
                                                                const char *fieldPath, const char *timeBasePath, int *arraySize)
{
    LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
    access_layer_plugin *al_plugin = (access_layer_plugin *)llp.al_plugin;
    LLenv lle = Lowlevel::getLLenv(ctxID);
    OperationContext *ctx = NULL;
    if (lle.context->getType() == CTX_ARRAYSTRUCT_TYPE)
    {
        ArraystructContext *arctx = dynamic_cast<ArraystructContext *>(lle.context);
        ctx = arctx->getOperationContext();
    }
    else if (lle.context->getType() == CTX_OPERATION_TYPE)
    {
        ctx = dynamic_cast<OperationContext *>(lle.context);
    }
    else
    {
        throw ALLowlevelException("Unexpected context in access_layer_plugin::begin_arraystruct_action_handler");
    }
    // printf("begin_arraystruct_action_handler--> fieldPath=%s\n ", fieldPath);

    if (ctx->getAccessmode() == READ_OP)
    {
        *arraySize = 0;
        std::string p = fieldPath;
        if (p == PLUGINS_NODE_PATH)
        {
            LLplugin::get_plugins.clear();
            findGetOperationPlugins(LLplugin::get_plugins);
            ArraystructContext *actx = lle.create(fieldPath, timeBasePath);
            *arraySize = LLplugin::get_plugins.size();
            *actxID = Lowlevel::addLLenv(lle.backend, actx);
            return;
        }
        else if (p == PLUGINS_READBACK_PATH)
        {
            ArraystructContext *actx = lle.create(fieldPath, timeBasePath);
            *arraySize = LLplugin::readbackPlugins.size();
            *actxID = Lowlevel::addLLenv(lle.backend, actx);
            return;
        }
        else if (p == GET_OPERATION)
        {
            LLplugin::pluginsNames.clear();
            std::vector<std::string> paths;
            for (std::map<std::string, std::vector<std::string>>::iterator it = LLplugin::get_plugins.begin(); it != LLplugin::get_plugins.end(); ++it)
            {
                std::string path = it->first;
                paths.push_back(path);
            }
            ArraystructContext *arctx = dynamic_cast<ArraystructContext *>(lle.context);
            const std::string &path = paths[arctx->getIndex()];

            LLplugin::pluginsNames = LLplugin::get_plugins[path];
            ArraystructContext *actx = lle.create(fieldPath, timeBasePath);
            *arraySize = LLplugin::get_plugins[path].size();
            *actxID = Lowlevel::addLLenv(lle.backend, actx);
            return;
        }
    }
    al_plugin->begin_arraystruct_action(ctxID, actxID, fieldPath, timeBasePath, arraySize);
}

int AccessLayerPluginManager::read_data_plugin_handler(const std::string &plugin_name, int ctxID, const char *fieldPath, const char *timeBasePath,
                                                       void **data, int datatype, int dim, int *size)
{
    // printf("read_data_handler is called for path=%s, plugin=%s\n", fieldPath, plugin_name.c_str());
    LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
    access_layer_plugin *al_plugin = (access_layer_plugin *)llp.al_plugin;
    LLenv lle = Lowlevel::getLLenv(ctxID);
    if ((lle.context->getType() == CTX_ARRAYSTRUCT_TYPE) && datatype == CHAR_DATA && dim == 1)
    {
        ArraystructContext *arctx = dynamic_cast<ArraystructContext *>(lle.context);
        // printf("read_data_plugin_handler::updating ...arctx->getPath()=%s\n", arctx->getPath().c_str());
        if (arctx->getPath() == PLUGINS_NODE_PATH)
        {
            std::string field = fieldPath;
            if (field == "path")
            {
                std::vector<std::string> paths;
                for (std::map<std::string, std::vector<std::string>>::iterator it = LLplugin::get_plugins.begin(); it != LLplugin::get_plugins.end(); ++it)
                {
                    std::string path = it->first;
                    paths.push_back(path);
                }
                const std::string &path = paths[arctx->getIndex()];
                LLplugin::getOperationPath = path;
                *data = strdup(path.c_str());
                *size = path.length();
                return 1;
            }
        }

        else if (arctx->getPath() == PLUGINS_READBACK_PATH)
        {

            std::string field = fieldPath;
            for (auto it = LLplugin::readbackPlugins.begin(); it != LLplugin::readbackPlugins.end(); ++it)
            {
                std::string &plugin_name = *it;
                LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
                access_layer_plugin *al_readback_plugin = (access_layer_plugin *)llp.al_plugin;
                read_plugins_provenance_infos(field, (void *)al_readback_plugin, data, size);
            }
        }

        else if (arctx->getPath() == GET_OPERATION)
        {
            assert(LLplugin::pluginsNames.size() > 0);
            const std::string &plugin_name = LLplugin::pluginsNames[arctx->getIndex()];
            /*if (LLplugin::getOperationPath.rfind("ids_properties/plugins/node")){ //do not update for this path
                    return 0;
                } */

            if (plugin_name == al_plugin->getName())
            {
                // checking if the plugin contributes to the get()/get_slice() operation, other wise, the get_operation data structure is not updated for this plugin
                if (al_plugin->node_operation(LLplugin::getOperationPath) == plugin::OPERATION::PUT_ONLY)
                {
                    return 0;
                }

                std::string field = fieldPath;
                read_plugins_provenance_infos(field, (void *)al_plugin, data, size);
                return 1;
            }
            else
                return 0;
        }
    }

    if (al_plugin->node_operation(LLplugin::getOperationPath) != plugin::OPERATION::PUT_ONLY)
        return al_plugin->read_data(ctxID, fieldPath, timeBasePath, data, datatype, dim, size);
    return 0;
}

void AccessLayerPluginManager::read_plugins_provenance_infos(const std::string &field, void *p_plugin, void **data, int *size)
{
    access_layer_plugin *al_plugin = (access_layer_plugin *)p_plugin;
    if (field == "name")
    {
        *data = strdup(al_plugin->getName().data());
        *size = al_plugin->getName().length();
    }
    else if (field == "description")
    {
        *data = strdup(al_plugin->getDescription().data());
        *size = al_plugin->getDescription().length();
    }
    else if (field == "commit")
    {
        *data = strdup(al_plugin->getCommit().data());
        *size = al_plugin->getCommit().length();
    }
    else if (field == "version")
    {
        *data = strdup(al_plugin->getVersion().data());
        *size = al_plugin->getVersion().length();
    }
    else if (field == "repository")
    {
        *data = strdup(al_plugin->getRepository().data());
        *size = al_plugin->getRepository().length();
    }
    else if (field == "parameters")
    {
        *data = strdup(al_plugin->getParameters().data());
        *size = al_plugin->getParameters().length();
    }
}

void AccessLayerPluginManager::write_data_plugin_handler(const std::string &plugin_name, int ctxID, const char *field, const char *timebase,
                                                         void *data, int datatype, int dim, int *size)
{
    access_layer_plugin *al_plugin = NULL;
    LLplugin &llp = LLplugin::llpluginsStore[plugin_name];
    al_plugin = (access_layer_plugin *)llp.al_plugin;
    if (al_plugin->node_operation(LLplugin::getOperationPath) != plugin::OPERATION::GET_ONLY)
        al_plugin->write_data(ctxID, field, timebase, data, datatype, dim, size);
}

void AccessLayerPluginManager::end_action_plugin_handler(int ctxID)
{
    LLenv lle = Lowlevel::getLLenv(ctxID);
    OperationContext *octx = NULL;
    if (lle.context->getType() == CTX_OPERATION_TYPE)
    {
        octx = dynamic_cast<OperationContext *>(lle.context);
    }
    else if (lle.context->getType() == CTX_ARRAYSTRUCT_TYPE)
    {
        ArraystructContext *actx = dynamic_cast<ArraystructContext *>(lle.context);
        octx = actx->getOperationContext();
    }
    if (octx == NULL)
        return;
    const std::string &dataObjectName = octx->getDataobjectName();
    std::set<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(dataObjectName.c_str(), pluginsNames);
    if (!isPluginBound)
        return;

    access_layer_plugin *al_plugin = NULL;
    for (auto it = pluginsNames.begin(); it != pluginsNames.end(); it++)
    {
        const std::string &pluginName = *it;
        // printf("Found plugin %s, for dataobject %s\n", pluginName.c_str(), dataObjectName.c_str());
        LLplugin &llp = LLplugin::llpluginsStore[pluginName];
        al_plugin = (access_layer_plugin *)llp.al_plugin;
        al_plugin->end_action(ctxID);
    }
}
