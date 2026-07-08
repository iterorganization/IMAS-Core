// Plugin management contract tests — User Cluster 3
// (FUNCTIONALITY_INVENTORY.md:318-430, TEST_STRATEGY.md step 5).
//
// The register / bind / unbind / unregister state machine and the
// al_setvalue_*_parameter_plugin calls, driven through the public C ABI
// (decision D1). Every behavior asserted here was first characterized against
// the built library, then classified once per decision D2:
//   * intended contract      -> asserted directly (green)
//   * genuine defect         -> correct behavior asserted, DISABLED_ (xfail),
//                               paired with a current-behavior tripwire
//
// The crash-class null-deref defects (setvalue on an unregistered name) are
// death tests and live with the other death tests in test_known_defects.cpp.
// Two defects that need a real, registerable plugin live here: the
// registered-but-never-bound plugin left un-destroyed on unregister, and the
// dlopen-failure swallowed by the NDEBUG-stripped assert.
//
// Fixture: a real plugin is dlopen-ed from AL_CONTRACT_PLUGIN_DIR (the
// build-time location of alcontract_plugin.so, from test_plugin_fixture.cpp),
// with IMAS_AL_ENABLE_PLUGINS=TRUE. The plugin registry is process-global and
// static; SetUp/TearDown force a clean slate so the suite is order-independent
// whether run per-case under ctest or as one direct ./contract_tests process.

#ifndef _WIN32  // plugins framework is dlopen/dlfcn-based (POSIX)

#include "al_contract.h"

#include <al_const.h>
#include <al_lowlevel.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// Both are always supplied by CMake; a silent fallback could drift from the
// build's real values, so require both explicitly.
#ifndef AL_CONTRACT_PLUGIN_DIR
#error "AL_CONTRACT_PLUGIN_DIR must be defined by CMake (dir holding alcontract_plugin.so)"
#endif
#ifndef AL_CONTRACT_PLUGIN_NAME
#error "AL_CONTRACT_PLUGIN_NAME must be defined by CMake (the registerable plugin name)"
#endif

namespace {

using ::al_contract::IsOk;

constexpr const char* kPlugin = AL_CONTRACT_PLUGIN_NAME;

// A plugin name that is never backed by a .so and never registered.
constexpr const char* kNeverRegistered = al_contract::kUnregisteredPluginName;

bool is_registered(const char* name) {
    bool r = false;
    al_status_t s = al_is_plugin_registered(name, &r);
    return s.code == 0 && r;
}

// Force `name` out of the process-global registry so each test starts clean.
//
// A plain al_unregister_plugin is not enough: the very defect this suite pins is
// that unregister only destroys a plugin that is *bound* to a path. So we bind
// it to a throwaway path first (which makes unregister take its destroy+erase
// path), then unregister. Only touches plugins whose entry has a live instance
// (the contract-test plugin always does); a half-registered entry from a failed
// dlopen is left alone, which is fine — no test reuses such a name.
void force_unregister(const char* name) {
    bool r = false;
    if (al_is_plugin_registered(name, &r).code != 0 || !r) {
        return;
    }
    al_bind_plugin("alcontract_cleanup/node", name);
    al_unregister_plugin(name);
}

class PluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        setenv("IMAS_AL_ENABLE_PLUGINS", "TRUE", 1);
        setenv("IMAS_AL_PLUGINS", AL_CONTRACT_PLUGIN_DIR, 1);
        force_unregister(kPlugin);
    }
    void TearDown() override {
        // Runs while the framework is still enabled so force_unregister works,
        // then clears the env so non-plugin cases in the same process are
        // unaffected (the read/write hot path checks IMAS_AL_ENABLE_PLUGINS).
        setenv("IMAS_AL_ENABLE_PLUGINS", "TRUE", 1);
        setenv("IMAS_AL_PLUGINS", AL_CONTRACT_PLUGIN_DIR, 1);
        force_unregister(kPlugin);
        unsetenv("AL_CONTRACT_PLUGIN_LOG");
        unsetenv("IMAS_AL_ENABLE_PLUGINS");
        unsetenv("IMAS_AL_PLUGINS");
    }
};

// ===========================================================================
// register / unregister lifecycle + is_plugin_registered
// ===========================================================================

TEST_F(PluginTest, RegisterMakesPluginRegistered) {
    ASSERT_FALSE(is_registered(kPlugin)) << "SetUp should leave a clean slate";
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    EXPECT_TRUE(is_registered(kPlugin));
}

TEST_F(PluginTest, RegisterTwiceReturnsError) {
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    al_status_t s = al_register_plugin(kPlugin);
    EXPECT_NE(s.code, 0)
        << "registering an already-registered name must error, not silently "
           "re-register (FUNCTIONALITY_INVENTORY.md:345-346)";
}

TEST_F(PluginTest, IsPluginRegisteredIsFalseForNeverRegisteredName) {
    bool r = true;
    al_status_t s = al_is_plugin_registered(kNeverRegistered, &r);
    AL_EXPECT_OK(s);
    EXPECT_FALSE(r);
}

TEST_F(PluginTest, UnregisterNeverRegisteredNameReturnsError) {
    al_status_t s = al_unregister_plugin(kNeverRegistered);
    EXPECT_NE(s.code, 0)
        << "unregistering a name that was never registered must error "
           "(FUNCTIONALITY_INVENTORY.md:345-346)";
}

// A *bound* plugin IS properly destroyed on unregister — the contrast that
// makes the never-bound quirk (below) a genuine defect rather than the spec.
TEST_F(PluginTest, UnregisterBoundPluginDestroysIt) {
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    AL_ASSERT_OK(al_bind_plugin("equilibrium/time", kPlugin));
    AL_ASSERT_OK(al_unregister_plugin(kPlugin));
    EXPECT_FALSE(is_registered(kPlugin))
        << "a plugin bound to a path must be destroyed and removed on "
           "unregister";
}

// ===========================================================================
// bind / unbind
// ===========================================================================

TEST_F(PluginTest, BindRegisteredPluginSucceeds) {
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    AL_EXPECT_OK(al_bind_plugin("equilibrium/time", kPlugin));
}

TEST_F(PluginTest, BindUnregisteredPluginReturnsError) {
    // Deliberately not registered.
    al_status_t s = al_bind_plugin("equilibrium/time", kNeverRegistered);
    EXPECT_NE(s.code, 0)
        << "binding a plugin that was never registered must error "
           "(src/al_lowlevel.cpp:193-197)";
}

TEST_F(PluginTest, DoubleBindSamePathReturnsError) {
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    AL_ASSERT_OK(al_bind_plugin("equilibrium/time", kPlugin));
    al_status_t s = al_bind_plugin("equilibrium/time", kPlugin);
    EXPECT_NE(s.code, 0)
        << "binding the same plugin to the same path twice must throw "
           "(FUNCTIONALITY_INVENTORY.md:373-374, src/al_lowlevel.cpp:269-273)";
}

// Unbinding a path nothing is bound to is a documented silent no-op (returns
// success), not an error (FUNCTIONALITY_INVENTORY.md:374-376,
// src/al_lowlevel.cpp:302-305). Note: al_unbind_plugin does not even require
// the plugin to be registered — it operates on the bound-path map directly.
TEST_F(PluginTest, UnbindNeverBoundPathIsSilentNoOp) {
    AL_EXPECT_OK(al_unbind_plugin("equilibrium/time", kPlugin));
}

// ===========================================================================
// al_setvalue_*_parameter_plugin — happy paths on a REGISTERED plugin
// (the unregistered-name crashes are death tests in test_known_defects.cpp)
// ===========================================================================

// Reads the fixture plugin's setParameter log (see test_plugin_fixture.cpp).
std::string read_plugin_log(const std::filesystem::path& p) {
    std::ifstream in(p);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// A per-test path for the fixture plugin's setParameter log, wired via the
// AL_CONTRACT_PLUGIN_LOG env var and RAII-cleaned. `tag` keeps concurrent cases
// from colliding on the same file.
class PluginLog {
public:
    explicit PluginLog(const char* tag)
        : path_(std::filesystem::temp_directory_path() /
                ("al_contract_pluginlog_" + std::to_string(::getpid()) + "_" +
                 tag + ".txt")) {
        std::filesystem::remove(path_);
        setenv("AL_CONTRACT_PLUGIN_LOG", path_.string().c_str(), 1);
    }
    ~PluginLog() { std::filesystem::remove(path_); }
    std::string contents() const { return read_plugin_log(path_); }

private:
    std::filesystem::path path_;
};

// Proves a parameter value crossed the C ABI and reached the plugin's
// setParameter, by matching the log line the fixture emits:
//   PARAM|<name>|<datatype>|<dim>|<value>
void expect_param_logged(const PluginLog& log, const std::string& name,
                         int datatype, const std::string& value) {
    const std::string logged = log.contents();
    EXPECT_NE(logged.find("PARAM|" + name + "|" + std::to_string(datatype) +
                          "|0|" + value),
              std::string::npos)
        << "plugin log did not record parameter '" << name << "'; got: ["
        << logged << "]";
}

TEST_F(PluginTest, SetValueIntScalarOnRegisteredPluginReachesPlugin) {
    PluginLog log("int");
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    AL_ASSERT_OK(al_setvalue_int_scalar_parameter_plugin("gain", 42, kPlugin));
    expect_param_logged(log, "gain", INTEGER_DATA, "42");
}

TEST_F(PluginTest, SetValueDoubleScalarOnRegisteredPluginReachesPlugin) {
    PluginLog log("dbl");
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    AL_ASSERT_OK(
        al_setvalue_double_scalar_parameter_plugin("scale", 1.5, kPlugin));
    expect_param_logged(log, "scale", DOUBLE_DATA, "1.5");
}

// The generic variant carries datatype/dim/size/data explicitly; assert the
// value reaches the plugin (not merely that the call returns OK), matching the
// strength of the int/double variants above.
TEST_F(PluginTest, SetValueGenericOnRegisteredPluginReachesPlugin) {
    PluginLog log("generic");
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    int value = 7;
    AL_ASSERT_OK(al_setvalue_parameter_plugin("mode", INTEGER_DATA, 0, nullptr,
                                              &value, kPlugin));
    expect_param_logged(log, "mode", INTEGER_DATA, "7");
}

// ===========================================================================
// framework-gating: with IMAS_AL_ENABLE_PLUGINS unset the calls must error
// (src/al_lowlevel.cpp:116-119), not proceed.
// ===========================================================================

TEST_F(PluginTest, RegisterWithFrameworkDisabledReturnsError) {
    unsetenv("IMAS_AL_ENABLE_PLUGINS");
    al_status_t s = al_register_plugin(kPlugin);
    EXPECT_NE(s.code, 0);
}

TEST_F(PluginTest, IsPluginRegisteredWithFrameworkDisabledReturnsError) {
    unsetenv("IMAS_AL_ENABLE_PLUGINS");
    bool r = false;
    al_status_t s = al_is_plugin_registered(kPlugin, &r);
    EXPECT_NE(s.code, 0);
}

// ===========================================================================
// DEFECT (D2): registered-but-never-bound plugin left un-destroyed on
// unregister. FUNCTIONALITY_INVENTORY.md:347-354, src/al_lowlevel.cpp:382-431:
// unregisterPlugin's destroy()+erase only run inside the boundPlugins loop, so
// a plugin that was never bound survives unregister (and re-registering it then
// throws "already registered").
// ===========================================================================

// CORRECT-CONTRACT test, expected-fail (DISABLED_). The day unregister is fixed
// to destroy unconditionally, drop the DISABLED_ prefix.
TEST_F(PluginTest, DISABLED_UnregisterNeverBoundPluginDestroysIt) {
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    // Intentionally never bound.
    AL_ASSERT_OK(al_unregister_plugin(kPlugin));
    EXPECT_FALSE(is_registered(kPlugin))
        << "correct contract: unregister must destroy and remove the plugin "
           "even when it was never bound";
}

// CURRENT-BEHAVIOR tripwire: pins that the never-bound plugin stays registered
// after unregister. If this flips (is_registered becomes false), the defect was
// fixed — enable DISABLED_UnregisterNeverBoundPluginDestroysIt above.
TEST_F(PluginTest, UnregisterNeverBoundPluginLeavesItRegistered_CurrentBehavior) {
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    // Intentionally never bound.
    AL_ASSERT_OK(al_unregister_plugin(kPlugin))
        << "unregister currently reports success even though it destroys "
           "nothing";
    EXPECT_TRUE(is_registered(kPlugin))
        << "tripwire: a never-bound plugin is currently left in the registry "
           "on unregister (FUNCTIONALITY_INVENTORY.md:347-354). If this fails, "
           "the defect was fixed — enable "
           "PluginTest.UnregisterNeverBoundPluginDestroysIt.";
}

// ===========================================================================
// DEFECT (D2): dlopen failure in registerPlugin is guarded only by an assert.
// FUNCTIONALITY_INVENTORY.md:355-360, src/al_lowlevel.cpp:350-357: a failed
// dlopen() is printf'd and asserted; under -DNDEBUG (this build:
// RelWithDebInfo/Release) the assert is stripped, so the null handle flows on.
//
// Empirically characterized on this platform: the null handle reaches dlsym,
// whose own null-check throws — so registerPlugin returns LOWLEVEL_ERR with the
// *misleading* "Cannot load symbol create" message rather than reporting the
// dlopen failure, and does NOT crash. (In an assert-enabled build the stripped
// assert would instead fire -> SIGABRT; that death only exists there, which is
// why this pair is a plain xfail, not a death test.)
// ===========================================================================

// Points IMAS_AL_PLUGINS at a temp dir holding a garbage <name>_plugin.so that
// exists (so registerPlugin gets past its boost::filesystem::exists check) but
// is not a loadable shared library, then registers `name`. Returns the status.
al_status_t register_unloadable(const std::filesystem::path& dir,
                                const char* name) {
    namespace fs = std::filesystem;
    fs::create_directories(dir);
    const fs::path so = dir / (std::string(name) + "_plugin.so");
    {
        std::ofstream out(so, std::ios::binary | std::ios::trunc);
        out << "this is deliberately not a valid shared library\n";
    }
    setenv("IMAS_AL_PLUGINS", dir.string().c_str(), 1);
    return al_register_plugin(name);
}

// Both tests key on whether the returned status *attributes the dlopen
// failure*. Correct handling would surface the dlerror (message mentions
// "dlopen"); the current swallowed-assert path does not — it reports the
// unrelated downstream symbol miss instead. Keying on "dlopen" presence is
// robust to the exact downstream text (which depends on dlsym's null-handle
// diagnostic, and on no global "create" symbol existing).
bool status_reports_dlopen(const al_status_t& s) {
    return s.message != nullptr &&
           std::string(s.message).find("dlopen") != std::string::npos;
}

// CORRECT-CONTRACT test, expected-fail (DISABLED_): the error must attribute the
// dlopen failure (surface the dlerror), not a downstream symptom.
TEST_F(PluginTest, DISABLED_RegisterUnloadableSharedLibReportsDlopenFailure) {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() /
                         ("al_contract_badplugin_" +
                          std::to_string(::getpid()) + "_a");
    al_status_t s = register_unloadable(dir, "unloadable_a");
    EXPECT_NE(s.code, 0);
    EXPECT_TRUE(status_reports_dlopen(s))
        << "correct contract: report the dlopen failure (dlerror). Got: "
        << (s.message ? s.message : "(null)");
    fs::remove_all(dir);
}

// CURRENT-BEHAVIOR tripwire: pins that the returned status does NOT mention the
// dlopen failure — the NDEBUG-stripped assert lets the null handle flow past
// dlopen, so the failure is swallowed and only a downstream symptom is reported.
// If the status starts naming dlopen, the handling was fixed — enable the
// DISABLED_ test above.
TEST_F(PluginTest, RegisterUnloadableSharedLibSwallowsAssert_CurrentBehavior) {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() /
                         ("al_contract_badplugin_" +
                          std::to_string(::getpid()) + "_b");
    al_status_t s = register_unloadable(dir, "unloadable_b");
    EXPECT_NE(s.code, 0) << "on this NDEBUG build it returns an error, not a crash";
    EXPECT_FALSE(status_reports_dlopen(s))
        << "tripwire: the dlopen failure is currently swallowed by the "
           "NDEBUG-stripped assert, so the status does not attribute it "
           "(FUNCTIONALITY_INVENTORY.md:355-360). If this fails, the dlopen "
           "handling was fixed — enable "
           "PluginTest.RegisterUnloadableSharedLibReportsDlopenFailure.";
    fs::remove_all(dir);
}

}  // namespace

#endif  // !_WIN32
