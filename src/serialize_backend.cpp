#include "serialize_backend.h"

#include <algorithm>
#include <sstream>

#define ENDIAN_MARKER_VALUE uint32_t(0x01020304)


/**
 * IMAS Flexbuffers serialization format
 * =====================================
 *
 * This serialization format was designed for speed and simplicity. We use the
 * Flexbuffers schemaless binary serialization format. Compared to schema-full
 * alternatives (like flatbuffers, Cap'n'proto and others) this adds some
 * overhead to store data types. This overhead is typically negligible compared
 * to the actual data transferred. The main benefit of Flexbuffers is that it's
 * a library-only header, making this more portable solution with a small binary
 * footprint.
 *
 * IDSs are serialized as nested Flexbuffer Vectors, mimicking the structures
 * and arrays of structures that IDSs consist of.
 *
 * An IDS is serialized as:
 *      [endian_marker, fieldname1, {field1}, fieldname2, {field2}, ...],
 * where:
 * 
 * - endian_marker is the binary representation (flexbuffers::Blob) of the
 *   integer 0x01020304. The serializer backend does not support byte swapping.
 *   Instead an error is raised if the endianness is not as expected.
 * - fieldname1, fieldname2, etc. are flexbuffers::Keys indicating the fieldname
 *   of the data that follows. These can be, for example
 *   "ids_properties/comment", "profiles_1d", etc.
 * - {field1}, {field2}, etc. depend on the type of data stored:
 *      - If the field is a data node, this expands to three items:
 *          1. datatype (flexbuffers::Int), one of CHAR_DATA, INTEGER_DATA,
 *             DOUBLE_DATA or COMPLEX_DATA
 *          2. size array (flexbuffers::TypedVector of type int), indicating the
 *             dimensionality and size in each dimension of the value.
 *          3. data (flexbuffers::Blob), the binary representation of the
 *             underlying data (array)
 *      - If the field is an array of structures, this is a single item: a
 *        flexbuffers::Vector with one item per structure in the AoS.
 * 
 *        The structure is another Vector, with a similar structure as the root
 *        IDS, except that the endian_marker is omitted.
 * 
 * Example:
 * --------
 * 
 * For a very small example core_profiles IDS
 *      core_profiles
 *      ├── ids_properties
 *      │   ├── comment: 'Example IDS for serialization'
 *      │   ├── homogeneous_time: 1
 *      │   └── ids_properties/version_put
 *      │       ├── data_dictionary: '3.41.0'
 *      │       ├── access_layer: '5.2.7'
 *      │       └── access_layer_language: 'imaspy 1.0.0+208.g80b0611'
 *      ├── profiles_1d[0]
 *      │   ├── profiles_1d[0]/grid
 *      │   │   └── rho_tor_norm: array([0. , 0.5, 1. ])
 *      │   └── t_i_average: array([3., 2., 1.])
 *      ├── profiles_1d[1]
 *      │   ├── profiles_1d[1]/grid
 *      │   │   └── rho_tor_norm: array([0. , 0.5, 1. ])
 *      │   └── t_i_average: array([4., 3., 2.])
 *      └── time: array([0.])
 * 
 * The serialized format can be inspected with the Python API of flexbuffers.
 * The output is annotated to highlight different elements
 *      $ python
 *      >>> data = ids.serialize()
 *      >>> import flatbuffers.flexbuffers
 *      >>> flatbuffers.flexbuffers.GetRoot(data).Value
 *      [
 *          b'\x04\x03\x02\x01',                #< The endian marker, little-endian representation of 0x01020304
 *          'ids_properties/comment',           #< Field name of the first data field
 *          50,                                 #< Data type (CHAR_DATA)
 *          [29],                               #< Size array, 1D with length 29
 *          b'Example IDS for serialization',   #< Blob representation of the data
 *          'ids_properties/homogeneous_time',  #< Field name of the second data field
 *          51,                                 #< Data type (INTEGER_DATA)
 *          [],                                 #< Size array, 0D element
 *          b'\x01\x00\x00\x00',                #< Blob representation of 1
 *          'ids_properties/version_put/data_dictionary',
 *          50,
 *          [6],
 *          b'3.41.0',
 *          'ids_properties/version_put/access_layer',
 *          50,
 *          [5],
 *          b'5.2.7',
 *          'ids_properties/version_put/access_layer_language',
 *          50,
 *          [25],
 *          b'imaspy 1.0.0+208.g80b0611',
 *          'profiles_1d',                      #< Field name of profiles_1d array of structures
 *          [                                   #< This Vector marks the start of the AoS, with two containing structures
 *              [                               #< Structure serializing profiles_1d[0]
 *                  'grid/rho_tor_norm',        #< First data node in profiles_1d[0]
 *                  52,                         #< Data type (DOUBLE_DATA)
 *                  [3],                        #< Size array, 1D with shape (3,)
 *                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xe0?\x00\x00\x00\x00\x00\x00\xf0?',
 *                  't_i_average',
 *                  52,
 *                  [3],
 *                  b'\x00\x00\x00\x00\x00\x00\x08@\x00\x00\x00\x00\x00\x00\x00@\x00\x00\x00\x00\x00\x00\xf0?'
 *              ],
 *              [                               #< Structure serializing profiles_1d[1]
 *                  'grid/rho_tor_norm',
 *                  52,
 *                  [3],
 *                  b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xe0?\x00\x00\x00\x00\x00\x00\xf0?',
 *                  't_i_average',
 *                  52,
 *                  [3],
 *                  b'\x00\x00\x00\x00\x00\x00\x10@\x00\x00\x00\x00\x00\x00\x08@\x00\x00\x00\x00\x00\x00\x00@'
 *              ]
 *          ],
 *          'time',
 *          52,
 *          [1],
 *          b'\x00\x00\x00\x00\x00\x00\x00\x00'
 *      ]
 */


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
        uint32_t endian_marker = ENDIAN_MARKER_VALUE;
        _builder->Blob(reinterpret_cast<void*>(&endian_marker), 4);
    } else {
        auto root = flexbuffers::GetRoot(_buffer);
        _push_element_map(root.AsVector());
        // Check that we have the same endian-ness as the machine that
        // serialized
        uint32_t endian_marker = *reinterpret_cast<const uint32_t*>(_cur_vector.top()[0].AsBlob().data());
        if (endian_marker != ENDIAN_MARKER_VALUE) {
            std::stringstream ss;
            ss << "Error when deserializing data: expected endian marker 0x";
            ss << std::hex << ENDIAN_MARKER_VALUE;
            ss << ", got 0x" << std::hex << endian_marker << ".";
            _cur_ctxid.pop();
            throw ALBackendException(ss.str());
        }
    }
}

void SerializeBackend::beginArraystructAction(
    ArraystructContext* ctx, int* size
) {
    _cur_ctxid.push(ctx->getUid());
    if (ctx->getParent() != NULL) {
        // A nested AoS action can be in a different index than we're
        // currently in:
        _check_aos_index(ctx->getParent());
    }
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
    _check_aos_index(ctx);

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
    _check_aos_index(ctx);

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

void SerializeBackend::_check_aos_index(Context *ctx) {
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        ArraystructContext *aos = dynamic_cast<ArraystructContext *>(ctx);

        if (_serializing) {
            for (auto &idx = _cur_aos_index.top(); idx < aos->getIndex(); ++idx) {
                _end_vector();
                _start_vector();
            }
        } else {
            if (_cur_aos_index.top() != aos->getIndex()) {
                // Update cur index
                _cur_aos_index.top() = aos->getIndex();
                // Update the flexbuffers::Vector to match the current aos index
                _element_map.pop();
                _cur_vector.pop();
                _push_element_map(_cur_vector.top()[aos->getIndex()].AsVector());
            }
        }
    }
}
