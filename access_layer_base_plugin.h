#ifndef AL_BASE_PLUGIN_H
#define AL_BASE_PLUGIN_H 1

#include "al_const.h"
#include "al_lowlevel.h"
#include <assert.h>
#include "provenance_plugin_feature.h"


class access_layer_base_plugin: public provenance_plugin_feature
{

private:
    std::string parameters;

public:
    access_layer_base_plugin() {}
    virtual ~access_layer_base_plugin() {}

    virtual void setParameter(const char *parameter_name, int datatype, int dim, int *size, void *data) = 0;
    void setParameters(const std::string &parameters){
        this->parameters = parameters;
    } 
};

typedef access_layer_base_plugin *create_t();
typedef void destroy_t(access_layer_base_plugin *);

#endif