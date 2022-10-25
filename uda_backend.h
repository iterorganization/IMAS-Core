#ifndef UDA_BACKEND_H
#define UDA_BACKEND_H 1

#include <sstream>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <cstdlib>
#include <unordered_map>
#include <mdsobjects.h>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <c++/UDA.hpp>
#include <fstream>
#include <stdlib.h>
#include <boost/cstdlib.hpp>

#include "ual_backend.h"
#include "ual_const.h"
#include "ual_context.h"
#include "ual_defs.h"
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

enum class CacheMode
{
    None,
    IDS,
    Struct,
};

}
}

class LIBRARY_API UDABackend : public Backend
{
private:
    bool verbose_ = false;
    std::string plugin_ = "IMAS";
    uda::Client uda_client_;
    std::shared_ptr<pugi::xml_document> doc_ = {};
    imas::uda::CacheType cache_ = {};
    imas::uda::CacheMode cache_mode_ = imas::uda::CacheMode::IDS;
    std::map<std::string, std::string> env_options_ = {};
    int open_mode_ = 0;

    void process_option(const std::string& option);
    void process_options(const std::string& options);
    void load_env_options();
    void populate_cache(const std::string& ids, const std::string& path, DataEntryContext* pulse_ctx, OperationContext* op_ctx);
    bool get_homogeneous_flag(const std::string& ids, DataEntryContext* pulse_ctx, OperationContext* op_ctx);

public:

    explicit UDABackend(bool verb=false) : verbose_(verb)
    {
        const char* env = getenv("IMAS_UDA_PLUGIN");
        if (env != nullptr) {
            plugin_ = env;
        }

        doc_ = imas::uda::load_xml();

        if (verbose_) {
            std::cout << "UDABackend constructor\n";
            std::cout << "UDA Server: " << uda_client_.serverHostName() << "\n";
            std::cout << "UDA Port: " << uda_client_.serverPort() << "\n";
            std::cout << "UDA Plugin: " << plugin_ << "\n";
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

    
};

#endif

#endif // UDA_BACKEND_H

