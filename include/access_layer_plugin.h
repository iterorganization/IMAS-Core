
/**
 * @file access_layer_plugin.h
 * @brief Defines the access_layer_plugin class which provides an interface for access layer plugins.
 */

#ifndef ACCESS_LAYER_PLUGIN_H
#define ACCESS_LAYER_PLUGIN_H 1

#include "access_layer_base_plugin.h"
#include "readback_plugin_feature.h"

/**
 * @class access_layer_plugin
 * @brief Interface for access layer plugins.
 * 
 * This class inherits from access_layer_base_plugin and readback_plugin_feature
 * and provides pure virtual functions for various actions and data operations.
 */
class access_layer_plugin : public access_layer_base_plugin, public readback_plugin_feature {

    public:

    /**
     * @brief Constructor for access_layer_plugin.
     */
    access_layer_plugin() {}

    /**
     * @brief Destructor for access_layer_plugin.
     */
    virtual ~access_layer_plugin() {}
    
    /**
     * @brief Begins a global action.
     * 
     * @param pulseCtx Context of the pulse.
     * @param dataobjectname Name of the data object.
     * @param datapath Path to the data.
     * @param mode Mode of the action.
     * @param opCtx Operation context.
     */
    virtual void begin_global_action(int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) = 0;

    /**
     * @brief Begins a slice action.
     * 
     * @param pulseCtx Context of the pulse.
     * @param dataobjectname Name of the data object.
     * @param mode Mode of the action.
     * @param time Time of the action.
     * @param interp Interpolation mode.
     * @param opCtx Operation context.
     */
    virtual void begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) = 0;

    /**
     * @brief Begins an array structure action.
     * 
     * @param ctxID Context ID.
     * @param actxID Pointer to the action context ID.
     * @param fieldPath Path to the field.
     * @param timeBasePath Path to the time base.
     * @param arraySize Pointer to the array size.
     */
    virtual void begin_arraystruct_action(int ctxID, int *actxID, const char* fieldPath, const char* timeBasePath, int *arraySize) = 0;

    /**
     * @brief Ends an action.
     * 
     * @param ctx Context ID.
     */
    virtual void end_action(int ctx) = 0; 

    /**
     * @brief Reads data.
     * 
     * @param ctx Context ID.
     * @param fieldPath Path to the field.
     * @param timeBasePath Path to the time base.
     * @param data Pointer to the data.
     * @param datatype Type of the data.
     * @param dim Dimension of the data.
     * @param size Pointer to the size of the data.
     * @return int Status of the read operation.
     */
    virtual int read_data(int ctx, const char* fieldPath, const char* timeBasePath, void **data, int datatype, int dim, int *size) = 0;

    /**
     * @brief Writes data.
     * 
     * @param ctxID Context ID.
     * @param field Path to the field.
     * @param timebase Path to the time base.
     * @param data Pointer to the data.
     * @param datatype Type of the data.
     * @param dim Dimension of the data.
     * @param size Pointer to the size of the data.
     */
    virtual void write_data(int ctxID, const char *field, const char *timebase, void *data, int datatype, int dim, int *size) = 0;

    /**
     * @brief Gets the node operation for a given path.
     * 
     * @param path Path to the node.
     * @return plugin::OPERATION Operation type for the node.
     */
    virtual plugin::OPERATION node_operation(const std::string &path) = 0;
};

#endif
