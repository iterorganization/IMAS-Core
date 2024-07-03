#ifndef UDA_BACKEND_H
#define UDA_BACKEND_H 1

#include <sstream>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <cstdlib>
#include <unordered_map>
//#include <mdsobjects.h>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <c++/UDA.hpp>
#include <fstream>
#include <stdlib.h>
#include <boost/cstdlib.hpp>

#include "al_backend.h"
#include "al_const.h"
#include "al_context.h"
#include "al_defs.h"
#include "uda_xml.hpp"
#include "uda_cache.hpp"

#define NODENAME_MANGLING  //Use IMAS mangling

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus

static const int UDA_BACKEND_VERSION_MAJOR = 0;
static const int UDA_BACKEND_VERSION_MINOR = 0;

namespace imas {
namespace uda {

/**
 * Cache mode specifying how the backend should behave.
 *
 * None   - no caching will be performed
 * IDS    - pre-caching will be performed for the entire IDS at the start of the request
 * Struct - pre-caching will be performed for each separate struct_array in the begin_structarray step
 */
enum class CacheMode
{
    None,
    IDS,
    Struct,
};

}
}

/**
 * UDA Backend for IMAS access layer.
 *
 * This is the backend for passing access layer requests to a UDA server where it can be handled by the IMAS plugin
 * from the ITER UDA plugins repo (https://git.iter.org/projects/IMAS/repos/uda-plugins/browse/source) to either return
 * remote IMAS data or mapped data from experimental databases.
 */
class LIBRARY_API UDABackend : public Backend
{
private:
    bool verbose_ = false;
    std::string plugin_ = "IMAS";
    uda::Client uda_client_;
    std::shared_ptr<pugi::xml_document> doc_ = {};
    imas::uda::CacheType cache_ = {};
    imas::uda::CacheMode cache_mode_ = imas::uda::CacheMode::IDS;
    int open_mode_ = 0;
    std::string dd_version_ = "";

    /**
     * Process any UDA backend specific options found on the DBEntry URI.
     *
     * @param uri the uri of the DBEntry
     * @throw ALException if any of the passed options have invalid values
     */
    void process_options(uri::Uri uri);

    /**
     * Generate all requests for the given `ids` starting at the given `path` and send these requests to the UDA server,
     * storing all returned data responses in a RAM-cache ready to be read in future calls to `readData`.
     *
     * @param ids the IDS we are reading
     * @param path the top level path in the IDS the requests from which the requests are generated
     * @param pulse_ctx the pulse context
     * @param op_ctx the operation context
     */
    void populate_cache(const std::string& ids, const std::string& path, DataEntryContext* pulse_ctx, OperationContext* op_ctx);

    /**
     * Read the value of the homogeneous flag.
     *
     * @param ids the IDS we are reading the flag for
     * @param pulse_ctx the pulse context
     * @param op_ctx the operation context
     * @return
     */
    bool get_homogeneous_flag(const std::string& ids, DataEntryContext* pulse_ctx, OperationContext* op_ctx);

public:

    /**
     * Construct a new UDA backend.
     *
     * @param verb flag to set the verbose mode
     */
    explicit UDABackend(bool verb=false)
        : verbose_(verb)
        , uda_client_{}
    {
        const char* env = getenv("IMAS_UDA_PLUGIN");
        if (env != nullptr) {
            plugin_ = env;
        }

        doc_ = imas::uda::load_xml();
        dd_version_ = imas::uda::get_dd_version(doc_);

        if (verbose_) {
            std::cout << "UDABackend constructor\n";
            std::cout << "UDA default plugin: " << plugin_ << "\n";
            std::cout << "IMAS data dictionary version: " << dd_version_ << "\n";
        }
    }

    ~UDABackend() override
    {
        if (verbose_) {
            std::cout << "UDABackend destructor\n";
        }
    }

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

    void deleteData(OperationContext *ctx,
                    std::string path) override;

    void beginArraystructAction(ArraystructContext* ctx, int* size) override;

    std::pair<int,int> getVersion(DataEntryContext *ctx) override;

    void get_occurrences(const char* ids_name, int** occurrences_list, int* size) override;

    bool performsTimeDataInterpolation() {
      return false;
    }

    bool supportsTimeRangeOperation() {
	  return false;
	}

    void setDataInterpolationComponent(DataInterpolation *component) {
      throw ALBackendException("UDA backend does not support time slices operations",LOG);
    }

    
};

#endif

#endif // UDA_BACKEND_H

