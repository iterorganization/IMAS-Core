// UDA-unique surface (issue #26, TRACEABILITY.md Part 5): the first half of
// UDA's unique surface -- cache modes, runtime DD loading, datapath, and
// capability negotiation. The write-pin / fetch-mode half is the sibling
// issue. Six areas, each a dedicated block below:
//   1. URI option surface (plugin, init_args, dd_version, cache_mode invalid
//      value, verbose, host/port).
//   2. Cache-mode invisibility (none/ids/struct identical reads; cache
//      cleared on close).
//   3. Runtime DD loading (IDSDef.xml present / absent / wrong-version).
//   4. datapath partial-get via cache_mode=ids -- the only living use of
//      al_begin_global_action's datapath argument anywhere in the codebase
//      (CLAUDE.md's stale "no backend uses it" claim, corrected alongside
//      this test).
//   5. Version-drift check inertness (UDABackend::getVersion is a
//      placeholder on both sides of al_lowlevel.cpp's comparison).
//   6. Server-version-gated supportsTimeRangeOperation() (> 1.4.0).
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

std::string uda_uri_with(const std::string& pulse_dir, const std::string& extra_query = "") {
    std::string uri = al_contract::uda_uri_base() +
                       "?backend=hdf5&cache_mode=none&path=" + pulse_dir;
    if (!extra_query.empty()) uri += "&" + extra_query;
    return uri;
}

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

    // Seed the standard equilibrium precondition
    // (ids_properties/homogeneous_time) plus one real DD scalar
    // (vacuum_toroidal_field/r0) through the plain HDF5 backend. Returns the
    // pulse dir.
    std::string seed_scalar(const std::string& pulse_dir, double r0 = 6.2) const {
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
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
    const std::string uri = uda_uri_with(pulse_dir);
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
    if (s.code == 0) al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

// verbose: "1"/"true" -> debug tracing on stdout; anything else (including
// absent) -> off (UDABackend::process_options). Characterized by capturing
// stdout around the open call rather than asserting on internal state.
TEST_F(UdaUniqueSurfaceTest, VerboseTrueEmitsDebugTracingOnStdout) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = uda_uri_with(pulse_dir, "verbose=true");

    testing::internal::CaptureStdout();
    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    const std::string captured = testing::internal::GetCapturedStdout();
    al_close_pulse(pulse_ctx, CLOSE_PULSE);

    EXPECT_NE(captured.find("UDABackend openPulse"), std::string::npos)
        << "verbose=true must emit UDABackend's debug tracing; captured: "
        << captured;
}

TEST_F(UdaUniqueSurfaceTest, VerboseAbsentEmitsNoDebugTracing) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = uda_uri_with(pulse_dir);  // no verbose= at all

    testing::internal::CaptureStdout();
    int pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    const std::string captured = testing::internal::GetCapturedStdout();
    al_close_pulse(pulse_ctx, CLOSE_PULSE);

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
    const std::string uri = uda_uri_with(pulse_dir, "plugin=NoSuchImasPlugin");

    int         pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_NE(s.code, 0) << "an unregistered plugin name must fail at open, "
                            "not silently fall back to the default";
    if (s.code == 0) al_close_pulse(pulse_ctx, CLOSE_PULSE);
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
    const std::string uri = uda_uri_with(pulse_dir, "init_args=some_key=1;other_key=2");

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
    al_end_action(op);
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
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
    const std::string uri = uda_uri_with(pulse_dir, "dd_version=not.a.real.version");

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
    al_end_action(op);
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
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
    if (s.code == 0) al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

// ===========================================================================
// Area 2: cache-mode invisibility.
// ===========================================================================
// The three cache modes are a performance knob: none takes readData's direct
// per-field IMAS::get path; ids pre-fetches the whole (or datapath-scoped,
// area 4) IDS at beginAction time; struct pre-fetches each struct_array's
// leaves at its own begin_arraystruct_action, and *only* that (a plain
// top-level field is never in scope for struct mode -- see the second test
// below). For a field genuinely covered by all three (a static, non-dynamic
// leaf nested one level inside a struct_array, so struct mode's own populate
// point is reached), all three must return the exact same physics value.
TEST_F(UdaUniqueSurfaceTest, CacheModeNoneIdsStructAgreeForAosCoveredField) {
    const std::string pulse_dir = fresh_pulse_dir();
    const std::vector<int> kShape = {2, 3, 2};  // shape_for_rank(3)
    const std::vector<int> kWritten = {1, 2, 3, 4, 5, 6, 7, 8, 9,
                                       10, 11, 12};

    // --- seed temporary/constant_integer3d(1 element)/value via HDF5 -------
    {
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
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
        const std::string uri = al_contract::uda_uri_base() +
                                 "?backend=hdf5&cache_mode=" + std::string(mode) +
                                 "&path=" + pulse_dir;
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
        al_end_action(raos);
        al_end_action(op);
        al_close_pulse(pulse_ctx, CLOSE_PULSE);
    }
}

// Cache-mode invisibility does NOT extend to struct mode's own *scope*:
// UDABackend::beginAction only populates the cache for cache_mode=ids
// (src/uda/uda_backend.cpp:1016); struct mode populates lazily, per
// struct_array, only inside beginArraystructAction (cache_.count(path) miss ->
// populate_cache(ids, path, ...) scoped to that one struct_array's subtree,
// uda_backend.cpp:927-934). A plain top-level field that is never inside any
// struct_array the caller enters is therefore never populated under struct
// mode, and readData's fallthrough (`cache_.count(path)` miss, not one of the
// two homogeneous-time/version preconditions, cache_mode_ != None ->
// `return 0`, uda_backend.cpp:724-726) reports it as the ordinary absent-leaf
// contract -- silently wrong-looking unless you know the scope rule. This is
// the caching strategy working as designed, not a defect: struct mode is
// scoped to structs, not the whole IDS.
TEST_F(UdaUniqueSurfaceTest, CacheModeStructNeverPopulatesFieldsOutsideAnyArraystruct) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string uri = al_contract::uda_uri_base() +
                             "?backend=hdf5&cache_mode=struct&path=" + pulse_dir;

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
        << "cache_mode=struct never populates a field that lives outside "
           "every struct_array the caller enters, even though it was "
           "genuinely written -- struct mode's scope is per-struct_array, "
           "not IDS-wide like ids mode";
    al_end_action(op);
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
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

    const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
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
        const std::string uda_uri = al_contract::uda_uri_base() +
                                    "?backend=hdf5&cache_mode=" +
                                    std::string(cache_mode) + "&path=" + pulse_dir;
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
        al_end_action(op);
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
    const std::string uri = uda_uri_with(pulse_dir);
    int                pulse_ctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx));
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

// Absent: neither $IDSDEF_PATH nor $IMAS_PREFIX set -> load_xml() throws
// before any network access at all (the constructor loads the DD before it
// ever touches the wire), surfaced through the C ABI as a non-zero
// al_status_t carrying the exact message.
TEST_F(UdaUniqueSurfaceTest, DdAbsentNeitherEnvVarSetFailsWithClearMessage) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = uda_uri_with(pulse_dir);

    ScopedEnv idsdef(/*name=*/"IDSDEF_PATH", /*value=*/nullptr);
    ScopedEnv prefix(/*name=*/"IMAS_PREFIX", /*value=*/nullptr);

    int         pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_NE(s.code, 0) << "open must fail when neither DD env var is set";
    EXPECT_NE(std::string(s.message).find(
                  "neither IMAS_PREFIX or IDSDEF_PATH"),
              std::string::npos)
        << "expected the documented missing-env-var message, got: " << s.message;
    if (s.code == 0) al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

// Absent: $IDSDEF_PATH set but points at a file that does not exist ->
// load_xml()'s pugixml parse fails, distinct message from the unset-env case.
TEST_F(UdaUniqueSurfaceTest, DdAbsentFileMissingAtIdsDefPathFailsWithClearMessage) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = uda_uri_with(pulse_dir);

    ScopedEnv idsdef(/*name=*/"IDSDEF_PATH",
                    /*value=*/"/nonexistent/path/IDSDef.xml");

    int         pulse_ctx = -1;
    al_status_t s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    EXPECT_NE(s.code, 0) << "open must fail when IDSDEF_PATH names a missing file";
    EXPECT_NE(std::string(s.message).find("IDSDef.xml not found"), std::string::npos)
        << "expected the documented not-found message, got: " << s.message;
    if (s.code == 0) al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

// Wrong-version: $IDSDEF_PATH points at a real, structurally valid, but
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
TEST_F(UdaUniqueSurfaceTest, DdWrongVersionLoadsSilentlyWithNoCrossVersionCheck) {
    const char* older_dd = std::getenv("IDSDEF_PATH_OLDER");
    if (!older_dd || !*older_dd) {
        GTEST_SKIP() << "IDSDEF_PATH_OLDER is unset -- the reference image "
                        "does not carry a second DD version (docker/uda/"
                        "Dockerfile); this row is skipped, not failed.";
    }

    const std::string pulse_dir = seed_scalar(fresh_pulse_dir(), /*r0=*/6.2);
    const std::string uri       = uda_uri_with(pulse_dir);

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
        << "no signal distinguishes this from a correct-version read -- the "
           "version drift is silent";
    al_end_action(op);
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
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
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
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
    const std::string uda_uri = al_contract::uda_uri_base() +
                                "?backend=hdf5&cache_mode=ids&path=" + pulse_dir;
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

    al_end_action(op);
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

// ===========================================================================
// Area 5: version-drift check inertness.
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
// never reads anything from the actual remote pulse). The comparison
// ((ver.first!=sver.first)||(ver.second<sver.second)) is therefore always
// (0!=0)||(0<0) = false -- the version-drift check can never fire for UDA,
// regardless of what is genuinely stored remotely. Directly testable only as
// a positive: open must always succeed on this axis (there is no way to force
// a "drifted" placeholder to differ from itself).
// ===========================================================================
TEST_F(UdaUniqueSurfaceTest, VersionDriftCheckNeverFiresRegardlessOfStoredPulse) {
    const std::string pulse_dir = seed_scalar(fresh_pulse_dir());
    const std::string uri = uda_uri_with(pulse_dir);
    int                pulse_ctx = -1;
    al_status_t        s = al_begin_dataentry_action(uri.c_str(), OPEN_PULSE, &pulse_ctx);
    AL_EXPECT_OK(s) << "OPEN_PULSE must never fail on the version-drift "
                       "comparison for UDA -- both sides of the comparison "
                       "are hardcoded {0,0} placeholders "
                       "(UDABackend::getVersion), so it is structurally inert";
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

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
        const std::string hdf5_uri = "imas:hdf5?path=" + pulse_dir;
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

    const std::string uri = uda_uri_with(pulse_dir);
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
        al_end_action(op);
    }
    al_close_pulse(pulse_ctx, CLOSE_PULSE);
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_UDA
