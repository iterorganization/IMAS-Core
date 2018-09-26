#include "hdf5_backend.h"

Hdf5Backend::Hdf5Backend() 
{
}

void Hdf5Backend::openPulse(PulseContext *ctx, int mode, std::string options)
{
	throw UALBackendException("openPulse method has to be completed for HDF5 Backend",LOG);
}

void Hdf5Backend::closePulse(PulseContext *ctx, int mode, std::string options)
{
	throw UALBackendException("closePulse method has to be completed for HDF5 Backend",LOG);
}

void Hdf5Backend::writeData(Context *ctx, std::string fieldname, std::string timebasename, void* data, int datatype, int dim, int* size)
{
	throw UALBackendException("writeData method has to be completed for HDF5 Backend",LOG);
}

void Hdf5Backend::readData(Context *ctx, std::string fieldname, std::string timebase, void** data, int* datatype, int* dim, int* size)
{
	throw UALBackendException("readData method has to be completed for HDF5 Backend",LOG);
}

void Hdf5Backend::deleteData(OperationContext *ctx, std::string path)
{
	throw UALBackendException("deleteData method has to be completed for HDF5 Backend",LOG);
}

void Hdf5Backend::beginArraystructAction(ArraystructContext *ctx, int *size)
{
	throw UALBackendException("beginArraystructAction method has to be completed for HDF5 Backend",LOG);
}

void Hdf5Backend::endAction(Context *ctx)
{
	throw UALBackendException("endAction method has to be completed for HDF5 Backend",LOG);
}

void Hdf5Backend::beginAction(OperationContext *ctx)
{
	throw UALBackendException("beginAction method has to be completed for HDF5 Backend",LOG);
}
