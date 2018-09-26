#include "hdf5_backend.h"

HDF5Backend::HDF5Backend()
{
}

HDF5Backend::HDF5Backend(Backend *targetB) 
{
}

HDF5Backend::~HDF5Backend()
{
}

void HDF5Backend::openPulse(PulseContext *ctx, int mode, std::string options)
{
	throw UALBackendException("openPulse method has to be completed for HDF5 Backend",LOG);
}

void HDF5Backend::closePulse(PulseContext *ctx, int mode, std::string options)
{
	throw UALBackendException("closePulse method has to be completed for HDF5 Backend",LOG);
}

void HDF5Backend::writeData(Context *ctx, std::string fieldname, std::string timebasename, void* data, int datatype, int dim, int* size)
{
	throw UALBackendException("writeData method has to be completed for HDF5 Backend",LOG);
}

void HDF5Backend::readData(Context *ctx, std::string fieldname, std::string timebase, void** data, int* datatype, int* dim, int* size)
{
	throw UALBackendException("readData method has to be completed for HDF5 Backend",LOG);
}

void HDF5Backend::deleteData(OperationContext *ctx, std::string path)
{
	throw UALBackendException("deleteData method has to be completed for HDF5 Backend",LOG);
}

void HDF5Backend::beginArraystructAction(ArraystructContext *ctx, int *size)
{
	throw UALBackendException("beginArraystructAction method has to be completed for HDF5 Backend",LOG);
}

<<<<<<< HEAD
void HDF5Backend::endAction(Context *ctx)
=======
void Hdf5Backend::endAction(Context *ctx)
{
<<<<<<< HEAD
void HDF5Backend::beginAction(OperationContext *ctx)
=======
void Hdf5Backend::beginAction(OperationContext *ctx)
>>>>>>> Fix to compile and execute correctly lowlevel with static and shared MDSplus libraries
{
	throw UALBackendException("beginAction method has to be completed for HDF5 Backend",LOG);
}
