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

#include <csignal>
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

// Shared by every test below that needs a real (Memory-backend, unit-tier)
// pulse to write/read through — issue #8's ownership-sweep tests are the
// first in this file to need one, since the earlier plugin-registry tests
// above operate on bare path strings with no backend at all.
int open_memory_pulse(al_contract::TempBase& base,
                      const al_contract::PulseId& pulse) {
    const std::string u = al_contract::build_uri(MEMORY_BACKEND, base.str(), pulse);
    int pctx = -1;
    EXPECT_EQ(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx).code,
              0);
    return pctx;
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
// al_setvalue_parameter_plugin's dim/size (FUNCTIONALITY_INVENTORY.md:399-420):
// is `dim` bound by MAXDIM? (issue #8 ownership sweep — resolved empirically,
// not assumed.)
//
// First hypothesis tried here was that dim > MAXDIM would be rejected; it
// isn't (confirmed by running this test against the built library before
// settling on the assertion below). LLplugin::setvalueParameterPlugin
// (al_lowlevel.cpp:453-457) forwards dim/size straight through to the
// plugin's setParameter with no comparison against MAXDIM and no copy into
// any core-side fixed-size buffer. MAXDIM bounds only al_read_data's/
// al_write_data's own shape arrays, not this call chain — a plugin author
// choosing to size an internal buffer at MAXDIM would be relying on a
// convention the core does not enforce for this ABI.
// ===========================================================================
TEST_F(PluginTest, SetValueGenericAcceptsDimAboveMaxdim) {
    PluginLog log("maxdim");
    AL_ASSERT_OK(al_register_plugin(kPlugin));

    constexpr int kDim = MAXDIM + 1;  // one past the documented bound
    int size[kDim];
    for (int i = 0; i < kDim; ++i) size[i] = i + 1;
    int value = 99;
    al_status_t s = al_setvalue_parameter_plugin("oversize", INTEGER_DATA, kDim,
                                                  size, &value, kPlugin);
    EXPECT_EQ(s.code, 0)
        << "dim beyond MAXDIM is not bounds-checked by the core, so the call "
           "must still succeed";
    const std::string logged = log.contents();
    EXPECT_NE(logged.find("PARAM|oversize|" + std::to_string(INTEGER_DATA) +
                          "|" + std::to_string(kDim) + "|-"),
              std::string::npos)
        << "dim must reach the plugin unmodified (not clamped to MAXDIM); "
           "got: [" << logged << "]";
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
// dlopen() is printf'd and asserted, so current behavior is BUILD-MODE
// DEPENDENT and the pins below are too (issue #32):
//
//   - assert-enabled builds (Debug, no NDEBUG): the assert fires -> SIGABRT.
//     Pinned with a death test so the abort happens in a child process and
//     the suite itself survives.
//   - NDEBUG builds (Release/RelWithDebInfo): the assert is stripped and the
//     null handle flows on to dlsym, whose own null-check throws — so
//     registerPlugin returns LOWLEVEL_ERR with the *misleading* "Cannot load
//     symbol create" message rather than reporting the dlopen failure, and
//     does NOT crash.
//
// Either way the correct contract is the same (a nonzero status attributing
// the actual dlopen error) — the DISABLED_ test below stays build-mode
// independent and becomes enableable once registerPlugin handles the failed
// dlopen instead of asserting on it.
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
    return std::string(s.message).find("dlopen") != std::string::npos;
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
        << s.message;
    fs::remove_all(dir);
}

#ifdef NDEBUG
// CURRENT-BEHAVIOR tripwire (NDEBUG builds): pins that the returned status does
// NOT mention the dlopen failure — the NDEBUG-stripped assert lets the null
// handle flow past dlopen, so the failure is swallowed and only a downstream
// symptom is reported. If the status starts naming dlopen, the handling was
// fixed — enable the DISABLED_ test above.
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
#else
// CURRENT-BEHAVIOR tripwire (assert-enabled builds): the product assertion at
// src/al_lowlevel.cpp fires on the failed dlopen and aborts. Run the call in a
// death-test child so the runner survives; pin SIGABRT + the assertion text
// specifically, so an unrelated crash is not mistaken for this defect. The
// fixture artifacts are created and removed by the PARENT — the child dying
// must not leak the temp dir (issue #32).
TEST_F(PluginTest, RegisterUnloadableSharedLibAbortsOnAssert_CurrentBehavior) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() /
                         ("al_contract_badplugin_" +
                          std::to_string(::getpid()) + "_b");
    // threadsafe-style death tests RE-EXECUTE the whole test in the child,
    // which would recompute `dir` with the child's own pid and leave that
    // directory behind; hand the parent's path down via the environment so
    // parent and child agree on the one directory the parent removes below.
    // overwrite=0: the re-executed child runs this line too and must NOT
    // replace the inherited parent value with its own pid-derived path.
    setenv("AL_CONTRACT_BADPLUGIN_DIR", dir.string().c_str(), /*overwrite=*/0);
    const std::filesystem::path used_dir =
        std::getenv("AL_CONTRACT_BADPLUGIN_DIR");
    ASSERT_EXIT(
        {
            const char* d = std::getenv("AL_CONTRACT_BADPLUGIN_DIR");
            register_unloadable(d != nullptr ? fs::path(d) : dir,
                                "unloadable_b");
        },
        ::testing::KilledBySignal(SIGABRT), "[Aa]ssert")
        << "expected the registerPlugin dlopen assert to abort in an "
           "assert-enabled build (src/al_lowlevel.cpp, "
           "FUNCTIONALITY_INVENTORY.md:355-360). If this no longer dies, the "
           "handling was fixed — enable "
           "PluginTest.RegisterUnloadableSharedLibReportsDlopenFailure.";
    unsetenv("AL_CONTRACT_BADPLUGIN_DIR");
    fs::remove_all(used_dir);
}
#endif

// ===========================================================================
// al_plugin_* low-level reentry (FUNCTIONALITY_INVENTORY.md:854-895, Part 3
// cluster 5) — issue #8 ownership sweep.
//
// These are plain C-ABI functions that go straight to
// `lle.backend->{beginAction,writeData,readData,endAction}`, bypassing plugin
// dispatch entirely — they are exactly what al_write_data/al_read_data fall
// back to when no regular plugin is bound to the path
// (al_lowlevel.cpp:1692,1728). No plugin registry object or dlopen'd plugin
// is needed to call them directly: same preconditions as the ordinary
// al_begin_global_action/al_write_data/al_read_data/al_end_action quartet.
// (al_plugin_begin_timerange_action is excluded — a known declaration/
// definition mismatch, tracked separately per TRACEABILITY.md.)
// ===========================================================================
class PluginReentry : public ::testing::Test {
protected:
    al_contract::TempBase base_;
    al_contract::PulseId pulse_{/*database=*/"test", /*version=*/"3",
                                /*pulse=*/12, /*run=*/0};
};

TEST_F(PluginReentry, WriteDataReentersTheBackendDirectly) {
    const int pctx = open_memory_pulse(base_, pulse_);

    int opctx = -1;
    AL_ASSERT_OK(al_plugin_begin_global_action(pctx, "magnetics", "",
                                               WRITE_OP, &opctx));
    int value = 55;
    AL_ASSERT_OK(al_plugin_write_data(opctx, "comment", "", &value,
                                      INTEGER_DATA, 0, nullptr));
    AL_ASSERT_OK(al_plugin_end_action(opctx));

    // Read back through the *ordinary* ABI to prove al_plugin_write_data
    // landed in the real backend storage, not some plugin-side stash.
    int op2 = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, "magnetics", "", READ_OP, &op2));
    int got = -1;
    AL_ASSERT_OK(al_contract::read_int_scalar(op2, "comment", &got));
    EXPECT_EQ(got, 55);
    al_end_action(op2);

    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST_F(PluginReentry, ReadDataAlsoReentersTheBackendDirectly) {
    const int pctx = open_memory_pulse(base_, pulse_);

    // Write through the ordinary ABI...
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, "magnetics", "", WRITE_OP, &op));
    AL_ASSERT_OK(al_contract::write_int_scalar(op, "comment", 77));
    al_end_action(op);

    // ...and read it back through the plugin reentry path.
    int opctx = -1;
    AL_ASSERT_OK(al_plugin_begin_global_action(pctx, "magnetics", "", READ_OP,
                                               &opctx));
    int got = -1;
    int size[MAXDIM] = {0};
    void* buf = &got;
    AL_ASSERT_OK(al_plugin_read_data(opctx, "comment", "", &buf, INTEGER_DATA,
                                     0, size));
    EXPECT_EQ(got, 77);
    AL_ASSERT_OK(al_plugin_end_action(opctx));

    al_close_pulse(pctx, CLOSE_PULSE);
}

// ===========================================================================
// Action lifecycle & data interception (FUNCTIONALITY_INVENTORY.md:784-824,
// Part 3 cluster 3) — issue #8 ownership sweep.
//
// al_write_data/al_read_data check LLplugin::getBoundPlugins for the target
// field path *before* touching the backend (al_lowlevel.cpp:1685-1693,
// 1721-1731): if a plugin is bound there, the call dispatches to the
// plugin's write_data/read_data instead of the backend. ContractTestPlugin's
// write_data is an inert no-op and its read_data always returns 0 ("no
// data"), so binding it to a path makes writes there vanish (never reach the
// backend) and reads come back as the "no data" default — proof the plugin,
// not the backend, handled the call.
// ===========================================================================
TEST_F(PluginTest, BoundPluginInterceptsWriteInsteadOfBackend) {
    al_contract::TempBase base;
    al_contract::PulseId pulse{"test", "3", 12, 0};
    const int pctx = open_memory_pulse(base, pulse);

    AL_ASSERT_OK(al_register_plugin(kPlugin));
    AL_ASSERT_OK(al_bind_plugin("magnetics/probe", kPlugin));

    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, "magnetics", "", WRITE_OP, &op));
    AL_ASSERT_OK(al_contract::write_int_scalar(op, "probe", 42));
    // A sibling field, never bound to any plugin: the normal path, as a
    // control proving the value 42 above wasn't lost for some unrelated
    // reason.
    AL_ASSERT_OK(al_contract::write_int_scalar(op, "control", 42));
    al_end_action(op);

    // Unregister *before* reading back, so the read genuinely reaches the
    // backend — otherwise the still-bound plugin's read_data() would also
    // intercept the read, and EXPECT_NE(probe, 42) below would pass even if
    // the write had actually landed (conflating write-interception with
    // read-interception, which BoundPluginInterceptsReadInsteadOfBackend
    // covers separately). al_unregister_plugin, not al_unbind_plugin: bind
    // normalizes "magnetics/probe" to "magnetics:0/probe" internally
    // (al_lowlevel.cpp LLplugin::bindPlugin's occurrence-suffix regex), but
    // unbind does no such normalization and looks up the raw string
    // unchanged — so al_unbind_plugin("magnetics/probe", kPlugin) here would
    // silently miss (confirmed: it prints "No plugin bound..." and leaves
    // the binding intact). al_unregister_plugin instead scans boundPlugins
    // by plugin name, not by path string, so it isn't sensitive to this
    // normalization mismatch.
    AL_ASSERT_OK(al_unregister_plugin(kPlugin));

    int op2 = -1;
    AL_ASSERT_OK(
        al_begin_global_action(pctx, "magnetics", "", READ_OP, &op2));
    int control = -1;
    AL_ASSERT_OK(al_contract::read_int_scalar(op2, "control", &control));
    EXPECT_EQ(control, 42) << "the unbound sibling field must round-trip "
                              "normally (control for the assertion below)";

    int probe = -1;
    AL_ASSERT_OK(al_contract::read_int_scalar(op2, "probe", &probe));
    EXPECT_NE(probe, 42)
        << "the bound plugin's inert write_data() must have absorbed the "
           "write — 42 must never have reached the Memory backend, even "
           "read back directly (unbound) afterward";
    al_end_action(op2);
    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST_F(PluginTest, BoundPluginInterceptsReadInsteadOfBackend) {
    al_contract::TempBase base;
    al_contract::PulseId pulse{"test", "3", 12, 0};
    const int pctx = open_memory_pulse(base, pulse);

    // Write for real, with no plugin bound.
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, "magnetics", "", WRITE_OP, &op));
    AL_ASSERT_OK(al_contract::write_int_scalar(op, "probe", 42));
    al_end_action(op);

    // Bind only for the read.
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    AL_ASSERT_OK(al_bind_plugin("magnetics/probe", kPlugin));

    int op2 = -1;
    AL_ASSERT_OK(
        al_begin_global_action(pctx, "magnetics", "", READ_OP, &op2));
    int got = -1;
    AL_ASSERT_OK(al_contract::read_int_scalar(op2, "probe", &got));
    EXPECT_NE(got, 42)
        << "the bound plugin's read_data() (returns 0, \"no data\") must "
           "answer instead of the backend, even though 42 really is stored";
    al_end_action(op2);
    al_close_pulse(pctx, CLOSE_PULSE);
}

// ===========================================================================
// al_write_plugins_metadata (FUNCTIONALITY_INVENTORY.md:422-430, User Cluster
// 3 + Part 3 cluster 1 provenance) — issue #8 ownership sweep.
//
// Writes, under the just-written IDS's "ids_properties/plugins/node" AOS, one
// entry per distinct bound-and-put-capable path: the path itself, a nested
// "put_operation" AOS with the bound plugin's provenance
// (getName/getDescription/getCommit/getVersion/getRepository/getParameters),
// and a nested "readback" AOS built from getReadbackName/* for any plugin
// that opts in — empty here, since ContractTestPlugin declares no readback
// capability for any path (access_layer_plugin_manager.cpp:359-489).
// ===========================================================================
TEST_F(PluginTest, WritePluginsMetadataStoresBoundPluginProvenance) {
    al_contract::TempBase base;
    al_contract::PulseId pulse{"test", "3", 12, 0};
    const int pctx = open_memory_pulse(base, pulse);

    AL_ASSERT_OK(al_register_plugin(kPlugin));
    AL_ASSERT_OK(al_bind_plugin("magnetics/comment", kPlugin));

    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, "magnetics", "", WRITE_OP, &op));
    AL_ASSERT_OK(al_write_plugins_metadata(op));
    al_end_action(op);

    int op2 = -1;
    AL_ASSERT_OK(
        al_begin_global_action(pctx, "magnetics", "", READ_OP, &op2));
    int nsize = 0;
    int naos = -1;
    AL_ASSERT_OK(al_begin_arraystruct_action(
        op2, "ids_properties/plugins/node", "", &nsize, &naos));
    ASSERT_EQ(nsize, 1) << "one bound put-capable path -> one node entry";

    std::string path;
    AL_ASSERT_OK(al_contract::read_char_array(naos, "path", &path));
    EXPECT_EQ(path, "comment")
        << "the dataobjectname prefix must be stripped from the stored path";

    int put_size = 0;
    int put_aos = -1;
    AL_ASSERT_OK(al_begin_arraystruct_action(naos, "put_operation", "",
                                             &put_size, &put_aos));
    ASSERT_EQ(put_size, 1);
    std::string name, version, commit, repository;
    AL_ASSERT_OK(al_contract::read_char_array(put_aos, "name", &name));
    AL_ASSERT_OK(al_contract::read_char_array(put_aos, "version", &version));
    AL_ASSERT_OK(al_contract::read_char_array(put_aos, "commit", &commit));
    AL_ASSERT_OK(
        al_contract::read_char_array(put_aos, "repository", &repository));
    EXPECT_EQ(name, "alcontract");
    EXPECT_EQ(version, "0.0.0");
    EXPECT_EQ(commit, "0000000");
    EXPECT_EQ(repository, "in-repo");
    if (put_aos != 0) al_end_action(put_aos);

    int readback_size = -1;
    int readback_aos = -1;
    AL_ASSERT_OK(al_begin_arraystruct_action(naos, "readback", "",
                                             &readback_size, &readback_aos));
    EXPECT_EQ(readback_size, 0)
        << "ContractTestPlugin declares no readback capability for any path";
    if (readback_aos != 0) al_end_action(readback_aos);

    if (naos != 0) al_end_action(naos);
    al_end_action(op2);
    al_close_pulse(pctx, CLOSE_PULSE);
}

// ===========================================================================
// al_bind_readback_plugins / al_unbind_readback_plugins
// (FUNCTIONALITY_INVENTORY.md:378-389, User Cluster 3 + Part 3 cluster 4
// readback metadata) — issue #8 ownership sweep.
//
// Documented as "function called before/after a get()"
// (access_layer_plugin_manager.cpp:52,268): given metadata previously
// written by al_write_plugins_metadata, al_bind_readback_plugins reads it
// back and auto-registers + auto-binds any plugin that declared itself a
// readback provider for the stored path — the whole point being that the
// get()-side caller need not already know which plugins apply.
// al_unbind_readback_plugins then auto-unregisters exactly what it
// auto-bound. ContractTestPlugin opts into being its own readback provider
// for one path via AL_CONTRACT_PLUGIN_READBACK_PATH (test_plugin_fixture.cpp)
// — self-consistent with its own provenance, so the version check in
// AccessLayerPluginManager::bind_readback_plugins passes.
// ===========================================================================
// Same fixture as PluginTest, plus the one extra env var that opts
// ContractTestPlugin into readback capability for a single path.
class ReadbackPlugins : public PluginTest {
protected:
    void SetUp() override {
        PluginTest::SetUp();
        setenv("AL_CONTRACT_PLUGIN_READBACK_PATH", "comment", 1);
    }
    void TearDown() override {
        unsetenv("AL_CONTRACT_PLUGIN_READBACK_PATH");
        PluginTest::TearDown();
    }
};

TEST_F(ReadbackPlugins, BindReadbackPluginsAutoRegistersAndBindsFromStoredMetadata) {
    al_contract::TempBase base;
    al_contract::PulseId pulse{"test", "3", 12, 0};
    const int pctx = open_memory_pulse(base, pulse);

    // --- put-time: bind, then write metadata describing this plugin as its
    // own readback provider for "comment" ---
    AL_ASSERT_OK(al_register_plugin(kPlugin));
    AL_ASSERT_OK(al_bind_plugin("magnetics/comment", kPlugin));
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, "magnetics", "", WRITE_OP, &op));
    AL_ASSERT_OK(al_write_plugins_metadata(op));
    al_end_action(op);
    AL_ASSERT_OK(al_unregister_plugin(kPlugin));  // simulate a fresh session

    ASSERT_FALSE(is_registered(kPlugin))
        << "must start the get() side genuinely unregistered";

    // --- get-time: bind_readback_plugins must find the stored metadata and
    // re-register + bind the plugin on its own ---
    int op2 = -1;
    AL_ASSERT_OK(
        al_begin_global_action(pctx, "magnetics", "", READ_OP, &op2));
    AL_ASSERT_OK(al_bind_readback_plugins(op2));
    EXPECT_TRUE(is_registered(kPlugin))
        << "bind_readback_plugins must auto-register a plugin named in "
           "stored metadata";
    al_status_t rebind = al_bind_plugin("magnetics/comment", kPlugin);
    EXPECT_NE(rebind.code, 0)
        << "the path must already be bound after bind_readback_plugins (a "
           "double-bind is a documented error) -- indirect ABI-observable "
           "proof of the bind";

    AL_ASSERT_OK(al_unbind_readback_plugins(op2));
    EXPECT_FALSE(is_registered(kPlugin))
        << "unbind_readback_plugins must auto-unregister what it auto-bound";

    al_end_action(op2);
    al_close_pulse(pctx, CLOSE_PULSE);
}

}  // namespace

#endif  // !_WIN32
