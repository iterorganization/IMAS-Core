#include "serialize_backend.h"

#include <algorithm>

std::pair<int, int> SerializeBackend::getVersion(DataEntryContext* ctx) {
  return std::pair<int, int>(1, 0);
}

void SerializeBackend::openPulse(DataEntryContext* ctx, int mode) {
    _cur_ctxid.push(ctx->getUid());

    _serializing = (mode != OPEN_PULSE);

    if (_serializing) {
        // Initialize internal buffer to 1 MB
        const std::size_t initial_size = 1 * 1024 * 1024;
        _builder = std::make_unique<flexbuffers::Builder>(
            initial_size, flexbuffers::BUILDER_FLAG_NONE
        );
    }
}

void SerializeBackend::closePulse(DataEntryContext* ctx, int mode) {
    // clear buffers
    if (_builder) _builder->Clear();
    _buffer.clear();
}

void SerializeBackend::beginAction(OperationContext* ctx) {
    if (ctx->getRangemode() == SLICE_OP)
        throw ALBackendException("Serialize Backend does not support slice mode", LOG);
    _cur_ctxid.push(ctx->getUid());

    if (_serializing) {
        _builder->Clear();
        // start a vector to store the IDS children in
        _start_vector();
    } else {
        auto root = flexbuffers::GetRoot(_buffer);
        _push_element_map(root.AsVector());
    }
}

void SerializeBackend::beginArraystructAction(
    ArraystructContext* ctx, int* size
) {
    _cur_ctxid.push(ctx->getUid());
    _cur_aos_index.push(0);

    if (_serializing) {
        if (*size <= 0) return;
        // serializing
        _builder->Key(ctx->getPath());
        // Start a vector to contain all AoS elements
        _start_vector();
        // Start a vector to contain nodes of AoS element 0
        _start_vector();
    } else {
        auto idx = _element_map.top().find(ctx->getPath());
        if (idx == _element_map.top().end()) {
            // not found:
            *size = 0;
            // set _cur_aos_index to -1 to signal that there's no element map pushed
            -- _cur_aos_index.top();
        } else {
            auto vector = _cur_vector.top()[idx->second + 1].AsVector();
            *size = vector.size();
            _cur_vector.push(vector);
            // Parse first element of the AoS
            _push_element_map(vector[0].AsVector());
        }
    }
}

void SerializeBackend::endAction(Context* ctx) {
    if (ctx->getUid() != _cur_ctxid.top())
        throw ALBackendException(
            "Unexpected nesting of contexts: ending " + std::to_string(ctx->getUid())
            + " top context is " + std::to_string(_cur_ctxid.top())
            , LOG
        );
    _cur_ctxid.pop();

    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        // End of an AoS
        if (_serializing) {
            // close structure vector
            _end_vector();
            // close the vector containing all aos structures
            _end_vector();
        } else if (_cur_aos_index.top() >= 0) {
            // Pop structure vector and its element map
            _element_map.pop();
            _cur_vector.pop();
            // Pop AoS vector
            _cur_vector.pop();
        } else {
            // _cur_aos_index.top() is negative if the AoS didn't exist, see
            // beginArraystructAction.
            // In this case there were no elements pushed to the stacks, so nothing
            // needs to be done
        }
        _cur_aos_index.pop();
    } else if (ctx->getType() == CTX_OPERATION_TYPE) {
        // End of put/get
        if (_serializing) {
            // close IDS structure vector
            _end_vector();
        } else {
            _element_map.pop();
            _cur_vector.pop();
        }
    }
}

void SerializeBackend::writeData(
    Context* ctx,
    std::string fieldname,
    std::string timebasename,
    void* data,
    int datatype,
    int dim,
    int* size
) {
    // handle special fieldname to set internal buffer
    if (fieldname == "<buffer>") {
        if (!_vector_starts.empty())
            throw ALBackendException("Incomplete vectors present", LOG);
        uint8_t *buffer = reinterpret_cast<uint8_t*>(data);
        if (*buffer != FLEXBUFFERS_SERIALIZER_PROTOCOL) {
            throw ALBackendException(
                "Serialize Backend: Unknown serializer protocol: "
                + std::to_string(int(*buffer)),
                LOG
            );
        }
        _buffer = std::vector<uint8_t>(buffer, buffer + (*size));
        return;
    }

    // handle serialization
    if (!_builder)
        throw ALBackendException("Writing data, but no builder available", LOG);

    // Check if we're in an AoS and have increased our index
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        ArraystructContext *aos = dynamic_cast<ArraystructContext *>(ctx);
        for (auto &idx = _cur_aos_index.top(); idx < aos->getIndex(); ++idx) {
            _end_vector();
            _start_vector();
        }
    }

    _builder->Key(fieldname);
    _builder->Int(datatype);
    _builder->Vector(size, std::size_t(dim));

    // Calculate product of all size elements
    std::size_t num_elements = 1;
    for (int i=0; i < dim; ++i) {
        num_elements = num_elements * size[i];
    }

    switch (datatype)
    {
    case CHAR_DATA:
        _builder->Blob(data, sizeof(char) * num_elements);
        break;
    case INTEGER_DATA:
        _builder->Blob(data, sizeof(int) * num_elements);
        break;
    case DOUBLE_DATA:
        _builder->Blob(data, sizeof(double) * num_elements);
        break;
    case COMPLEX_DATA:
        _builder->Blob(data, 2*sizeof(double) * num_elements);
        break;
    default:
        throw ALBackendException("Unsupported data type for the Serialize Backend",LOG);
        break;
    }
}

int SerializeBackend::readData(
    Context* ctx,
    std::string fieldname,
    std::string timebasename,
    void** data,
    int* datatype,
    int* dim,
    int* size
) {
    if (fieldname == "<buffer>") {
        if (!_builder)
            throw ALBackendException("Reading <buffer>, but no builder available", LOG);
        _builder->Finish();
        auto & buffer = _builder->GetBuffer();
        *datatype = CHAR_DATA;
        *dim = 1;
        *size = buffer.size() + 1;
        // allocate C memory for the data:
        int8_t *tmp = reinterpret_cast<int8_t *>(malloc(*size));
        tmp[0] = static_cast<int8_t>(FLEXBUFFERS_SERIALIZER_PROTOCOL);
        memcpy(tmp + 1, buffer.data(), buffer.size());
        // Assign to data
        *data = tmp;
        return 1;
    } 
    if (_builder)
        throw ALBackendException("Cannot read data when serializing.");

    // deserializing
    // First check if we're still looking at the correct AoS
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        ArraystructContext *aos = dynamic_cast<ArraystructContext *>(ctx);
        if (_cur_aos_index.top() != aos->getIndex()) {
            // Update cur index
            _cur_aos_index.top() = aos->getIndex();
            // Update the flexbuffers::Vector to match the current aos index
            _element_map.pop();
            _cur_vector.pop();
            _push_element_map(_cur_vector.top()[aos->getIndex()].AsVector());
        }
    }

    auto idx = _element_map.top().find(fieldname);
    if (idx == _element_map.top().end()) {
        // Fieldname not found
        return 0;
    }

    auto &vector = _cur_vector.top();
    int i = idx->second;
    // Set output
    *datatype = vector[i+1].AsInt32();
    auto sizes = vector[i+2].AsTypedVector();
    *dim = sizes.size();
    for( int j=0; j<*dim; ++j) {
        size[j] = sizes[j].AsInt32();
    }
    auto blob = vector[i+3].AsBlob();
    // Copy the blob into data
    *data = malloc(blob.size());
    memcpy(*data, blob.data(), blob.size());

    return 1;
}

void SerializeBackend::deleteData(OperationContext* ctx, std::string path) {
    // NOOP for serialization
}


void SerializeBackend::get_occurrences(
    const char* ids_name,
    int** occurrences_list,
    int* size
) {
    throw ALBackendException("get_occurrences is not implemented in the Serialize Backend", LOG);
}

void SerializeBackend::_start_vector() {
    _vector_starts.push(_builder->StartVector());
}

void SerializeBackend::_end_vector() {
    _builder->EndVector(_vector_starts.top(), false, false);
    _vector_starts.pop();
}

void SerializeBackend::_push_element_map(flexbuffers::Vector vector) {
    _cur_vector.push(vector);
    std::unordered_map<std::string, int> element_map;
    for (int i=0; i < vector.size(); ++i) {
        auto ele = vector[i];
        if (ele.IsKey()) {
            element_map[ele.AsKey()] = i;
        }
    }
    _element_map.push(element_map);
}
