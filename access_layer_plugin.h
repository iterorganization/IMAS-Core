#ifndef ACCESS_LAYER_PLUGIN_H
#define ACCESS_LAYER_PLUGIN_H 1

#include "access_layer_base_plugin.h"
#include "readback_plugin_feature.h"

class access_layer_plugin : public access_layer_base_plugin, public readback_plugin_feature {

    public:

    access_layer_plugin() {}
    virtual ~access_layer_plugin() {}
    
    virtual void begin_global_action(int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) = 0;
    virtual void begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) = 0;
    virtual void begin_arraystruct_action(int ctxID, int *actxID, const char* fieldPath, const char* timeBasePath, int *arraySize) = 0;
    virtual void end_action(int ctx) = 0; 

    virtual int read_data(int ctx, const char* fieldPath, const char* timeBasePath, void **data, int datatype, int dim, int *size) = 0;
    virtual void write_data(int ctxID, const char *field, const char *timebase, void *data, int datatype, int dim, int *size) = 0;

    virtual plugin::OPERATION node_operation(const std::string &path) = 0;
};

#endif
