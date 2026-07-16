// UDA-unique surface (issues #26 + #27, TRACEABILITY.md Part 5): the whole of
// UDA's unique surface -- cache modes, runtime DD loading, datapath,
// capability negotiation (#26, areas 1-6), plus fetch mode and the
// write/delete pins (#27, areas 7-10):
//   1. URI option surface (plugin, init_args, dd_version, cache_mode invalid
//      value, verbose, host/port).
//   2. Cache-mode invisibility (none/ids/struct identical reads; top-level
//      struct-mode data-loss defect; cache cleared on close).
//   3. Runtime DD loading (IDSDef.xml present / absent / wrong-version).
//   4. datapath partial-get via cache_mode=ids -- the only living use of
//      al_begin_global_action's datapath argument anywhere in the codebase
//      (CLAUDE.md's stale "no backend uses it" claim, corrected alongside
//      this test).
//   5. Version-drift check inertness (UDABackend::getVersion is a
//      placeholder on both sides of al_lowlevel.cpp's comparison).
//   6. Server-version-gated supportsTimeRangeOperation() (> 1.4.0).
//   7. Fetch mode: download, local-backend handoff, cache reuse.
//   8. Fetch mode: local_cache URI option override.
//   9. Fetch mode: the stale-cache / write-divergence pin -- a fetch-mode
//      write mutates only the local cache copy (there is no upload path), so
//      the divergent copy keeps being served.
//   10. Remote-mode write and delete pinned unsupported (no writeData /
//       deleteData handler on the reference server plugin).
//
// Same seed(HDF5)-then-reopen(UDA) shape as the rest of the tier (issue #24):
// the plain HDF5 backend seeds a pulse dir, the UDA backend in remote mode
// reopens the identical path and the test asserts what comes back through the
// public C ABI. Real DD-4.1.1 paths only (UDA validates every path against
// the schema it loads at startup, src/uda/uda_xml.cpp) -- see test_uda.cpp's
// characterization-discovered facts for the ids_properties/homogeneous_time
// precondition every remote read needs first.
#ifdef AL_CONTRACT_HAVE_UDA

#include "al_contract.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

// RAII env-var save/restore -- several characterizations here (runtime DD
// loading's absent/wrong-version rows) must mutate process-global getenv()
// state for exactly one UDABackend construction and then put it back,
// because UDABackend::UDABackend loads IDSDef.xml unconditionally from
// $IDSDEF_PATH/$IMAS_PREFIX at construction time (src/uda/uda_xml.cpp
// load_xml()) -- there is no other seam to inject a different DD file.
class ScopedEnv {
public:
    ScopedEnv(const char* name, const char* value) : name_(name) {
        const char* prev = std::getenv(name);
        had_prev_ = prev != nullptr;
        if (had_prev_) prev_ = prev;
        if (value) {
            setenv(name, value, 1);
        } else {
            unsetenv(name);
        }
    }
    ~ScopedEnv() {
        if (had_prev_) {
            setenv(name_.c_str(), prev_.c_str(), 1);
        } else {
            unsetenv(name_.c_str());
        }
    }
    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;

private:
    std::string name_;
    bool        had_prev_ = false;
    std::string prev_;
};

class UdaUniqueSurfaceTest : public ::testing::Test {
protected:
    void SetUp() override { AL_CONTRACT_SKIP_IF_UDA_UNCONFIGURED(); }

    al_contract::TempBase base_;

    std::string fresh_pulse_dir() const {
        const std::string dir = base_.str() + "/pulse";
        std::error_code    ec;
        std::filesystem::create_directories(dir, ec);
        return dir;
    }

    // Mirrors UDABackend::fetch_files' default cache-directory formula
    // (src/uda/uda_backend.cpp:427-429) exactly, so tests can assert the
    // fetch-mode cache landed where the spike (#23) characterized without a
    // seam into backend internals: cache_path = $TMPDIR/uda-cache-of-$USER
    // (or $TMPDIR/uda-cache if $USER is unset) joined with remote_path's
    // relative_path() (an absolute path's relative_path() strips the leading
    // "/", so this nests the whole pulse_dir under the cache root).
    static std::string default_fetch_cache_dir(const std::string& pulse_dir) {
        namespace fs = std::filesystem;
        const char* user = std::getenv("USER");
        fs::path    root = user && *user
                              ? fs::temp_directory_path() / ("uda-cache-of-" + std::string{user})
                              : fs::temp_directory_path() / "uda-cache";
        return (root / fs::path{pulse_dir}.relative_path()).string();
    }

    // Seed the standard equilibrium precondition
    // (ids_properties/homogeneous_time) plus one real DD scalar
    // (vacuum_toroidal_field/r0) through the plain HDF5 backend. Returns the
    // pulse dir.
    std::string seed_scalar(const std::string& pulse_dir, double r0 = 6.2) const {
        const std::string hdf5_uri = al_contract::hdf5_uri_for(pulse_dir);
        int                pulse_ctx = -1;
        EXPECT_EQ(al_begin_dataentry_action(hdf5_uri.c_str(), FORCE_CREATE_PULSE,
                                            &pulse_ctx)
                      .code,
                  0);
        int op = -1;
        EXPECT_EQ(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                         WRITE_OP, &op)
                      .code,
                  0);
        EXPECT_EQ(al_contract::write_data<int>(
                      op, "ids_properties/homogeneous_time", {}, {1})
                      .code,
                  0);
        EXPECT_EQ(al_contract::write_data<double>(
                      op, equilibrium_seed::kScalar, {}, {r0})
                      .code,
                  0);
        EXPECT_EQ(al_end_action(op).code, 0);
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
        return pulse_dir;
    }
};

// ===========================================================================
// Area 1: URI option surface.
// ===========================================================================

// cache_mode: any value outside {none, ids, struct} throws immediately, purely
// client-side (UDABackend::process_options, src/uda/uda_backend.cpp:298-308)
// -- no network round trip is needed to observe this, but the fixture's skip
// discipline is kept uniform with the rest of the tier regardless.
TEST_F(UdaUniqueSurfaceTest, InvalidCacheModeThrows) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);
    // Replace the valid cache_mode=none with an invalid value.
    std::string bad_uri = uri;
    const auto  pos     = bad_uri.find("cache_mode=none");
    ASSERT_NE(pos, std::string::npos);
    bad_uri.replace(pos, std::strlen("cache_mode=none"), "cache_mode=bogus");

    int         pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(bad_uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_NE(s.code, 0) << "an invalid cache_mode value must be rejected";
    EXPECT_NE(std::string(s.message).find("invalid cache mode"), std::string::npos)
        << "expected the documented rejection message, got: " << s.message;
    if (s.code == 0) {
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}

// verbose: "1"/"true" -> debug tracing on stdout; anything else (including
// absent) -> off (UDABackend::process_options). Characterized by capturing
// stdout around the open call rather than asserting on internal state.
TEST_F(UdaUniqueSurfaceTest, VerboseTrueEmitsDebugTracingOnStdout) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri =
        al_contract::uda_hdf5_uri_for(pulse_dir, "none", "verbose=true");

    testing::internal::CaptureStdout();
    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    const std::string captured = testing::internal::GetCapturedStdout();
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));

    EXPECT_NE(captured.find("UDABackend openPulse"), std::string::npos)
        << "verbose=true must emit UDABackend's debug tracing; captured: "
        << captured;
}

TEST_F(UdaUniqueSurfaceTest, VerboseAbsentEmitsNoDebugTracing) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri =
        al_contract::uda_hdf5_uri_for(pulse_dir);  // no verbose= at all

    testing::internal::CaptureStdout();
    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    const std::string captured = testing::internal::GetCapturedStdout();
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));

    EXPECT_EQ(captured.find("UDABackend openPulse"), std::string::npos)
        << "verbose unset must default to no debug tracing; captured: "
        << captured;
}

// plugin: the URI option that names which server-side plugin to talk to
// (default "IMAS", process_options + openPulse). A name the reference stack
// never registered fails at open -- the client's init directive
// ("<plugin>::init(...)") reaches a real server that has no such plugin.
TEST_F(UdaUniqueSurfaceTest, PluginOptionNamingUnregisteredPluginFailsAtOpen) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = al_contract::uda_hdf5_uri_for(
        pulse_dir, "none", "plugin=NoSuchImasPlugin");

    int         pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_NE(s.code, 0) << "an unregistered plugin name must fail at open, "
                            "not silently fall back to the default";
    if (s.code == 0) {
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}

// init_args: passed through to "<plugin>::init(<args>)" with ';' rewritten to
// ',' (UDABackend::openPulse, src/uda/uda_backend.cpp:487-493). The reference
// IMAS server plugin's init() (imas_plugin.cpp: `plugin.init(plugin_interface)`
// inside handle_request, called unconditionally before dispatch) takes no
// arguments from the request at all -- it only tracks a per-connection _init
// bool. So init_args is accepted (never rejected) but has no observable
// effect on this reference plugin: a read with arbitrary init_args succeeds
// identically to one without.
TEST_F(UdaUniqueSurfaceTest, InitArgsAcceptedAndIgnoredByReferencePlugin) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string uri = al_contract::uda_hdf5_uri_for(
        pulse_dir, "none", "init_args=some_key=1;other_key=2");

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                        READ_OP, &op));
    std::vector<int>    shape;
    std::vector<double> data;
    AL_EXPECT_OK(al_contract::read_data<double>(op, equilibrium_seed::kScalar, 0,
                                                &shape, &data));
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], 6.2) << "arbitrary init_args must not change read "
                               "behavior against the reference plugin";
    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
}

// dd_version: an explicit override is forwarded verbatim as a query param on
// every directive the client builds (e.g. src/uda/uda_backend.cpp:657-658),
// but no backend on either side of the wire (client schema resolution is
// driven purely by the file loaded at $IDSDEF_PATH; the server's HDF5 backend
// is DD-agnostic) ever inspects that forwarded string. An arbitrary override
// value is therefore accepted and has no observable effect on a real-path
// read -- a distinct, narrower finding than area 3's DD *file* characterization
// below (this is about the per-request query option, not the client's loaded
// schema).
TEST_F(UdaUniqueSurfaceTest, DdVersionOverrideAcceptedWithNoObservableEffect) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string uri = al_contract::uda_hdf5_uri_for(
        pulse_dir, "none", "dd_version=not.a.real.version");

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                        READ_OP, &op));
    std::vector<int>    shape;
    std::vector<double> data;
    AL_EXPECT_OK(al_contract::read_data<double>(op, equilibrium_seed::kScalar, 0,
                                                &shape, &data));
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], 6.2) << "an arbitrary dd_version override must not "
                               "change read behavior -- no backend consults it";
    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
}

// host/port: parsed from the URI authority (UDABackend constructor +
// openPulse, both call uda::Client::setServerHostName/setServerPort when the
// authority carries a non-empty host / non-zero port). A host that resolves
// but a port nothing listens on fails to connect distinctly from every other
// failure mode characterized above (those are all post-connection request
// failures; this one never reaches the plugin dispatch at all).
TEST_F(UdaUniqueSurfaceTest, WrongPortFailsToConnectDistinctlyFromRequestFailures) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    // Port 1 is reserved (tcpmux) and nothing UDA-shaped listens there.
    const std::string uri = "imas://localhost:1/uda?backend=hdf5&cache_mode=none&path=" +
                             pulse_dir;

    int         pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_NE(s.code, 0) << "a port nothing listens on must fail at open";
    if (s.code == 0) {
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}

// ===========================================================================
// Area 2: cache-mode invisibility.
// ===========================================================================
// The three cache modes are performance knobs and must not change physics
// values: none takes readData's direct per-field IMAS::get path; ids
// pre-fetches the whole (or datapath-scoped, area 4) IDS at beginAction time;
// struct pre-fetches each struct_array's leaves at its own
// begin_arraystruct_action. A field covered by all three (a static,
// non-dynamic leaf nested one level inside a struct_array, so struct mode's
// own population point is reached) does return the exact same value. The D2
// pair below pins the defect for a top-level field that struct mode currently
// drops instead.
TEST_F(UdaUniqueSurfaceTest, CacheModeNoneIdsStructAgreeForAosCoveredField) {
    const std::string pulse_dir = fresh_pulse_dir();
    const std::vector<int> kShape = {2, 3, 2};  // shape_for_rank(3)
    const std::vector<int> kWritten = {1, 2, 3, 4, 5, 6, 7, 8, 9,
                                       10, 11, 12};

    // --- seed temporary/constant_integer3d(1 element)/value via HDF5 -------
    {
        const std::string hdf5_uri = al_contract::hdf5_uri_for(pulse_dir);
        int                pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(
            al_begin_global_action(pulse_ctx, "temporary", "", WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<int>(
            op, "ids_properties/homogeneous_time", {}, {1}));
        int size = 1;
        int aos  = -1;
        AL_EXPECT_OK(
            al_begin_arraystruct_action(op, "constant_integer3d", "", &size, &aos));
        AL_EXPECT_OK(al_contract::write_data<int>(aos, "value", kShape, kWritten));
        AL_EXPECT_OK(al_end_action(aos));
        AL_ASSERT_OK(al_end_action(op));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    // --- reopen through UDA under each cache mode ---------------------------
    for (const char* mode : {"none", "ids", "struct"}) {
        SCOPED_TRACE(mode);
        const std::string uri =
            al_contract::uda_hdf5_uri_for(pulse_dir, mode);
        int pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(
            al_begin_global_action(pulse_ctx, "temporary", "", READ_OP, &op));
        int rsize = 0;
        int raos  = -1;
        AL_ASSERT_OK(
            al_begin_arraystruct_action(op, "constant_integer3d", "", &rsize, &raos));
        EXPECT_EQ(rsize, 1);
        std::vector<int> shape;
        std::vector<int> data;
        AL_EXPECT_OK(al_contract::read_data<int>(raos, "value", 3, &shape, &data));
        EXPECT_EQ(data, kWritten)
            << "cache_mode=" << mode << " must return the same value as every "
                                        "other cache mode for a field its own "
                                        "population point covers";
        AL_EXPECT_OK(al_end_action(raos));
        AL_EXPECT_OK(al_end_action(op));
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}

// NEW DEFECT (xfail): cache-mode invisibility currently does not extend to a
// top-level field under struct mode. The mode's implementation only populates
// its cache inside beginArraystructAction:
// UDABackend::beginAction only populates the cache for cache_mode=ids
// (src/uda/uda_backend.cpp:1016); struct mode populates lazily, per
// struct_array, only inside beginArraystructAction (cache_.count(path) miss ->
// populate_cache(ids, path, ...) scoped to that one struct_array's subtree,
// uda_backend.cpp:927-934). A plain top-level field that is never inside any
// struct_array the caller enters is therefore never populated under struct
// mode, and readData's fallthrough (`cache_.count(path)` miss, not one of the
// two homogeneous-time/version preconditions, cache_mode_ != None ->
// `return 0`, uda_backend.cpp:724-726) reports it as the ordinary absent-leaf
// contract. That loses a genuinely written physics value based solely on the
// cache-mode selection, violating issue #26's requirement that none/ids/
// struct return identical data.

// CORRECT-CONTRACT, expected-fail (DISABLED_): struct mode must return the
// same written value as none and ids for a top-level field.
TEST_F(UdaUniqueSurfaceTest, DISABLED_CacheModeStructPreservesTopLevelField) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string uri =
        al_contract::uda_hdf5_uri_for(pulse_dir, "struct");

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                        READ_OP, &op));
    std::vector<int>    shape;
    std::vector<double> data;
    AL_EXPECT_OK(al_contract::read_data<double>(op, equilibrium_seed::kScalar,
                                                0, &shape, &data));
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], 6.2)
        << "cache_mode=struct must preserve a written top-level physics value "
           "exactly as cache_mode=none and cache_mode=ids do";
    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
}

// CURRENT-BEHAVIOR tripwire: struct mode reports success but returns the
// ordinary absent-leaf sentinel for the genuinely written top-level field.
TEST_F(UdaUniqueSurfaceTest,
       CacheModeStructCurrentlyReturnsAbsentSentinelForTopLevelField) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string uri =
        al_contract::uda_hdf5_uri_for(pulse_dir, "struct");

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                        READ_OP, &op));
    std::vector<int>    shape;
    std::vector<double> data;
    al_status_t s = al_contract::read_data<double>(op, equilibrium_seed::kScalar,
                                                    0, &shape, &data);
    AL_EXPECT_OK(s) << "reading a top-level field under cache_mode=struct "
                       "reports success (the ordinary absent-leaf contract), "
                       "not an error";
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], al_contract::kEmptyDouble)
        << "struct mode now preserves the written value -- enable "
           "DISABLED_CacheModeStructPreservesTopLevelField; until then its "
           "per-struct_array population scope must keep reproducing this "
           "known data-loss defect";
    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
}

// ===========================================================================
// NEW DEFECT (xfail): closing a UDA remote-mode session leaks a server-side
// pulse handle, so a subsequent LOCAL open of the identical on-disk file can
// block on HDF5's file lock.
//
// Root cause, confirmed empirically by tracing the exact directives exchanged
// (verbose=1): UDABackend::openPulse/closePulse both strip `cache_mode` and
// `verbose` before building the `uri=` argument they send to the server
// (src/uda/uda_backend.cpp, e.g. closePulse:557-564), so
// IMAS::open(uri='imas:hdf5?dd_version=4.1.1&path=P') and
// IMAS::close(uri='imas:hdf5?dd_version=4.1.1&path=P') use the identical
// key. But two of the *read*-triggering directive builders don't follow that
// convention: UDABackend::readData's cache_mode=None branch
// (uda_backend.cpp:654-659) and UDABackend::populate_cache
// (uda_backend.cpp:807-812, used by cache_mode=ids and struct alike) each
// build their own `IMAS::get(uri=...)` directive from the SAME query object
// but only ever remove "backend" -- never cache_mode/verbose -- so their
// `uri=` differs from the one `open`/`close` agreed on (confirmed via the
// verbose trace: `get(uri='imas:hdf5?...&cache_mode=none')` vs.
// `open(uri='imas:hdf5?...')`, no cache_mode at all). By contrast
// beginArraystructAction's own None-mode branch (uda_backend.cpp:954-961)
// does strip both, matching open/close -- so this is specifically a
// readData/populate_cache inconsistency, not a blanket "every get() path"
// one. The reference `IMAS` server plugin's own `get()` handler
// (source/imas/imas_plugin.cpp) treats an unrecognized `uri=` as a brand new
// pulse and implicit-opens it (`if (_open_entries.count(uri) == 0) {
// open(...); }`) -- creating a SECOND `_open_entries` record, with its own
// real `al_begin_dataentry_action` and therefore its own HDF5 file handle,
// keyed under a uri the client's subsequent `close()` directive can never
// match (it only ever sends the stripped key). That second handle is never
// closed: it leaks for the lifetime of the server's per-connection process.
// HDF5's default file locking then blocks a later LOCAL open of the
// identical on-disk file until that server process exits -- observed
// directly: after one full UDA seed(HDF5)-then-reopen(UDA)-then-close(UDA)
// cycle, on the exact same pulse_dir a plain `imas:hdf5?path=...` OPEN_PULSE
// in the *same test process* fails with "Unable to open HDF5 master file",
// even though the file is untouched and still on disk. Reproduces
// identically whether the read went through readData's None-mode "get" path
// or populate_cache's ids/struct-mode path (both hit the same unstripped-uri
// bug), so this is not a cache_mode=ids-specific finding despite living in
// the cache-mode area -- it's the actual mechanism behind the "cache cleared
// on close" question this area set out to characterize: the RAM cache_ *is*
// cleared (UDABackend::closePulse, uda_backend.cpp:554), but a server-side
// resource opened on the read path outlives it regardless.
// ===========================================================================

// Runs one full seed(HDF5)-then-reopen(UDA)-then-close(UDA) cycle, then
// attempts a plain local HDF5 OPEN_PULSE against the identical pulse dir in
// this same process, returning that attempt's status -- shared by the
// correct-contract test and its tripwire below (mirrors UdaAosKnownDefects'
// pattern in test_uda_breadth.cpp).
al_status_t local_reopen_status_after_uda_session_closes(const char* cache_mode) {
    al_contract::TempBase base;
    const std::string     pulse_dir = base.str() + "/pulse";
    std::error_code       ec;
    std::filesystem::create_directories(pulse_dir, ec);

    const std::string hdf5_uri = al_contract::hdf5_uri_for(pulse_dir);
    {
        int pulse_ctx = -1;
        EXPECT_EQ(al_begin_dataentry_action(hdf5_uri.c_str(), FORCE_CREATE_PULSE,
                                            &pulse_ctx)
                      .code,
                  0);
        int op = -1;
        EXPECT_EQ(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                         WRITE_OP, &op)
                      .code,
                  0);
        EXPECT_EQ(al_contract::write_data<int>(
                      op, "ids_properties/homogeneous_time", {}, {1})
                      .code,
                  0);
        EXPECT_EQ(al_contract::write_data<double>(
                      op, equilibrium_seed::kScalar, {}, {6.2})
                      .code,
                  0);
        EXPECT_EQ(al_end_action(op).code, 0);
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    {
        const std::string uda_uri =
            al_contract::uda_hdf5_uri_for(pulse_dir, cache_mode);
        int pulse_ctx = -1;
        EXPECT_EQ(
            al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE, &pulse_ctx).code,
            0);
        int op = -1;
        EXPECT_EQ(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                         READ_OP, &op)
                      .code,
                  0);
        std::vector<int>    shape;
        std::vector<double> data;
        EXPECT_EQ(al_contract::read_data<double>(op, equilibrium_seed::kScalar, 0,
                                                  &shape, &data)
                      .code,
                  0);
        AL_EXPECT_OK(al_end_action(op));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    int pctx = -1;
    return al_begin_dataentry_action(hdf5_uri.c_str(), OPEN_PULSE, &pctx);
}

// CORRECT-CONTRACT, expected-fail (DISABLED_): closing a UDA session must
// release every server-side handle it opened, so a subsequent local open of
// the identical file must succeed immediately.
TEST_F(UdaUniqueSurfaceTest, DISABLED_ClosingUdaSessionReleasesEveryServerSideHandle) {
    al_status_t s = local_reopen_status_after_uda_session_closes("none");
    AL_EXPECT_OK(s) << "a local HDF5 open of the same pulse dir must succeed "
                       "right after the UDA session that read it has closed";
}

// CURRENT-BEHAVIOR tripwire: the local reopen fails today, every time,
// because of the leaked server-side handle described above.
TEST_F(UdaUniqueSurfaceTest, ClosingUdaSessionCurrentlyLeaksServerSideHandle) {
    al_status_t s = local_reopen_status_after_uda_session_closes("none");
    EXPECT_NE(s.code, 0)
        << "a local reopen after a UDA session now succeeds -- enable "
           "DISABLED_ClosingUdaSessionReleasesEveryServerSideHandle (the "
           "get()-directive uri-stripping bug in readData/populate_cache/"
           "beginArraystructAction, src/uda/uda_backend.cpp, is now fixed)";
    EXPECT_NE(std::string(s.message).find("Unable to open HDF5 master file"),
              std::string::npos)
        << "expected the documented HDF5-lock failure signature, got a "
           "different error instead: " << s.message;
}

// ===========================================================================
// Area 3: runtime DD loading (present / absent / wrong-version).
//
// UDABackend::UDABackend unconditionally calls imas::uda::load_xml() +
// get_dd_version() (src/uda/uda_xml.cpp), so every open pays this cost and
// every failure mode surfaces at al_begin_dataentry_action.
// ===========================================================================

// Present (the default reference-stack configuration): already exercised by
// every other test in the tier; restated here as the baseline area-3 row.
TEST_F(UdaUniqueSurfaceTest, DdPresentLoadsAndOpenSucceeds) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);
    int                pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
}

// Absent: neither $IDSDEF_PATH nor $IMAS_PREFIX set -> load_xml() throws
// before any network access at all (the constructor loads the DD before it
// ever touches the wire), surfaced through the C ABI as a non-zero
// al_status_t carrying the exact message.
TEST_F(UdaUniqueSurfaceTest, DdAbsentNeitherEnvVarSetFailsWithClearMessage) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);

    ScopedEnv idsdef(/*name=*/"IDSDEF_PATH", /*value=*/nullptr);
    ScopedEnv prefix(/*name=*/"IMAS_PREFIX", /*value=*/nullptr);

    int         pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_NE(s.code, 0) << "open must fail when neither DD env var is set";
    EXPECT_NE(std::string(s.message).find(
                  "neither IMAS_PREFIX or IDSDEF_PATH"),
              std::string::npos)
        << "expected the documented missing-env-var message, got: " << s.message;
    if (s.code == 0) {
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}

// Absent: $IDSDEF_PATH set but points at a file that does not exist ->
// load_xml()'s pugixml parse fails, distinct message from the unset-env case.
TEST_F(UdaUniqueSurfaceTest, DdAbsentFileMissingAtIdsDefPathFailsWithClearMessage) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);

    ScopedEnv idsdef(/*name=*/"IDSDEF_PATH",
                    /*value=*/"/nonexistent/path/IDSDef.xml");

    int         pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_NE(s.code, 0) << "open must fail when IDSDEF_PATH names a missing file";
    EXPECT_NE(std::string(s.message).find("IDSDef.xml not found"), std::string::npos)
        << "expected the documented not-found message, got: " << s.message;
    if (s.code == 0) {
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}

// NEW DEFECT (xfail): $IDSDEF_PATH can point at a real, structurally valid,
// but wrong-version DD without the client rejecting it. The correct contract
// and current-behavior tripwire below use an older DD:
// *older* DD (3.42.0, IDSDEF_PATH_OLDER, docker/uda/Dockerfile) than the
// reference stack's own DD 4.1.1. load_xml()/get_dd_version() do not compare
// the loaded file's version against anything -- there is no cross-check
// against the actual data's DD version, the server's DD version, or any
// pinned expectation. The load succeeds silently, and a real DD path that
// exists identically in both schemas (vacuum_toroidal_field/r0, present at
// the same nesting in both 3.42.0 and 4.1.1) round-trips exactly as if the
// "correct" DD had been loaded: nothing signals that a version-drifted
// schema is in use. This is the DD-loading analogue of area 5's version-drift
// inertness below -- distinct mechanism (a schema file walked at runtime vs.
// a placeholder getVersion()), same practical consequence: nothing here would
// catch a genuine cross-version drift for a path that happens to be
// structurally stable, exactly the class of risk NORTH_STAR.md flags for the
// COCOS-sign-flip-bearing paths that are *not* stable across versions (a
// mismatch there would corrupt values silently rather than erroring, since
// this mechanism checks path existence only, never semantic version
// agreement). Skipped, not failed, if the second DD version was not baked
// into this reference image (docker/uda/Dockerfile).

// CORRECT-CONTRACT, expected-fail (DISABLED_): opening data written against a
// different DD must be refused rather than silently treating path agreement
// as semantic version compatibility.
TEST_F(UdaUniqueSurfaceTest, DISABLED_DdWrongVersionIsRejected) {
    const char* older_dd = std::getenv("IDSDEF_PATH_OLDER");
    if (!older_dd || !*older_dd) {
        GTEST_SKIP() << "IDSDEF_PATH_OLDER is unset -- the reference image "
                        "does not carry a second DD version (docker/uda/"
                        "Dockerfile); this row is skipped, not failed.";
    }

    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);

    ScopedEnv idsdef(/*name=*/"IDSDEF_PATH", /*value=*/older_dd);

    int         pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_NE(s.code, 0)
        << "a DD 3.42.0 client must refuse data served from the DD 4.1.1 "
           "reference stack instead of silently assuming compatibility";
    if (s.code == 0) {
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}

// CURRENT-BEHAVIOR tripwire: the wrong-version DD loads silently and a path
// stable across the two DD versions reads as if the versions agreed.
TEST_F(UdaUniqueSurfaceTest,
       DdWrongVersionCurrentlyLoadsSilentlyWithNoCrossVersionCheck) {
    const char* older_dd = std::getenv("IDSDEF_PATH_OLDER");
    if (!older_dd || !*older_dd) {
        GTEST_SKIP() << "IDSDEF_PATH_OLDER is unset -- the reference image "
                        "does not carry a second DD version (docker/uda/"
                        "Dockerfile); this row is skipped, not failed.";
    }

    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);

    ScopedEnv idsdef(/*name=*/"IDSDEF_PATH", /*value=*/older_dd);

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx))
        << "loading a real but version-drifted DD file must not itself fail";
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                        READ_OP, &op));
    std::vector<int>    shape;
    std::vector<double> data;
    AL_EXPECT_OK(al_contract::read_data<double>(op, equilibrium_seed::kScalar, 0,
                                                &shape, &data))
        << "a path stable across DD versions must still resolve and read "
           "back correctly under the wrong-version schema";
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], 6.2)
        << "wrong-version loading is now rejected -- enable "
           "DISABLED_DdWrongVersionIsRejected; until then this stable path "
           "must keep exposing the silent cross-version acceptance defect";
    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
}

// ===========================================================================
// Area 4: datapath partial-get via cache_mode=ids.
//
// The only living use of al_begin_global_action's datapath argument
// (FUNCTIONALITY_INVENTORY.md's Cluster 2 entry, CLAUDE.md, both already
// correct on this point -- this test is the evidence that claim cites).
// UDABackend::beginAction (src/uda/uda_backend.cpp:1016-1032) uses
// getDatapath() to scope populate_cache()'s subtree fetch: with
// cache_mode=ids and a non-empty datapath, only paths under
// "<ids>/<datapath>" are pre-fetched into the RAM cache. readData
// (src/uda/uda_backend.cpp:601-727) falls through to `return 0` (reported
// through the C ABI as the ordinary "absent leaf" contract, matching
// UdaBreadthTest.ReadOfUnwrittenLeafOnSeededIdsReturnsHdf5AbsentSentinel) for
// any field neither found in the cache nor one of the two homogeneous-
// time/version preconditions -- so a real, written field *outside* the
// datapath's scope reads back as if it had never been written at all, even
// though the plain HDF5 backend underneath genuinely has it.
// ===========================================================================
TEST_F(UdaUniqueSurfaceTest, DatapathScopesCachePopulationFieldOutsideScopeReadsAsAbsent) {
    const std::string pulse_dir = fresh_pulse_dir();

    // --- seed two real DD fields in different subtrees via HDF5 -------------
    {
        const std::string hdf5_uri = al_contract::hdf5_uri_for(pulse_dir);
        int                pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds,
                                            "", WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<int>(
            op, "ids_properties/homogeneous_time", {}, {1}));
        // In scope for datapath="vacuum_toroidal_field".
        AL_EXPECT_OK(al_contract::write_data<double>(
            op, equilibrium_seed::kScalar, {}, {6.2}));
        // Out of scope: a real DD-4.1.1 leaf under a sibling subtree.
        AL_EXPECT_OK(al_contract::write_data<int>(op, "code/output_flag", {},
                                                   {42}));
        AL_ASSERT_OK(al_end_action(op));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    // --- reopen through UDA, cache_mode=ids, datapath scoped to one subtree ---
    const std::string uda_uri =
        al_contract::uda_hdf5_uri_for(pulse_dir, "ids");
    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uda_uri.c_str(), OPEN_PULSE, &pulse_ctx));

    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds,
                                        /*datapath=*/"vacuum_toroidal_field",
                                        READ_OP, &op));

    // In-scope field: real data comes back.
    {
        std::vector<int>    shape;
        std::vector<double> data;
        AL_EXPECT_OK(al_contract::read_data<double>(
            op, equilibrium_seed::kScalar, 0, &shape, &data));
        ASSERT_EQ(data.size(), 1u);
        EXPECT_EQ(data[0], 6.2)
            << "the field inside the datapath's own subtree must round-trip";
    }

    // Out-of-scope field: silently reads as absent, even though it was
    // genuinely written and the plain HDF5 backend has it.
    {
        std::vector<int> shape;
        std::vector<int> data;
        al_status_t      s = al_contract::read_data<int>(op, "code/output_flag",
                                                         0, &shape, &data);
        AL_EXPECT_OK(s) << "reading outside the datapath's scope reports "
                           "success (the ordinary absent-leaf contract), not "
                           "an error";
        ASSERT_EQ(data.size(), 1u);
        EXPECT_EQ(data[0], al_contract::kEmptyInt)
            << "a real, written field outside the datapath's cache-populated "
               "subtree reads back as absent through cache_mode=ids -- the "
               "datapath scope genuinely hides data outside it, not just "
               "prioritizes fetch order";
    }

    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
}

// ===========================================================================
// Area 5: version-drift check inertness (reworked by issue #39: the tripwire
// now uses a genuinely MISMATCHED stored fixture).
//
// al_begin_dataentry_action (src/al_lowlevel.cpp:868-883) compares
// backend->getVersion(NULL) (the "compiled" version) against
// backend->getVersion(pctx) (the "stored pulse" version) on
// OPEN_PULSE/FORCE_OPEN_PULSE, throwing on a mismatch -- the same mechanism
// MDSplus's version-drift row (TRACEABILITY.md Part 4, test_mdsplus_
// version_drift.cpp) exercises with a genuine mismatch. For UDA
// (src/uda/uda_backend.cpp:277-290) both sides are placeholders:
// getVersion(NULL) returns the compiled {UDA_BACKEND_VERSION_MAJOR,
// UDA_BACKEND_VERSION_MINOR} = {0, 0} (uda_backend.h:37-38); getVersion(pctx)
// with a non-null ctx always returns {0, 0} too ("temporary placeholder",
// never reads anything from the actual remote pulse). The comparison is
// therefore always false -- the drift check can never fire for UDA,
// regardless of what is genuinely stored remotely.
//
// A fresh matching pulse cannot discriminate that inertness: a future correct
// implementation would ALSO accept it. The pins below therefore open a pulse
// whose stored HDF5_BACKEND_VERSION was deliberately rewritten to an
// impossible value ("999.0") by the isolated fixture producer
// hdf5_fixture_tool (issue #36) -- fixture preparation only; every asserted
// behavior stays on the public C ABI.
//
// Empirically characterized against the reference stack (issue #39): the
// mismatched pulse does NOT open -- but the refusal comes from the SERVER
// side, not the client's drift check. The server's IMAS plugin opens the
// pulse with its own IMAS-Core, whose HDF5BackendFactory rejects any stored
// version other than "1.0" (see test_hdf5_version_drift.cpp), and that error
// is forwarded to the client as UNKNOWN_ERR with the backend's "No backend
// writer with version: 999.0" text. The client-side check stays inert: it
// never produces its own LOWLEVEL_ERR "Compatibility ..." refusal. The
// active tripwire pins exactly that split; implementing real UDA
// stored-version retrieval flips the refusal to the client's own message and
// makes the DISABLED_ correct-contract test pass.
// ===========================================================================

#ifdef AL_CONTRACT_HDF5_FIXTURE_TOOL
namespace {
// Seeds a pulse, then rewrites its stored backend version out-of-band.
std::string make_version_drifted_pulse(const std::string& pulse_dir) {
    const std::string cmd = std::string("\"") + AL_CONTRACT_HDF5_FIXTURE_TOOL +
                            "\" \"" + pulse_dir + "\" \"999.0\"";
    EXPECT_EQ(std::system(cmd.c_str()), 0) << "fixture tool failed: " << cmd;
    return pulse_dir;
}
}  // namespace

// Baseline: a matching stored version opens through UDA.
TEST_F(UdaUniqueSurfaceTest, MatchingStoredVersionOpensThroughUda) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);
    int               pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    AL_EXPECT_OK(s);
    if (s.code == 0) {
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}

// CORRECT-CONTRACT, expected-fail (DISABLED_): the CLIENT's own drift check
// must refuse the mismatched pulse -- LOWLEVEL_ERR with al_lowlevel.cpp's
// "Compatibility ..." diagnostic -- instead of comparing two hardcoded {0,0}
// placeholders and relying on a downstream server error to catch the drift.
TEST_F(UdaUniqueSurfaceTest,
       DISABLED_OpenRefusesMismatchedStoredBackendVersion) {
    const std::string pulse_dir =
        make_version_drifted_pulse(seed_scalar(fresh_pulse_dir()));
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);
    int               pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_EQ(s.code, alerror::lowlevel_err)
        << "the client-side drift check must refuse the mismatch itself; "
           "message: " << s.message;
    EXPECT_NE(std::string(s.message).find("Compatibility"), std::string::npos)
        << "message: " << s.message;
    if (s.code == 0) {
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}

// CURRENT-BEHAVIOR tripwire: the mismatched pulse is refused, but by the
// SERVER-side HDF5 open (forwarded as UNKNOWN_ERR naming the stored version),
// never by the client's own inert drift check. If this starts failing, the
// refusal moved (or vanished) -- most likely real client-side stored-version
// retrieval was implemented: enable the DISABLED_ correct-contract test.
TEST_F(UdaUniqueSurfaceTest,
       VersionDriftCheckCurrentlyDefersToServerSideRefusal) {
    const std::string pulse_dir =
        make_version_drifted_pulse(seed_scalar(fresh_pulse_dir()));
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);
    int                pulse_ctx = -1;
    al_status_t        s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_EQ(s.code, alerror::unknown_err)
        << "tripwire: today's refusal is the forwarded server-side backend "
           "error, surfacing as UNKNOWN_ERR; message: " << s.message;
    EXPECT_NE(std::string(s.message).find("No backend writer with version"),
              std::string::npos)
        << "the refusal must originate from the server-side HDF5 factory; "
           "message: " << s.message;
    EXPECT_NE(std::string(s.message).find("999.0"), std::string::npos)
        << "message: " << s.message;
    EXPECT_EQ(std::string(s.message).find("Compatibility"), std::string::npos)
        << "tripwire: the client-side drift check must currently stay inert "
           "(placeholder {0,0} comparison) -- if its own 'Compatibility' "
           "refusal appears, enable "
           "DISABLED_OpenRefusesMismatchedStoredBackendVersion; message: "
        << s.message;
    if (s.code == 0) {
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}
#endif  // AL_CONTRACT_HDF5_FIXTURE_TOOL

// ===========================================================================
// Area 6: server-version-gated supportsTimeRangeOperation().
//
// UDABackend::supportsTimeRangeOperation() delegates to
// supportsTimeDataInterpolation() (uda_backend.h:194-196), which issues
// "<plugin>::version()" to the server and returns `version > "1.4.0"_v`
// (src/uda/uda_backend.cpp:1189-1223, semver.hpp) -- the only capability flag
// in the codebase gated on a remote party's reported version rather than a
// compile-time constant. Against this reference stack the plugin reports
// 1.8.0 (docker/uda/README.md), so the capability is granted: an
// al_begin_timerange_action call must pass al_lowlevel.cpp's capability gate
// (src/al_lowlevel.cpp:1053, "Selected backend does not support time range
// operations.") rather than being refused outright. This is deliberately
// scoped to the *capability* boundary only -- the subsequent read on this
// same call already has its own pinned defect
// (UdaSliceAndTimeRange.TimeRangeReadWithoutResamplingCurrentlyFailsWithUnknownInterpMode,
// test_uda_breadth.cpp), which is a separate, uninitialized-interpmode bug
// unrelated to capability negotiation.
// ===========================================================================
TEST_F(UdaUniqueSurfaceTest, TimeRangeCapabilityGrantedByReferenceServerVersion180) {
    const std::string pulse_dir = fresh_pulse_dir();
    {
        const std::string hdf5_uri = al_contract::hdf5_uri_for(pulse_dir);
        int                pulse_ctx = -1;
        AL_ASSERT_OK(al_begin_dataentry_action(hdf5_uri.c_str(),
                                               FORCE_CREATE_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds,
                                            "", WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<int>(
            op, "ids_properties/homogeneous_time", {}, {1}));
        double t[2]     = {1.0, 2.0};
        double v[2]     = {10.0, 20.0};
        int    shape[1] = {2};
        AL_EXPECT_OK(al_write_data(op, "time", "time", t, DOUBLE_DATA, 1, shape));
        AL_EXPECT_OK(al_write_data(op, "vacuum_toroidal_field/b0", "time", v,
                                   DOUBLE_DATA, 1, shape));
        AL_ASSERT_OK(al_end_action(op));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);
    int                pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));

    int    op          = -1;
    double dtime       = 0.0;
    int    dtime_shape = 0;
    al_status_t s = al_begin_timerange_action(pulse_ctx, equilibrium_seed::kIds,
                                              READ_OP, 1.0, 2.0, &dtime,
                                              &dtime_shape, UNDEFINED_INTERP, &op);
    EXPECT_EQ(std::string(s.message).find(
                  "does not support time range operations"),
              std::string::npos)
        << "the reference server (plugin 1.8.0 > 1.4.0) must grant the "
           "time-range capability -- al_begin_timerange_action must not be "
           "refused with the 'does not support time range operations' "
           "message; got: " << s.message;
    if (s.code == 0) {
        AL_EXPECT_OK(al_end_action(op));
    }
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
}

// ===========================================================================
// Area 7: fetch mode -- download, local-backend handoff, cache reuse.
//
// UDABackend::UDABackend calls fetch_files(uri) unconditionally when
// fetch=1/true (src/uda/uda_backend.cpp:272-274), which lists the remote
// pulse's files via "<plugin>::listFiles(...)" (dispatched to the reference
// server's BYTES plugin -- built and registered by ukaea/uda's own
// source/plugins/CMakeLists.txt by default, confirmed present in
// udaPlugins.conf on this reference image; the fetch-mode subtask's own
// server-side dependency, distinct from the IMAS plugin every other test in
// this tier uses), then for backend=hdf5 downloads only "master.h5" via
// "BYTES::read(...)" (fetch_files(backend), uda_backend.cpp:391-393). Once
// access_local_ is set, every subsequent Backend virtual (openPulse,
// readData, writeData, deleteData, closePulse, ...) delegates straight to a
// freshly constructed local backend pointed at the downloaded copy -- so a
// fetch-mode pulse behaves exactly like the plain local backend from here
// on, confirmed by reading back the seeded scalar unchanged.
// ===========================================================================
TEST_F(UdaUniqueSurfaceTest, FetchModeDownloadsHandsOffToLocalBackendAndReusesCacheOnReopen) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string cache_dir = default_fetch_cache_dir(pulse_dir);
    // Guard against a stale cache dir from a prior interrupted run polluting
    // this test's "must genuinely download" assertion.
    std::error_code ec;
    std::filesystem::remove_all(cache_dir, ec);

    const std::string uri = al_contract::uda_uri_base() +
                             "?fetch=1&backend=hdf5&path=" + pulse_dir +
                             "&verbose=1";

    // --- first open: must genuinely download -------------------------------
    testing::internal::CaptureStdout();
    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    std::string captured = testing::internal::GetCapturedStdout();

    EXPECT_NE(captured.find("UDABackend creating local cache directory"),
              std::string::npos)
        << "first fetch-mode open must create a fresh local cache directory; "
           "captured: " << captured;
    EXPECT_NE(captured.find("UDABackend files downloaded to"), std::string::npos)
        << "first fetch-mode open must report a genuine download; captured: "
        << captured;
    ASSERT_TRUE(std::filesystem::exists(cache_dir))
        << "the cache directory must be created at the location #23's spike "
           "characterized (local_cache override or "
           "$TMPDIR/uda-cache-of-$USER/<remote_path>): " << cache_dir;
    ASSERT_TRUE(std::filesystem::exists(std::filesystem::path{cache_dir} / "master.h5"))
        << "the downloaded HDF5 master file must exist in the cache dir";

    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                        READ_OP, &op));
    std::vector<int>    shape;
    std::vector<double> data;
    AL_EXPECT_OK(al_contract::read_data<double>(op, equilibrium_seed::kScalar, 0,
                                                &shape, &data));
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], 6.2)
        << "seeded data must read back correctly once handed off to the "
           "local backend";
    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));

    // --- second open: must reuse the cache, not re-download -----------------
    testing::internal::CaptureStdout();
    pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    captured = testing::internal::GetCapturedStdout();

    EXPECT_NE(captured.find("UDABackend cache directory already exists"),
              std::string::npos)
        << "a reopen must find the cache directory already present; "
           "captured: " << captured;
    EXPECT_NE(captured.find("UDABackend cached local file already exists"),
              std::string::npos)
        << "a reopen must not re-download master.h5 -- download_file's "
           "early-exists-return must fire; captured: " << captured;

    op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                        READ_OP, &op));
    shape.clear();
    data.clear();
    AL_EXPECT_OK(al_contract::read_data<double>(op, equilibrium_seed::kScalar, 0,
                                                &shape, &data));
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], 6.2) << "the reused cache must still serve correct data";
    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));

    std::filesystem::remove_all(cache_dir, ec);
}

// ===========================================================================
// Area 8: fetch mode -- the local_cache URI option overrides the default
// cache-directory naming (fetch_files(const uri::Uri&), uda_backend.cpp:414,
// 428). Distinct from area 7: this characterizes the override itself, not
// the default-naming/reuse behavior.
// ===========================================================================
TEST_F(UdaUniqueSurfaceTest, FetchModeLocalCacheOptionOverridesDefaultCacheDir) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string explicit_cache_root = base_.str() + "/explicit-cache";
    // local_cache replaces only the cache *root*: fetch_files still joins it
    // with remote_path_.relative_path() (uda_backend.cpp:428-429), so the
    // download still lands nested under the given root, not at its top level.
    const std::string expected_cache_dir =
        (std::filesystem::path{explicit_cache_root} /
         std::filesystem::path{pulse_dir}.relative_path())
            .string();

    const std::string uri = al_contract::uda_uri_base() +
                             "?fetch=1&backend=hdf5&path=" + pulse_dir +
                             "&local_cache=" + explicit_cache_root;

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));

    EXPECT_TRUE(std::filesystem::exists(
        std::filesystem::path{expected_cache_dir} / "master.h5"))
        << "an explicit local_cache overrides the cache *root* only -- the "
           "remote path is still nested underneath it, same as the default "
           "formula's own nesting (area 7): " << expected_cache_dir;
    EXPECT_FALSE(std::filesystem::exists(default_fetch_cache_dir(pulse_dir)))
        << "the default cache root must NOT be used when local_cache is given";

    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                        READ_OP, &op));
    std::vector<int>    shape;
    std::vector<double> data;
    AL_EXPECT_OK(al_contract::read_data<double>(op, equilibrium_seed::kScalar, 0,
                                                &shape, &data));
    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data[0], 6.2);
    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
}

// ===========================================================================
// Area 9: the stale-cache / write-divergence pin -- the trap an HLI
// implementer could hit. UDABackend::writeData delegates straight to the
// local backend
// once access_local_ is set (uda_backend.cpp:1107-1109): there is no upload
// path back to the server at all. Combined with download_file's
// early-exists-return (area 7/8), a fetch-mode write "succeeds" but only ever
// lands on the local cache copy; because that copy is never re-downloaded,
// the divergent value keeps being served on every later fetch-mode open,
// while the server-side pulse (read through ordinary remote mode) never
// changes. Pinned end-to-end, not fixed.
//
// Writes a real DD-4.1.1 leaf ("code/output_flag") that was never seeded, not
// the already-seeded scalar: HDF5Writer::write_ND_Data unconditionally calls
// HDF5DataSetHandler::create for any dataset not already tracked in this
// session's `opened_data_sets` map (hdf5_writer.cpp:417-423, no H5Lexists
// check against the file) -- so a WRITE_OP that overwrites data seeded in a
// *prior* session hits "Unable to create HDF5 dataset" (H5Dcreate2 fails on
// an already-linked name). That is a genuine, separate HDF5-backend
// Limit on updating pre-existing data in a fresh session, orthogonal to
// UDA fetch mode -- exercising it here would conflate two different defects.
// Writing a brand-new field sidesteps it while still proving the same
// divergence: a "successful" fetch-mode write that never reaches the server.
// ===========================================================================
TEST_F(UdaUniqueSurfaceTest, FetchModeWriteDivergesFromServerAndStalePersistsAcrossReopen) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string cache_root = base_.str() + "/write-divergence-cache";
    const char*       kNewField  = "code/output_flag";

    const std::string fetch_uri = al_contract::uda_uri_base() +
                                  "?fetch=1&backend=hdf5&path=" + pulse_dir +
                                  "&local_cache=" + cache_root;

    // --- write through fetch mode: lands only on the local cache copy ------
    {
        int pulse_ctx = -1;
        AL_ASSERT_OK(
            al_begin_dataentry_action(fetch_uri.c_str(), OPEN_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                            WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<int>(op, kNewField, {}, {42}))
            << "a write through fetch mode must \"succeed\" (it lands on the "
               "local cache copy) -- this is the trap, not a crash";
        AL_ASSERT_OK(al_end_action(op));
        EXPECT_EQ(al_close_pulse(pulse_ctx, CLOSE_PULSE).code, 0);
    }

    // --- confirm the server-side pulse is unchanged (reopen via remote mode) ---
    {
        const std::string remote_uri =
            al_contract::uda_hdf5_uri_for(pulse_dir);
        int                pulse_ctx = -1;
        AL_ASSERT_OK(
            al_begin_dataentry_action(remote_uri.c_str(), OPEN_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                            READ_OP, &op));
        std::vector<int> shape;
        std::vector<int> data;
        al_status_t      s = al_contract::read_data<int>(op, kNewField, 0, &shape,
                                                          &data);
        AL_EXPECT_OK(s) << "reading an absent field reports success (the "
                           "ordinary absent-leaf contract), not an error";
        ASSERT_EQ(data.size(), 1u);
        EXPECT_EQ(data[0], al_contract::kEmptyInt)
            << "the fetch-mode write must never reach the server -- there is "
               "no upload path (UDABackend::writeData delegates straight to "
               "the local backend once access_local_ is set)";
        AL_EXPECT_OK(al_end_action(op));
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }

    // --- the divergent local copy persists across close/reopen -------------
    {
        int pulse_ctx = -1;
        AL_ASSERT_OK(
            al_begin_dataentry_action(fetch_uri.c_str(), OPEN_PULSE, &pulse_ctx));
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                            READ_OP, &op));
        std::vector<int> shape;
        std::vector<int> data;
        AL_EXPECT_OK(al_contract::read_data<int>(op, kNewField, 0, &shape, &data));
        ASSERT_EQ(data.size(), 1u);
        EXPECT_EQ(data[0], 42)
            << "the divergent local copy must keep being served on later "
               "opens -- download_file's early-exists-return never re-fetches "
               "the genuine server-side (absent) value, so the divergence is "
               "permanent until the cache dir is manually cleared";
        AL_EXPECT_OK(al_end_action(op));
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }
}

// ===========================================================================
// Area 10: remote write and delete pinned unsupported. Remote mode is
// read-only by construction: the reference server plugin dispatches no
// writeData/deleteData handler at all -- issue #23's spike confirmed this by
// issuing the exact directives via uda_cli, both returning "[handle_request]:
// Unknown function requested!". But the two C-ABI entry points surface that
// dispatch failure very differently, a divergence only visible by driving the
// actual client library calls (not the spike's uda_cli probing of the raw
// directive):
//   - UDABackend::deleteData (uda_backend.cpp:1159-1186) issues
//     "IMAS::deleteData(...)" via uda::Client::get(), whose error-checking
//     path does surface the server's dispatch failure -- caught as a
//     uda::UDAException, re-thrown as an ALException, reaching al_delete_data
//     as a non-zero al_status_t carrying that exact text. This is the
//     intended, correctly-refused contract -- covered, not a defect.
//   - UDABackend::writeData (uda_backend.cpp:1104-1157) issues
//     "IMAS::writeData(...)" via uda::Client::put() instead, which never
//     throws on an in-band server dispatch failure (only on transport-level
//     faults) -- so writeData returns normally and al_write_data reports
//     al_status_t.code == 0. NEW DEFECT (xfail): the caller is told the write
//     succeeded, with no signal at all that nothing was persisted
//     server-side -- more dangerous than a clean refusal (confirmed by
//     reopening via ordinary remote mode afterward and finding the scalar
//     unchanged). The correct contract mirrors deleteData's: a remote write
//     must refuse with the same dispatch-failure text, not report false
//     success.
// ===========================================================================

// Shared by the correct-contract test and its tripwire below (mirrors
// UdaAosKnownDefects' pattern in test_uda_breadth.cpp): writes through remote
// mode and returns the write's own al_status_t plus whether the value
// genuinely reached the server (reopened via a fresh remote-mode session).
al_status_t remote_write_status_and_confirm_unpersisted(
    const std::string& pulse_dir, bool* reached_server) {
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);

    al_status_t write_status;
    {
        int pulse_ctx = -1;
        EXPECT_EQ(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx)
                      .code,
                  0);
        int op = -1;
        EXPECT_EQ(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                         WRITE_OP, &op)
                      .code,
                  0);
        write_status = al_contract::write_data<double>(
            op, equilibrium_seed::kScalar, {}, {9.9});
        AL_EXPECT_OK(al_end_action(op));
        AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
    }

    int pulse_ctx = -1;
    EXPECT_EQ(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx).code,
              0);
    int op = -1;
    EXPECT_EQ(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                     READ_OP, &op)
                  .code,
              0);
    std::vector<int>    shape;
    std::vector<double> data;
    EXPECT_EQ(al_contract::read_data<double>(op, equilibrium_seed::kScalar, 0,
                                             &shape, &data)
                  .code,
              0);
    *reached_server = (data.size() == 1u && data[0] == 9.9);
    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));

    return write_status;
}

// CORRECT-CONTRACT, expected-fail (DISABLED_): a remote write must be refused
// with the same dispatch-failure text deleteData surfaces, not silently
// report success.
TEST_F(UdaUniqueSurfaceTest, DISABLED_RemoteWriteFailsWithDispatchError) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    bool               reached_server = false;
    al_status_t        s = remote_write_status_and_confirm_unpersisted(
        pulse_dir, &reached_server);
    EXPECT_NE(s.code, 0) << "remote mode must refuse a write -- there is no "
                            "writeData handler on the reference server plugin";
    EXPECT_NE(std::string(s.message).find("Unknown function requested"),
              std::string::npos)
        << "expected the server's exact dispatch-failure text, got: "
        << s.message;
}

// CURRENT-BEHAVIOR tripwire: al_write_data reports success (code == 0) every
// time, even though the value never reaches the server.
TEST_F(UdaUniqueSurfaceTest, RemoteWriteCurrentlyReportsSuccessButNeverPersists) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    bool               reached_server = false;
    al_status_t        s = remote_write_status_and_confirm_unpersisted(
        pulse_dir, &reached_server);
    EXPECT_EQ(s.code, 0)
        << "a remote write now fails -- enable "
           "DISABLED_RemoteWriteFailsWithDispatchError (uda::Client::put()'s "
           "error path in UDABackend::writeData, src/uda/uda_backend.cpp, now "
           "surfaces the server's dispatch failure); got: " << s.message;
    EXPECT_FALSE(reached_server)
        << "the falsely-successful write must never actually reach the "
           "server -- remote mode is read-only by construction regardless of "
           "what al_write_data reported";
}

TEST_F(UdaUniqueSurfaceTest, RemoteDeleteFailsWithUnknownFunctionRequested) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string uri = al_contract::uda_hdf5_uri_for(pulse_dir);

    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pulse_ctx, equilibrium_seed::kIds, "",
                                        WRITE_OP, &op));

    al_status_t s = al_delete_data(op, equilibrium_seed::kScalar);
    EXPECT_NE(s.code, 0) << "remote mode must refuse a delete -- there is no "
                            "deleteData handler on the reference server plugin";
    EXPECT_NE(std::string(s.message).find("Unknown function requested"),
              std::string::npos)
        << "expected the server's exact dispatch-failure text, got: "
        << s.message;

    AL_EXPECT_OK(al_end_action(op));
    AL_EXPECT_OK(al_close_pulse(pulse_ctx, CLOSE_PULSE));
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_UDA
