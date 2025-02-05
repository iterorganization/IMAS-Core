
/**
 * @file al_const.h
 * @brief Contains all constants used within the AL low-levels.
 */

#ifndef AL_CONST_H
#define AL_CONST_H 1

#include "al_defs.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum BACKEND
 * @brief Enumeration of backend types.
 */
enum BACKEND
{
  NO_BACKEND = BACKEND_ID_0, /**< No backend */
  ASCII_BACKEND = BACKEND_ID_0 + 1, /**< ASCII backend */
  MDSPLUS_BACKEND = BACKEND_ID_0 + 2, /**< MDSPlus backend */
  HDF5_BACKEND = BACKEND_ID_0 + 3, /**< HDF5 backend */
  MEMORY_BACKEND = BACKEND_ID_0 + 4, /**< Memory backend */
  UDA_BACKEND = BACKEND_ID_0 + 5, /**< UDA backend */
  FLEXBUFFERS_BACKEND = BACKEND_ID_0 + 6 /**< Flexbuffers backend */
};

#ifdef __cplusplus
}

#include <array>
#include <map>
#include <string>

namespace alconst {

  // Constants for backend types
  const int no_backend = BACKEND::NO_BACKEND;
  const int ascii_backend = BACKEND::ASCII_BACKEND;
  const int mdsplus_backend = BACKEND::MDSPLUS_BACKEND;
  const int hdf5_backend = BACKEND::HDF5_BACKEND;
  const int memory_backend = BACKEND::MEMORY_BACKEND;
  const int uda_backend = BACKEND::UDA_BACKEND;
  const int flexbuffers_backend = BACKEND::FLEXBUFFERS_BACKEND;

  // Constants for operations
  const int global_op = GLOBAL_OP;
  const int slice_op = SLICE_OP;
  const int timerange_op = TIMERANGE_OP;

  const int read_op = READ_OP;
  const int write_op = WRITE_OP;

  // Constants for interpolation types
  const int closest_interp = CLOSEST_INTERP;
  const int previous_interp = PREVIOUS_INTERP;
  const int linear_interp = LINEAR_INTERP;
  const int undefined_interp = UNDEFINED_INTERP;

  // Constants for time
  const double undefined_time = UNDEFINED_TIME;

  // Constants for pulse operations
  const int open_pulse = OPEN_PULSE;
  const int force_open_pulse = FORCE_OPEN_PULSE;
  const int create_pulse = CREATE_PULSE;
  const int force_create_pulse = FORCE_CREATE_PULSE;
  const int close_pulse = CLOSE_PULSE;
  const int erase_pulse = ERASE_PULSE;

  // Constants for data types
  const int char_data = CHAR_DATA;
  const int integer_data = INTEGER_DATA;
  const int double_data = DOUBLE_DATA;
  const int complex_data = COMPLEX_DATA;

  // Constants for serializer protocols
  const int ascii_serializer_protocol = ASCII_SERIALIZER_PROTOCOL;
  const int flexbuffers_serializer_protocol = FLEXBUFFERS_SERIALIZER_PROTOCOL;
  const int default_serializer_protocol = DEFAULT_SERIALIZER_PROTOCOL;

  // Arrays of constants
const std::array<int,6> backend_id_list =
    {
      {
	NO_BACKEND,
	ASCII_BACKEND,
	MDSPLUS_BACKEND,
	HDF5_BACKEND,
	MEMORY_BACKEND,
	UDA_BACKEND
      }
    };
  const std::array<int,3> op_range_list =
    {
      {
	GLOBAL_OP,
	SLICE_OP,
  TIMERANGE_OP
      }
    };
  const std::array<int,3> op_access_list =
    {
      {
	READ_OP,
	WRITE_OP,
      }
    };
  const std::array<int,4> op_interp_list =
    {
      {
	UNDEFINED_INTERP,
	CLOSEST_INTERP,
	PREVIOUS_INTERP,
	LINEAR_INTERP
      }
    };
  const std::array<int,6> access_pulse_list =
    {
      {
	OPEN_PULSE,
	FORCE_OPEN_PULSE,
	CREATE_PULSE,
	FORCE_CREATE_PULSE,
	CLOSE_PULSE,ERASE_PULSE
      }
    };
  const std::array<int,4> data_type_list =
    {
      {CHAR_DATA,
       INTEGER_DATA,
       DOUBLE_DATA,
       COMPLEX_DATA
      }
    };
  const std::array<int,2> serializer_protocol_list =
    {
      {
	ASCII_SERIALIZER_PROTOCOL,
  FLEXBUFFERS_SERIALIZER_PROTOCOL
      }
    };
  
  // Maps of constants to strings
  const std::map<int,std::string> constmap =
    {
      {NO_BACKEND, "NO_BACKEND"},
      {ASCII_BACKEND, "ASCII_BACKEND"},
      {MDSPLUS_BACKEND, "MDSPLUS_BACKEND"},
      {HDF5_BACKEND, "HDF5_BACKEND"},
      {MEMORY_BACKEND, "MEMORY_BACKEND"},
      {UDA_BACKEND, "UDA_BACKEND"},
      {GLOBAL_OP, "GLOBAL_OP"},
      {SLICE_OP, "SLICE_OP"},
      {READ_OP, "READ_OP"},
      {WRITE_OP, "WRITE_OP"},
      {REPLACE_OP, "REPLACE_OP"},
      {UNDEFINED_INTERP, "UNDEFINED_INTERP"},
      {CLOSEST_INTERP, "CLOSEST_INTERP"},
      {PREVIOUS_INTERP, "PREVIOUS_INTERP"},
      {LINEAR_INTERP, "LINEAR_INTERP"},
      {UNDEFINED_TIME, "UNDEFINED_TIME"},
      {OPEN_PULSE, "OPEN_PULSE"},
      {FORCE_OPEN_PULSE, "FORCE_OPEN_PULSE"},
      {CREATE_PULSE, "CREATE_PULSE"},
      {FORCE_CREATE_PULSE, "FORCE_CREATE_PULSE"},
      {CLOSE_PULSE, "CLOSE_PULSE"},
      {ERASE_PULSE, "ERASE_PULSE"},
      {CHAR_DATA, "CHAR_DATA"},
      {INTEGER_DATA, "INTEGER_DATA"},
      {DOUBLE_DATA, "DOUBLE_DATA"},
      {COMPLEX_DATA, "COMPLEX_DATA"},
      {ASCII_SERIALIZER_PROTOCOL, "ASCII_SERIALIZER_PROTOCOL"},
      {FLEXBUFFERS_SERIALIZER_PROTOCOL, "FLEXBUFFERS_SERIALIZER_PROTOCOL"}
    };
} // namespace alconst

namespace alerror {

  // Constants for error types
  const int unknown_err = UNKNOWN_ERR;
  const int context_err = CONTEXT_ERR;
  const int backend_err = BACKEND_ERR;
  const int lowlevel_err = LOWLEVEL_ERR;

  // Maps of errors to strings
  const std::map<int,std::string> errmap =
    {
      {UNKNOWN_ERR, "UNKNOWN_ERR"},
      {CONTEXT_ERR, "CONTEXT_ERR"},
      {BACKEND_ERR, "BACKEND_ERR"},
      {LOWLEVEL_ERR, "LOWLEVEL_ERR"}
    };
} // namespace alerror

namespace plugin {

  /**
   * @enum OPERATION
   * @brief Enumeration of plugin operations.
   */
  enum OPERATION
  {
  PUT_ONLY = 1, /**< Put only operation */
  GET_ONLY = 2, /**< Get only operation */
  PUT_AND_GET = 3 /**< Put and get operation */
  };

} // namespace plugin

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns the string of the passed constant identifier.
 * @param[in] id Constant ID
 * @return String associated with the constant ID
 */
LIBRARY_API const char *const2str(int id);

/**
 * @brief Returns the string of the passed integer error.
 * @param[in] id Error ID
 * @return String associated with the error ID
 */
LIBRARY_API const char *err2str(int id);

/**
 * @brief Returns the string of the AL version.
 * @return String of the AL version
 */
LIBRARY_API const char *getALVersion();

/**
 * @brief Returns the string of the Data Dictionary version.
 * @return String of the Data Dictionary version
 */
LIBRARY_API const char *getDDVersion();

#if defined(__cplusplus)
}
#endif

#endif // AL_CONST_H
