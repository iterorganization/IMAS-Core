
/**
 * @file access_layer_plugin_manager.h
 * @brief Header file for the AccessLayerPluginManager class.
 */

#ifndef AL_PLUGIN_MANAGER_H
#define AL_PLUGIN_MANAGER_H 1

#include <string>
#include <vector>
#include <map>
#include "al_const.h"

#include "provenance_plugin_feature.h"

/**
 * @struct plugin_info
 * @brief Structure to hold information about a plugin.
 * 
 * @var plugin_info::name
 * Name of the plugin.
 * @var plugin_info::description
 * Description of the plugin.
 * @var plugin_info::commit
 * Commit hash of the plugin.
 * @var plugin_info::version
 * Version of the plugin.
 * @var plugin_info::repository
 * Repository URL of the plugin.
 * @var plugin_info::parameters
 * Parameters of the plugin.
 * @var plugin_info::application_index
 * Application index of the plugin.
 */

   struct plugin_info{
      std::string name;
      std::string description;
      std::string commit;
      std::string version;
      std::string repository;
      std::string parameters;
      int application_index;
  };
/**
 * @class AccessLayerPluginManager
 * @brief Manages access layer plugins and their operations.
 * 
 * Inherits from provenance_plugin_feature.
 */
class AccessLayerPluginManager: public provenance_plugin_feature {

private:
    /**
     * @brief Writes a field value.
     * @param ctxID Context ID.
     * @param field Field name.
     * @param value Field value.
     */
    void write_field(int ctxID, const std::string &field, const std::string &value);

    /**
     * @brief Adds a readback plugin.
     * @param plugin_name Name of the plugin.
     * @param path Path to the plugin.
     */
    void addReadbackPlugin(const std::string &plugin_name, const std::string &path);

    /**
     * @brief Sorts plugins.
     * @param p First plugin info.
     * @param q Second plugin info.
     * @return True if p should come before q, false otherwise.
     */
    static bool sortPlugins (const plugin_info &p, const plugin_info &q);

    /**
     * @brief Finds plugins based on operation and path.
     * @param node_operation Operation type.
     * @param path Path to search for plugins.
     * @param plugins Vector to store found plugins.
     */
    static void findPlugins(const plugin::OPERATION &node_operation, const std::string &path, std::vector<std::string> &plugins);

    /**
     * @brief Finds plugins for put operations.
     * @param plugins Map to store found plugins.
     */
    static void findPutOperationPlugins(std::map<std::string, std::vector<std::string>> &plugins);

    /**
     * @brief Finds plugins for get operations.
     * @param plugins Map to store found plugins.
     */
    static void findGetOperationPlugins(std::map<std::string, std::vector<std::string>> &plugins);

    /**
     * @brief Reads provenance information of plugins.
     * @param field Field name.
     * @param p_plugin Plugin pointer.
     * @param data Data pointer.
     * @param size Size of the data.
     */
    void read_plugins_provenance_infos(const std::string &field, void *p_plugin, void**data, int *size);

protected:
    /**
     * @brief Writes infrastructure information of plugins.
     * @param ctxID Context ID.
     */
    virtual void write_plugins_infrastructure_infos(int ctxID);

public:
    /**
     * @brief Constructor for AccessLayerPluginManager.
     */
    AccessLayerPluginManager();

    /**
     * @brief Destructor for AccessLayerPluginManager.
     */
    ~AccessLayerPluginManager();

    /**
     * @brief Gets the name of the plugin.
     * @return Name of the plugin.
     */
    std::string getName() override;

    /**
     * @brief Gets the description of the plugin.
     * @return Description of the plugin.
     */
    std::string getDescription() override;

    /**
     * @brief Gets the commit hash of the plugin.
     * @return Commit hash of the plugin.
     */
    std::string getCommit() override;

    /**
     * @brief Gets the version of the plugin.
     * @return Version of the plugin.
     */
    std::string getVersion() override;

    /**
     * @brief Gets the repository URL of the plugin.
     * @return Repository URL of the plugin.
     */
    std::string getRepository() override;

    /**
     * @brief Gets the parameters of the plugin.
     * @return Parameters of the plugin.
     */
    std::string getParameters() override;

    /**
     * @brief Writes metadata of plugins.
     * @param ctxID Context ID.
     */
    virtual void write_plugins_metadata(int ctxID);

    /**
     * @brief Binds readback plugins.
     * @param ctxID Context ID.
     */
    virtual void bind_readback_plugins(int ctxID);

    /**
     * @brief Unbinds readback plugins.
     * @param ctxID Context ID.
     */
    virtual void unbind_readback_plugins(int ctxID);

    /**
     * @brief Handles the beginning of an array structure action.
     * @param plugin_name Name of the plugin.
     * @param ctxID Context ID.
     * @param actxID Action context ID.
     * @param fieldPath Field path.
     * @param timeBasePath Time base path.
     * @param arraySize Size of the array.
     */
    virtual void begin_arraystruct_action_handler(const std::string &plugin_name, int ctxID, int *actxID, 
        const char* fieldPath, const char* timeBasePath, int *arraySize);

    /**
     * @brief Handles reading data from a plugin.
     * @param plugin_name Name of the plugin.
     * @param ctxID Context ID.
     * @param fieldPath Field path.
     * @param timeBasePath Time base path.
     * @param data Data pointer.
     * @param datatype Data type.
     * @param dim Dimension of the data.
     * @param size Size of the data.
     * @return Status code.
     */
    virtual int read_data_plugin_handler(const std::string &plugin_name, int ctxID, const char* fieldPath, const char* timeBasePath, 
        void **data, int datatype, int dim, int *size);

    /**
     * @brief Handles writing data to a plugin.
     * @param plugin_name Name of the plugin.
     * @param ctxID Context ID.
     * @param field Field name.
     * @param timebase Time base.
     * @param data Data pointer.
     * @param datatype Data type.
     * @param dim Dimension of the data.
     * @param size Size of the data.
     */
    virtual void write_data_plugin_handler(const std::string &plugin_name, int ctxID, const char *field, const char *timebase, 
              void *data, int datatype, int dim, int *size);

    /**
     * @brief Handles the end of an action.
     * @param ctxID Context ID.
     */
    virtual void end_action_plugin_handler(int ctxID);

    /**
     * @brief Gets the access mode.
     * @param ctxID Context ID.
     * @return Access mode.
     */
    int getAccessmode(int ctxID);

    /**
     * @brief Checks if write access should be skipped.
     * @param ctxID Context ID.
     * @param fieldPath Field path.
     * @return True if write access should be skipped, false otherwise.
     */
    bool skipWriteAccess(int ctxID, const char* fieldPath);
};

#endif
