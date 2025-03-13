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
#include <cstdlib>
#include <filesystem>
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
    bool fetch_ = false;
    bool access_local_ = false;
    std::string plugin_ = "IMAS";
    uda::Client uda_client_;
    std::shared_ptr<pugi::xml_document> doc_ = {};
    imas::uda::CacheType cache_ = {};
    imas::uda::CacheMode cache_mode_ = imas::uda::CacheMode::IDS;
    int open_mode_ = 0;
    std::string dd_version_ = "";

    DataEntryContext* local_ctx_ = nullptr;
    Backend* backend_ = nullptr;

    // Cache for homogenous time value for non-cached reads
    std::unordered_map<std::string, bool> homogeneous_time_ = {};

    // Cached values used to read HDF5 files on demand
    std::filesystem::path remote_path_ = {};
    std::filesystem::path local_path_ = {};
    std::vector<std::string> ids_filenames_ = {};

    /**
     * Process any UDA backend specific options found on the DBEntry URI.
     *
     * @param uri the uri of the DBEntry
     * @throw ALException if any of the passed options have invalid values
     */
    void process_options(const uri::Uri& uri);

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
     * @return the homogeneous flag
     */
    bool get_homogeneous_flag(const std::string& ids, DataEntryContext* pulse_ctx, OperationContext* op_ctx);

    /**
     * Fetch the underlying IDS data files and then do all data access locally.
     *
     * @param local_path the path to the directory in which to save the data locally
     * @param remote_path the path to the data on the remote server
     * @param backend the backend used to access the data on the remote server
     * @return if the fetch was successful
     */
    bool fetch_files(const std::string& backend);

    /**
     * Fetch the underlying IDS data files and then do all data access locally.
     */
    void fetch_files(const uri::Uri& uri);

    void download_file(const std::string& filename);

public:

    /**
     * Construct a new UDA backend.
     *
     * @param uri the URI of the data entry that the backend is being used to read
     */
    explicit UDABackend(const uri::Uri& uri);

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

    void get_occurrences(Context* ctx, const char* ids_name, int** occurrences_list, int* size) override;

    bool supportsTimeDataInterpolation();

    // Do nothing, UDA plugin will need to initDataInterpolationComponent on data backend when it knows which backend
      // is being used, i.e. when a URI is given.
    void initDataInterpolationComponent() {}
      
    bool supportsTimeRangeOperation() {
        return this->supportsTimeDataInterpolation();
    }
    
};

#endif

#endif // UDA_BACKEND_H

