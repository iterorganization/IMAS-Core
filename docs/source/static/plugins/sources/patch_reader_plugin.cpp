#include "patch_reader_plugin.h"

#include "simple_logger.h"

PatchReaderPlugin::PatchReaderPlugin()
:AL_reader_helper_plugin(), op(2)
{
}

PatchReaderPlugin::~PatchReaderPlugin()
{
}

std::string PatchReaderPlugin::getName() {
    return "nbc";
}

std::string PatchReaderPlugin::getDescription() {
    return "";
}

std::string PatchReaderPlugin::getVersion() {
    return "1.0.0";
}

plugin::OPERATION PatchReaderPlugin::node_operation(const std::string &path) {
    return plugin::OPERATION::GET_ONLY;
}

void PatchReaderPlugin::setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) {
   if (dim == 0) {
      int* p = (int*) data;
      if (std::string(parameter_name) == std::string("op"))
        LOG_DEBUG << "setting op parameter :" << parameter_name << " to: " << p[0]; 
        op = p[0];      
   }
   else {
     LOG_DEBUG << "Patch reader plugin supports only one scalar parameter ('op')"; 
   }
}

int PatchReaderPlugin::read_data(int ctx, const char* fieldPath, const char* timeBasePath, 
                void **data, int datatype, int dim, int *size) {
    if (dim == 2) { //in this example, we are handling only 2D data
        al_plugin_read_data(ctx, fieldPath, timeBasePath, data, datatype, dim, size); //reading data from the backend
        double *p = (double*) *data;
        for (int j = 0; j < size[1]; j++)
          for (int i = 0; i< size[0]; i++)
            p[i+j*size[0]]*=(double)op;
        return 1; //data available
    }
    return 0;
}
