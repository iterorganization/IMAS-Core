#include "parameter_plugin.h"

//#include "simple_logger.h"

ParameterPlugin::ParameterPlugin()
:AL_reader_helper_plugin(), op(2)
{
}

ParameterPlugin::~ParameterPlugin()
{
}

std::string ParameterPlugin::getName() {
    return "parameter";
}

std::string ParameterPlugin::getDescription() {
    return "";
}

std::string ParameterPlugin::getVersion() {
    return "1.0.0";
}

plugin::OPERATION ParameterPlugin::node_operation(const std::string &path) {
    return plugin::OPERATION::GET_ONLY;
}


void ParameterPlugin::setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) {
   if (dim > 2) 
       throw ALPluginException("Parameter dimension (>2) not supported!");
   if (dim == 2) {
       if (size[0] > 1 && size[1] > 1)
            throw ALPluginException("Matrix are not supported!");
       else if (size[0] == 1 && size[1] == 1)
            dim = 0;
       else
            dim = 1;
   }
   
   if (dim == 0) {
      double* p = (double*) data;
      if (std::string(parameter_name) == std::string("op"))
        printf("setting parameter :%s to %d\n", parameter_name, (int) p[0]); 
        op = (int) p[0];      
   }
   else if(dim == 1) {
     double* p = (double*) data;
     int len = size[1];
     if (size[0] > len)
          len = size[0];
     for (int i = 0; i < len; i++) {
        printf("p[%d]=%d\n", i, (int) p[i]);       
     } 
   }
}
