#ifndef AL_PLUGIN_H
#define AL_PLUGIN_H 1

#include "ual_const.h"
#include "ual_lowlevel.h"


class access_layer_plugin {
   public:
    access_layer_plugin() {}
    virtual ~access_layer_plugin() {}

    virtual void setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) = 0;
    
    virtual void begin_global_action(int pulseCtx, const char* dataobjectname, int mode, int opCtx) = 0;
    virtual void begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) = 0;
    virtual void begin_arraystruct_action(int ctxID, int *actxID, const char* fieldPath, const char* timeBasePath, int *arraySize) = 0;

    virtual int read_data(int ctx, const char* fieldPath, const char* timeBasePath, void **data, int datatype, int dim, int *size) = 0;
    virtual void write_data(int ctxID, const char *field, const char *timebase, void *data, int datatype, int dim, int *size) = 0;
    //virtual void close_pulse(int pulseCtx, int mode) = 0;

};

typedef access_layer_plugin* create_t();
typedef void destroy_t(access_layer_plugin*);

#endif
