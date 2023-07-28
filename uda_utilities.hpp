#pragma once

#ifndef IMAS_UDA_UTILITIES_HPP
#define IMAS_UDA_UTILITIES_HPP

/**
 * Utility functions used in the UDA backend.
 */

#include <deque>
#include <vector>
#include <unordered_map>
#include <boost/variant.hpp>

#include "al_const.h"
#include "al_context.h"

namespace imas {
namespace uda {

/**
 * Convertor struct used in convert_imas_to_uda function to convert backend id to string.
 */
struct BackendId {
    std::string operator()(int i) {
        switch (i) {
            case MDSPLUS_BACKEND: return "mdsplus";
            case HDF5_BACKEND: return "hdf5";
            case ASCII_BACKEND: return "ascii";
            default: throw std::runtime_error{"unknown backed: " + std::to_string(i)};
        }
    }
};

/**
 * Convertor struct used in convert_imas_to_uda function to convert open mode to string.
 */
struct OpenMode {
    std::string operator()(int i) {
        switch (i) {
            case OPEN_PULSE: return "open";
            case CREATE_PULSE: return "create";
            case FORCE_OPEN_PULSE: return "force_open";
            case FORCE_CREATE_PULSE: return "force_create";
            default: throw std::runtime_error{"unknown open mode: " + std::to_string(i)};
        }
    }
};

/**
 * Convertor struct used in convert_imas_to_uda function to convert close mode to string.
 */
struct CloseMode {
    std::string operator()(int i) {
        switch (i) {
            case CLOSE_PULSE: return "close";
            case ERASE_PULSE: return "erase";
            default: throw std::runtime_error{"unknown close mode: " + std::to_string(i)};
        }
    }
};

/**
 * Convertor struct used in convert_imas_to_uda function to convert access mode to string.
 */
struct AccessMode {
    std::string operator()(int i) {
        switch (i) {
            case READ_OP: return "read";
            case WRITE_OP: return "write";
            case REPLACE_OP: return "replace";
            default: throw std::runtime_error{"unknown access mode: " + std::to_string(i)};
        }
    }
};

/**
 * Convertor struct used in convert_imas_to_uda function to convert range mode to string.
 */
struct RangeMode {
    std::string operator()(int i) {
        switch (i) {
            case GLOBAL_OP: return "global";
            case SLICE_OP: return "slice";
            default: throw std::runtime_error{"unknown range mode: " + std::to_string(i)};
        }
    }
};

/**
 * Convertor struct used in convert_imas_to_uda function to convert interp mode to string.
 */
struct InterpMode {
    std::string operator()(int i) {
        switch (i) {
            case UNDEFINED_INTERP: return "undefined";
            case CLOSEST_INTERP: return "closest";
            case PREVIOUS_INTERP: return "previous";
            case LINEAR_INTERP: return "linear";
            default: throw std::runtime_error{"unknown interp mode: " + std::to_string(i)};
        }
    }
};

/**
 * Convertor struct used in convert_imas_to_uda function to convert data type to string.
 */
struct DataType {
    std::string operator()(int i) {
        switch (i) {
            case CHAR_DATA: return "char";
            case INTEGER_DATA: return "integer";
            case DOUBLE_DATA: return "double";
            case COMPLEX_DATA: return "complex";
            default: throw std::runtime_error{"unknown data type: " + std::to_string(i)};
        }
    }
};

/**
 * Convertor struct used in convert_imas_to_uda function to convert context type to string.
 */
struct ContextType {
    std::string operator()(int i) {
        switch (i) {
            case CTX_PULSE_TYPE: return "pulse";
            case CTX_OPERATION_TYPE: return "operation";
            case CTX_ARRAYSTRUCT_TYPE: return "arraystruct";
            default: throw std::runtime_error{"unknown context type: " + std::to_string(i)};
        }
    }
};

/**
 * Helper function which can be used to convert IMAS constants into string values.
 *
 * Example:
 *  convert_imas_to_uda<RangeMode>(GLOBAL_OP)
 *
 * @tparam T the converter struct to use
 * @param i the IMAS constant to convert
 * @return a string representation of the IMAS constant
 */
template <typename T>
std::string convert_imas_to_uda(int i)
{
    return T()(i);
}

}} // namespace imas::uda

#endif // IMAS_UDA_UTILITIES_HPP
