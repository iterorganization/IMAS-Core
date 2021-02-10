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

#include "dummy_backend.h"
#include "ual_backend.h"
#include "ual_const.h"
#include "ual_context.h"
#include "ual_defs.h"

#define NODENAME_MANGLING  //Use IMAS mangling

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus


class LIBRARY_API MachineMapping
{
public:
    MachineMapping() : mappings_{}
    {
        auto file_name = getenv("UDA_IMAS_PLUGIN_MAP");
        if (file_name == nullptr) {
            std::cerr << "environmental variable UDA_IMAS_PLUGIN_MAP not set\n";
            return;
        }

        std::ifstream in_file(file_name);
        if (!in_file.good()) {
            std::cerr << "cannot read mapping file " << file_name << "\n";
            return;
        }

        std::string line;
        while (std::getline(in_file, line)) {
            if (line[0] == '#') {
                continue;
            }

            std::vector<std::string> words;
            boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);
            if (words.size() != 4) {
                throw std::runtime_error(std::string("bad line in ") + file_name + ": " + line);
            }

            auto machine = words[0];
            auto plugin = words[1];

            std::vector<std::string> args;
            if (plugin.find('(') != std::string::npos) {
                std::string arg_string = plugin.substr(0);
                boost::split(args, arg_string, boost::is_any_of(","), boost::token_compress_on);
            }

            mappings_[machine] = MappingValue{ plugin, args };
        }
    }

    std::vector<std::string> args(const std::string& machine)
    {
        if (mappings_.count(machine) == 0) {
            return {};
        }
        auto& plugin_map = mappings_[machine];
        return plugin_map.args;
    }

    std::string plugin(const std::string& machine)
    {
        if (mappings_.count(machine) == 0) {
            return {};
        }
        auto& plugin_map = mappings_[machine];
        return plugin_map.plugin;
    }

private:
    struct MappingValue {
        std::string plugin;
        std::vector<std::string> args;
    };

    std::unordered_map<std::string, MappingValue> mappings_;
};

class LIBRARY_API UDABackend : public Backend
{
private:
    bool verbose = false;
    std::string plugin = "IMAS_MAPPING";
    uda::Client uda_client;
    int ctx_id = -1;
    MachineMapping machine_mapping;

public:

    explicit UDABackend(bool verb=false) : verbose(verb)
    {
        const char* env = getenv("UDA_PLUGIN");
        if (env != nullptr) {
            plugin = env;
        }

        if (verbose) {
            std::cout << "UDABackend constructor\n";
            std::cout << "UDA Server: " << uda_client.serverHostName() << "\n";
            std::cout << "UDA Port: " << uda_client.serverPort() << "\n";
            std::cout << "UDA Plugin: " << plugin << "\n";
        }
    }

    ~UDABackend() override
    {
        if (verbose) {
            std::cout << "UDABackend destructor\n";
        }
    }

    void openPulse(PulseContext *ctx, int mode, std::string options) override;

    void closePulse(PulseContext *ctx, int mode, std::string options) override;

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

};

#endif

#endif // UDA_BACKEND_H

