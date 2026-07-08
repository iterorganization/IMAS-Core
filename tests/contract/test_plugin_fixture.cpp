// Minimal in-repo plugin, built as a loadable `<name>_plugin.so` for the
// plugin-lifecycle contract tests (issue #7, TEST_STRATEGY.md step 5).
//
// This is the *fixture/harness* half of the suite (the only place classic
// red-green applies): a real, deterministic plugin the C ABI can actually
// `dlopen`, so the register / bind / unbind / unregister state machine and the
// `al_setvalue_*_parameter_plugin` happy paths can be exercised end to end
// rather than only through their failure modes.
//
// It implements the whole `access_layer_plugin` interface (all pure virtuals of
// access_layer_base_plugin + provenance_plugin_feature + readback_plugin_feature)
// so `create()` returns a fully-concrete object, and exports the C
// `create()`/`destroy()` factory symbols the framework resolves via `dlsym`
// (src/al_lowlevel.cpp:360-377). It links no libal symbols — every plugin base
// class is header-only — so the module is self-contained and needs no rpath.
//
// setParameter appends a machine-readable line to the file named by the
// AL_CONTRACT_PLUGIN_LOG env var (when set), so a test can prove a value passed
// through the C ABI actually reached the plugin, not merely that the call
// returned success.

#include "access_layer_plugin.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

class ContractTestPlugin : public access_layer_plugin {
public:
    // --- the one method the setvalue_* ABI calls reach ---------------------
    void setParameter(const char* parameter_name, int datatype, int dim,
                      int* /*size*/, void* data) override {
        const char* log_path = std::getenv("AL_CONTRACT_PLUGIN_LOG");
        if (log_path == nullptr) {
            return;
        }
        std::FILE* f = std::fopen(log_path, "a");
        if (f == nullptr) {
            return;
        }
        // Format: PARAM|<name>|<datatype>|<dim>|<scalar value or ->
        // The scalar value is decoded for the two convenience variants so the
        // test can assert the exact value that crossed the ABI boundary.
        if (dim == 0 && data != nullptr && datatype == INTEGER_DATA) {
            std::fprintf(f, "PARAM|%s|%d|%d|%d\n", parameter_name, datatype, dim,
                         *static_cast<int*>(data));
        } else if (dim == 0 && data != nullptr && datatype == DOUBLE_DATA) {
            std::fprintf(f, "PARAM|%s|%d|%d|%g\n", parameter_name, datatype, dim,
                         *static_cast<double*>(data));
        } else {
            std::fprintf(f, "PARAM|%s|%d|%d|-\n", parameter_name, datatype, dim);
        }
        std::fclose(f);
    }

    // --- remaining access_layer_plugin surface: inert, never invoked by the
    //     lifecycle tests (they never drive a get/put through the plugin) -----
    void begin_global_action(int, const char*, const char*, int, int) override {}
    void begin_slice_action(int, const char*, int, double, int, int) override {}
    void begin_arraystruct_action(int, int*, const char*, const char*,
                                  int*) override {}
    void end_action(int) override {}
    int read_data(int, const char*, const char*, void**, int, int,
                  int*) override {
        return 0;  // "no data" per the readData 0/1 convention
    }
    void write_data(int, const char*, const char*, void*, int, int,
                    int*) override {}
    plugin::OPERATION node_operation(const std::string&) override {
        return plugin::PUT_AND_GET;
    }

    // --- provenance_plugin_feature -----------------------------------------
    std::string getName() override { return "alcontract"; }
    std::string getDescription() override { return "contract-test plugin"; }
    std::string getCommit() override { return "0000000"; }
    std::string getVersion() override { return "0.0.0"; }
    std::string getRepository() override { return "in-repo"; }
    std::string getParameters() override { return ""; }

    // --- readback_plugin_feature -------------------------------------------
    std::string getReadbackName(const std::string&, int*) override {
        return "";
    }
    std::string getReadbackDescription(const std::string&) override {
        return "";
    }
    std::string getReadbackCommit(const std::string&) override { return ""; }
    std::string getReadbackVersion(const std::string&) override { return ""; }
    std::string getReadbackRepository(const std::string&) override {
        return "";
    }
    std::string getReadbackParameters(const std::string&) override {
        return "";
    }
};

}  // namespace

// The C factory symbols the framework resolves after dlopen (create_t/destroy_t
// in access_layer_base_plugin.h).
extern "C" access_layer_base_plugin* create() { return new ContractTestPlugin(); }
extern "C" void destroy(access_layer_base_plugin* p) { delete p; }
