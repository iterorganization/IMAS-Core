/**
 * @file provenance_plugin_feature.h
 * @brief Defines the provenance_plugin_feature abstract base class.
 *
 * This header file contains the definition of the provenance_plugin_feature
 * abstract base class, which provides an interface for provenance plugin
 * features. The class includes methods to retrieve various metadata about
 * the plugin, such as its name, description, commit, version, repository,
 * and parameters.
 */
#ifndef PROVENANCE_PLUGIN_FEATURE_H
#define PROVENANCE_PLUGIN_FEATURE_H 1

#include <string>

/**
 * @class provenance_plugin_feature
 * @brief Abstract base class for provenance plugin features.
 *
 * This class defines the interface for provenance plugin features, 
 * providing methods to retrieve various metadata about the plugin.
 */
class provenance_plugin_feature {

    public:
    /**
    * @brief Retrieve the name of the plugin.
    * 
    * This function returns the name of the plugin as a string.
    * 
    */
    provenance_plugin_feature() {}

    /**
    * @brief Retrieve the description of the plugin.
    * 
    * This function returns the description of the plugin as a string.
    * 
    */
    virtual ~provenance_plugin_feature() {}

    virtual std::string getName() = 0;         ///< Retrieve the name of the plugin.
    virtual std::string getDescription() = 0;  ///< Retrieve the description of the plugin.
    virtual std::string getCommit() = 0;       ///< Retrieve the commit hash of the plugin.
    virtual std::string getVersion() = 0;      ///< Retrieve the version of the plugin.
    virtual std::string getRepository() = 0;   ///< Retrieve the repository URL of the plugin.
    virtual std::string getParameters() = 0;   ///< Retrieve the parameters of the plugin.

};

#endif
