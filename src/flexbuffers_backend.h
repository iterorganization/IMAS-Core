#ifndef FLEXBUFFERS_BACKEND_H
#define FLEXBUFFERS_BACKEND_H

#include "al_backend.h"

#include <cstdint>
#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>

// Locale dependent flexbuffers causes an issue during linking, disable it:
#define FLATBUFFERS_LOCALE_INDEPENDENT 0
#include "flatbuffers/flexbuffers.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

class LIBRARY_API FlexbuffersBackend : public Backend
{
  public:
    FlexbuffersBackend() {};
    virtual ~FlexbuffersBackend() {};

    // Implement Backend functions
    std::pair<int,int> getVersion(DataEntryContext *ctx) override;
    void openPulse(DataEntryContext *ctx, int mode) override;
    void closePulse(DataEntryContext *ctx, int mode) override;
    void beginAction(OperationContext *ctx) override;
    void endAction(Context *ctx) override; 
    void writeData(Context *ctx, 
        std::string fieldname,
        std::string timebasename, 
        void* data,
        int datatype,
        int dim,
        int* size) override;
    int readData(Context *ctx,
        std::string fieldname,
        std::string timebasename, 
        void** data,
        int* datatype,
        int* dim,
        int* size) override;
    void deleteData(OperationContext *ctx, std::string path) override;
    void beginArraystructAction(ArraystructContext *ctx, int *size) override;
    void get_occurrences(const char* ids_name, int** occurrences_list, int* size) override;
    // timerange (get_sample) API is not supported:
    bool performsTimeDataInterpolation() override { return false; }
    bool supportsTimeRangeOperation() override { return false; }
    void setDataInterpolationComponent(DataInterpolation *component) override {
      throw ALBackendException("Flexbuffers backend does not support time slices operations",LOG);
    }

  private:
    /* Indicate if we are in serializing mode, or in deserializing */
    bool _serializing;
    /* Stack for sanity checking that contexts are correctly nested */
    std::stack<unsigned long int> _cur_ctxid;
    /* Track the last observed index in the current Array of Structures context */
    std::stack<int> _cur_aos_index;

    // Attributes for serializing
    /* Flexbuffer builder for serializing */
    std::unique_ptr<flexbuffers::Builder> _builder;
    /* Track start positions of opened vectors, required for ending them */
    std::stack<std::size_t> _vector_starts;

    // Attributes for deserializing
    /* Buffer containing the data to deserialize */
    std::vector<uint8_t> _buffer;
    /* Mapping of fieldnames to locations in the current flexbuffer::Vector */
    std::stack<std::unordered_map<std::string, int>> _element_map;
    /* Current active vector */
    std::stack<flexbuffers::Vector> _cur_vector;

    /* Helper method when serializing: start a new vector */
    void _start_vector();
    /* Helper method when serializing: end a vector */
    void _end_vector();
    /* Helper method when deserializing: create a map of element names for the structure
     * contained in the provided vector
     */
    void _push_element_map(flexbuffers::Vector vector);
    /** Helper method to check if the current AoS index has changed and act
     * accordingly */
    void _check_aos_index(Context* ctx);
};

#endif  // FLEXBUFFERS_BACKEND_H
