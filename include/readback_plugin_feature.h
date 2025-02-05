/**
 * @file readback_plugin_feature.h
 * @brief Defines the abstract base class for readback plugin features.
 *
 * This header file contains the declaration of the readback_plugin_feature class,
 * which provides an interface for readback plugin features. 
 *
 */


#ifndef READBACK_PLUGIN_FEATURE_H
#define READBACK_PLUGIN_FEATURE_H 1

#include <string>

/**
 * @class readback_plugin_feature
 * @brief Abstract base class for readback plugin features.
 *
 * This class provides an interface for readback plugin features, defining
 * several pure virtual functions that must be implemented by derived classes.
 */
class readback_plugin_feature {

    public:

    /**
     * @brief Constructor for the readback_plugin_feature class.
     */
    readback_plugin_feature() {}
    /**
     * @brief Virtual destructor for the readback_plugin_feature class.
     *
     * This destructor ensures that derived class destructors are called properly.
     */
    virtual ~readback_plugin_feature() {}

    virtual std::string getReadbackName(const std::string &path, int *application_index) = 0; // Get the name of the readback
    virtual std::string getReadbackDescription(const std::string &path) = 0; // Get the description of the readback
    virtual std::string getReadbackCommit(const std::string &path) = 0; // Get the commit ID of the readback
    virtual std::string getReadbackVersion(const std::string &path) = 0; // Get the version of the readback
    virtual std::string getReadbackRepository(const std::string &path) = 0; // Get the repository URL of the readback
    virtual std::string getReadbackParameters(const std::string &path) = 0; // Get the parameters of the readback
};

#endif
