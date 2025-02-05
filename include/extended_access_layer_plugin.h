
/**
 * @file extended_access_layer_plugin.h
 * @brief Header file for the extended_access_layer_plugin class.
 */

#ifndef EXTENDED_ACCESS_LAYER_PLUGIN_H
#define EXTENDED_ACCESS_LAYER_PLUGIN_H 1

#include "access_layer_plugin.h"

/**
 * @class extended_access_layer_plugin
 * @brief An abstract class that extends the access_layer_plugin class.
 * 
 * This class provides an interface for plugins that extend the access layer
 * with additional functionality. 
 */
class extended_access_layer_plugin : public access_layer_plugin {

    public:
    /**
    * @brief Constructor for the extended_access_layer_plugin class.
    */
    extended_access_layer_plugin() {}

    /**
    * @brief Destructor for the extended_access_layer_plugin class.
    */
    virtual ~extended_access_layer_plugin() {}
    
    /**
    * @brief Pure virtual function to begin a time range action.
    * 
    * This function must be implemented by derived classes to perform actions
    * over a specified time range.
    * 
    * @param pulseCtx The pulse context identifier.
    * @param dataobjectname The name of the data object.
    * @param mode The mode of operation.
    * @param tmin The minimum time value.
    * @param tmax The maximum time value.
    * @param dtime A vector of time values.
    * @param interp The interpolation method.
    * @param opCtx The operation context identifier.
    */
    virtual void begin_timerange_action(int pulseCtx, const char* dataobjectname, int mode, double tmin, double tmax, std::vector<double> dtime, int interp, int opCtx) = 0;
};

#endif
