#include "hdf5_backend_factory.h"

#include "ual_backend.h"

HDF5BackendFactory::HDF5BackendFactory(std::string backend_version):backend_version_(backend_version)
{
}

HDF5BackendFactory::~HDF5BackendFactory()
{
}

std::unique_ptr < HDF5Writer > HDF5BackendFactory::createWriter(const std::string &options)
{
    if (backend_version_.compare("1.0") == 0) {
        std::unique_ptr < HDF5Writer > writer = std::unique_ptr < HDF5Writer > (new HDF5Writer(backend_version_, options));
        return std::move(writer);
    } else {
        std::string message("No backend writer with version: ");
        message += backend_version_;
        throw UALBackendException(message, LOG);
    }

}

std::unique_ptr < HDF5Reader > HDF5BackendFactory::createReader(const std::string &options)
{
    if (backend_version_.compare("1.0") == 0) {
        std::unique_ptr < HDF5Reader > reader = std::unique_ptr < HDF5Reader > (new HDF5Reader(backend_version_, options));
        return std::move(reader);
    } else {
        std::string message("No backend reader with version: ");
        message += backend_version_;
        throw UALBackendException(message, LOG);
    }
}

std::unique_ptr < HDF5EventsHandler > HDF5BackendFactory::createEventsHandler()
{
    if (backend_version_.compare("1.0") == 0) {
        auto eventsHandler = std::unique_ptr < HDF5EventsHandler > (new HDF5EventsHandler());
        return std::move(eventsHandler);
    } else {
        std::string message("No backend events handler with version: ");
        message += backend_version_;
        throw UALBackendException(message, LOG);
    }
}
