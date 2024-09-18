#ifndef EXTENDED_ACCESS_LAYER_PLUGIN_H
#define EXTENDED_ACCESS_LAYER_PLUGIN_H 1

#include "access_layer_plugin.h"

class extended_access_layer_plugin : public access_layer_plugin {

    public:
    extended_access_layer_plugin() {}
    virtual ~extended_access_layer_plugin() {}
    
    virtual void begin_timerange_action(int pulseCtx, const char* dataobjectname, int mode, double tmin, double tmax, std::vector<double> dtime, int interp, int opCtx) = 0;
};

#endif