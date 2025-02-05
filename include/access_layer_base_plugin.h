/**
 * @file access_layer_base_plugin.h
 * @brief Defines the access_layer_base_plugin class and related types.
 */

#ifndef AL_BASE_PLUGIN_H
#define AL_BASE_PLUGIN_H 1

#include "al_const.h"
#include "al_lowlevel.h"
#include <assert.h>
#include "provenance_plugin_feature.h"

/**
 * @class access_layer_base_plugin
 * @brief A base class for access layer plugins, inheriting from provenance_plugin_feature.
 */
class access_layer_base_plugin: public provenance_plugin_feature
{
private:
    std::string parameters; ///< Stores the parameters as a string.

public:
    /**
     * @brief Default constructor for access_layer_base_plugin.
     */
    access_layer_base_plugin() {}

    /**
     * @brief Virtual destructor for access_layer_base_plugin.
     */
    virtual ~access_layer_base_plugin() {}

    /**
     * @brief Pure virtual function to set a parameter.
     * @param parameter_name The name of the parameter.
     * @param datatype The data type of the parameter.
     * @param dim The dimension of the parameter.
     * @param size The size of the parameter.
     * @param data The data of the parameter.
     */
    virtual void setParameter(const char *parameter_name, int datatype, int dim, int *size, void *data) = 0;

    /**
     * @brief Sets the parameters.
     * @param parameters The parameters as a string.
     */
    void setParameters(const std::string &parameters){
        this->parameters = parameters;
    } 
};

/**
 * @typedef create_t
 * @brief Typedef for a function that creates an access_layer_base_plugin.
 */
typedef access_layer_base_plugin *create_t();

/**
 * @typedef destroy_t
 * @brief Typedef for a function that destroys an access_layer_base_plugin.
 */
typedef void destroy_t(access_layer_base_plugin *);

#endif
