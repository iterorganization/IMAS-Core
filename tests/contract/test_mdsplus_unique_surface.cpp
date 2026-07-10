// MDSplus-unique surface: struct-aware slice interpolation, timerange
// resampling, segment-backed dynamic writes, timebase cache (issue #17,
// TRACEABILITY.md Part 4's last "gap (deferred)" row).
//
// Every other MDSplus test file joins MDSplus to a fixture that also runs on
// HDF5/Memory/ASCII/Flexbuffers (parity) or curates real DD paths for the
// synthetic-path shapes those fixtures can't transfer (issue #15's breadth
// matrix). This file is different: it exercises behavior that has **no
// HDF5-tier analogue at all**, because it depends on MDSplus's real
// segment-based time-series storage model, not just its real-DD-path model
// tree.
//
// --- struct-aware slice interpolation -----------------------------------
// HDF5's slice mode (Hdf5TimeDependent.SliceInterpolationModes,
// test_capability_gated.cpp) only interpolates a single scalar leaf.
// MDSplus's slice mode can also interpolate a whole array-of-structures
// *element* at once: al_begin_arraystruct_action, called on a SLICE_OP
// operation context whose target path is a real DD-conformant dynamic AOS,
// reaches MDSplusBackend::readSliceApd (src/mdsplus/mdsplus_backend.cpp:3359)
// instead of the plain-leaf MDSplusBackend::readSlice. For LINEAR_INTERP with
// the requested time strictly between two stored elements, readSliceApd
// calls MDSplusBackend::interpolateStruct (mdsplus_backend.cpp:3675), which
// walks the whole struct recursively and interpolates every numeric leaf it
// finds -- confirmed empirically below by writing two leaves inside the same
// AOS element ("time" and "global_quantities/ip") and observing BOTH come
// back correctly interpolated from a single al_begin_arraystruct_action call.
//
// Real path used: equilibrium/time_slice (struct_array,
// timebasepath="time" -- i.e. each element carries its own "time" leaf,
// matching DD 4.1.1's own coordinate1 "time_slice(itime)/time"; confirmed by
// walking build-mdsplus/_deps/data-dictionary-src/IDSDef.xml, the same
// artifact issue #15 curated real paths against). This is the same path
// TRACEABILITY.md's AosMatrix/EquilibriumSeedMatrix divergence notes and
// test_mdsplus_real_paths.cpp's file header already name as the proof that
// real DD-conformant AOS paths round-trip on MDSplus.
//
// --- timerange resampling -------------------------------------------------
// Empirically, there is no unique surface here: supportsTimeRangeOperation()
// is unconditionally false for MDSplus (mdsplus_backend.cpp:5444-5446), so
// al_lowlevel.cpp's capability gate (al_lowlevel.cpp:1053) refuses
// al_begin_timerange_action with LOWLEVEL_ERR before the backend is ever
// reached -- already pinned as parity by
// CapabilityMatrix.TimeRangeReadPositiveOrRefused/Mdsplus
// (test_capability_gated.cpp). MDSplus's real time-dependent reads go
// through slice mode instead (struct-aware interpolation above), never
// through time-range resampling. The test below re-asserts the refusal
// directly against this file's own real DD path, so this file is a
// self-contained record of the finding rather than a cross-file assumption.
//
// --- segment-backed dynamic writes -----------------------------------------
// MDSplusBackend::beginWriteArraystruct dispatches to
// MDSplusBackend::writeDynamicApd (mdsplus_backend.cpp:2688) with its `append`
// parameter set to true specifically when the enclosing operation is a
// SLICE_OP against a real timebase-carrying AOS (mdsplus_backend.cpp:5257,
// 5264,5317,5324) -- as opposed to the whole-container rewrite a GLOBAL_OP
// write does (mdsplus_backend.cpp:5243,5245,5292,5295, the form the
// struct-aware-interpolation/timebase-cache tests above use to seed their
// fixtures). This is the ABI-observable manifestation of "segment-backed
// dynamic write": al_begin_slice_action(WRITE_OP, UNDEFINED_TIME) +
// al_begin_arraystruct_action, called once per new element, appends that
// element's serialized MDSplus::Apd as one more segment on the AOS node (the
// same per-segment storage readDynamicApd/getApdSliceAt already walk on read,
// mdsplus_backend.cpp:3158,3293) -- confirmed below by appending kN elements
// one WRITE session at a time and reading every one of them back
// independently, in contrast to HDF5's scalar slice-append defect
// (Hdf5SliceAppend.OnlyLastAppendedSlicePersists_CurrentBehavior,
// test_capability_gated.cpp) where only the last write survives. This is
// correct MDSplus behavior, not a defect -- no xfail pairing needed.
//
// A different, narrower shape -- a bare top-level scalar leaf slice-appended
// against a *separate* external timebase field via al_write_data's `timebase`
// parameter (mirroring Hdf5SliceAppend's own idiom verbatim) -- was tried
// first and empirically found to throw "%TREE-E-NOSEGMENTS" on MDSplus the
// very first time either field is written with no pre-existing segment in
// the other; that narrower shape was dropped in favor of the AOS-append form
// above, which is the one this codebase's own write dispatch already treats
// as the deliberate, first-class "append" path (the `append=true` call sites
// just cited). Whether the scalar-leaf shape is a distinct, fixable defect or
// an unsupported combination is left unresolved -- out of scope for this
// characterization pass, which pins the crash-class defect it did surface
// (MdsplusSliceWriteKnownDefects* below) and otherwise sticks to the
// mechanism the backend actually supports.
//
// --- timebase cache --------------------------------------------------------
// MDSplusBackend keeps a per-node cache of computed segment/timebase-index
// mappings (segmentIdxMap, mdsplus_backend.h:283), populated on first use by
// getSegmentIdxFromSliceIdx (mdsplus_backend.cpp:3221) and consulted again by
// every subsequent getApdSliceAt call -- e.g. the two lookups a single
// LINEAR_INTERP struct-aware slice read needs (sliceIdx and sliceIdx1) reuse
// the one cached vector rather than recomputing it twice. Grepping every
// touch site shows it is invalidated unconditionally (a whole-map clear, not
// scoped to the written node) at the *start* of every write dispatch
// (MDSplusBackend::writeData, mdsplus_backend.cpp:4577;
// MDSplusBackend::beginWriteArraystruct, mdsplus_backend.cpp:4634) and again
// at the *end* of an AOS write (mdsplus_backend.cpp:5342) and at
// MDSplusBackend::deleteData (mdsplus_backend.cpp:2141) -- so no write path
// through the C ABI can leave a stale segment map for a later read to
// consult. The test below is this finding's characterization: it warms the
// cache with a slice-interpolated read, forces an intervening write (both an
// unrelated GLOBAL scalar write, and a SLICE_OP append that grows the very
// AOS whose segment map was cached by one more element -- see the
// segment-backed-writes section above for why append, not a second GLOBAL
// rewrite, is the correct way to grow it), and confirms a repeated slice read
// afterwards is neither corrupted by the invalidation nor stale to the new
// layout the intervening write introduced.
#ifdef AL_CONTRACT_HAVE_MDSPLUS

#include "al_contract.h"
#include "equilibrium_seed.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using al_contract::PulseId;

namespace {

// --- real DD-4.1.1 paths (equilibrium/time_slice, see file header) --------
constexpr const char* kIds        = equilibrium_seed::kIds;  // "equilibrium"
constexpr const char* kAos        = "time_slice";             // struct_array
constexpr const char* kAosTime    = "time";                   // timebasepath
constexpr const char* kIp         = "global_quantities/ip";   // FLT_0D leaf

// ===========================================================================
// Struct-aware slice interpolation: a whole AOS element, two leaves at once.
// ===========================================================================
class MdsplusStructAwareSliceInterpolation : public ::testing::Test {
protected:
    void SetUp() override {
        AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED();
        base_.make_legacy_tree(pulse_);
        const std::string uri =
            al_contract::build_uri(MDSPLUS_BACKEND, base_.str(), pulse_);
        ASSERT_FALSE(uri.empty());
        AL_ASSERT_OK(
            al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx_));
        write_two_time_slices();
    }
    void TearDown() override {
        if (pctx_ >= 0) al_close_pulse(pctx_, CLOSE_PULSE);
    }

    // time_slice[0] = {time=1.0, ip=10.0}; time_slice[1] = {time=2.0, ip=20.0}.
    void write_two_time_slices() {
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pctx_, kIds, "", WRITE_OP, &op));
        int size = 2;
        int aos  = -1;
        AL_ASSERT_OK(al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kAosTime, {}, {kT0}));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kIp, {}, {kV0}));
        AL_EXPECT_OK(al_iterate_over_arraystruct(aos, 1));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kAosTime, {}, {kT1}));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kIp, {}, {kV1}));
        AL_ASSERT_OK(al_end_action(aos));
        AL_ASSERT_OK(al_end_action(op));
    }

    // Slice-reads time_slice at `time`/`interp`; returns the single
    // interpolated element's ("time", "ip") pair via out params. Fails the
    // test fatally (not via a return code) if the struct-aware slice itself
    // is refused, so a downstream value mismatch can never be blamed on a
    // broken setup.
    void slice_read_struct(double time, int interp, double* out_time,
                           double* out_ip) {
        int op = -1;
        ASSERT_EQ(al_begin_slice_action(pctx_, kIds, READ_OP, time, interp, &op)
                      .code,
                  0);
        int size = 0;
        int aos  = -1;
        ASSERT_EQ(
            al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos).code, 0)
            << "struct-aware slice read must be accepted";
        ASSERT_EQ(size, 1)
            << "a slice read of an AOS must collapse to exactly one element";

        std::vector<int> shape;
        std::vector<double> time_val, ip_val;
        EXPECT_EQ(al_contract::read_data<double>(aos, kAosTime, 0, &shape,
                                                 &time_val)
                      .code,
                  0);
        EXPECT_EQ(
            al_contract::read_data<double>(aos, kIp, 0, &shape, &ip_val).code,
            0);
        ASSERT_EQ(time_val.size(), 1u);
        ASSERT_EQ(ip_val.size(), 1u);
        *out_time = time_val[0];
        *out_ip   = ip_val[0];

        EXPECT_EQ(al_end_action(aos).code, 0);
        EXPECT_EQ(al_end_action(op).code, 0);
    }

    static constexpr double kT0 = 1.0, kT1 = 2.0;
    static constexpr double kV0 = 10.0, kV1 = 20.0;

    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/17,
                   /*run=*/0};
    int     pctx_ = -1;
};

TEST_F(MdsplusStructAwareSliceInterpolation, ClosestPicksNearerElement) {
    double t = -1.0, ip = -1.0;
    slice_read_struct(1.4, CLOSEST_INTERP, &t, &ip);
    EXPECT_EQ(t, kT0);
    EXPECT_EQ(ip, kV0);

    slice_read_struct(1.6, CLOSEST_INTERP, &t, &ip);
    EXPECT_EQ(t, kT1);
    EXPECT_EQ(ip, kV1);
}

TEST_F(MdsplusStructAwareSliceInterpolation, PreviousPicksLastElementAtOrBelow) {
    double t = -1.0, ip = -1.0;
    slice_read_struct(1.9, PREVIOUS_INTERP, &t, &ip);
    EXPECT_EQ(t, kT0);
    EXPECT_EQ(ip, kV0);
}

// The struct-aware case: LINEAR_INTERP at the midpoint interpolates BOTH
// leaves of the element together ("time" and "global_quantities/ip"), from
// one al_begin_arraystruct_action call -- this is the behavior with no HDF5
// analogue (HDF5's slice mode only ever interpolates one scalar leaf at a
// time, never a whole struct).
TEST_F(MdsplusStructAwareSliceInterpolation, LinearInterpolatesWholeStruct) {
    double t = -1.0, ip = -1.0;
    slice_read_struct(1.5, LINEAR_INTERP, &t, &ip);
    EXPECT_EQ(t, (kT0 + kT1) / 2.0) << "the element's own timebase leaf must "
                                       "interpolate, not just the signal";
    EXPECT_EQ(ip, (kV0 + kV1) / 2.0);
}

// ===========================================================================
// Timerange resampling: no unique MDSplus surface -- refused unconditionally.
// ===========================================================================
TEST(MdsplusTimerangeResampling, RefusedWithNoUniqueSurface) {
    AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED();
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 17, 0};
    base.make_legacy_tree(pulse);
    const std::string uri =
        al_contract::build_uri(MDSPLUS_BACKEND, base.str(), pulse);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx));

    int    op = -1;
    double dtime = 0.0;
    int    dtime_shape = 0;
    al_status_t s = al_begin_timerange_action(pctx, kIds, READ_OP, 0.0, 10.0,
                                              &dtime, &dtime_shape,
                                              UNDEFINED_INTERP, &op);
    EXPECT_NE(s.code, 0)
        << "supportsTimeRangeOperation() is unconditionally false for "
           "MDSplus (mdsplus_backend.cpp:5444-5446) -- there is no MDSplus "
           "timerange-resampling surface to characterize; time-dependent "
           "reads go through slice mode instead, see "
           "MdsplusStructAwareSliceInterpolation above";
    EXPECT_EQ(s.code, LOWLEVEL_ERR)
        << "documented refusal is LOWLEVEL_ERR (al_lowlevel.cpp:1053), same "
           "as every other non-HDF5 always-on backend";

    al_close_pulse(pctx, CLOSE_PULSE);
}

// ===========================================================================
// Segment-backed dynamic writes: incremental AOS appends all persist.
// ===========================================================================
// MDSplusBackend::beginWriteArraystruct dispatches to
// MDSplusBackend::writeDynamicApd (mdsplus_backend.cpp:2688) with its `append`
// parameter set to `true` specifically when the enclosing operation context
// is a SLICE_OP whose target is a real timebase-carrying AOS
// (mdsplus_backend.cpp:5257,5264,5317,5324 -- contrasted with the `false`/
// whole-rewrite form the GLOBAL_OP writes at mdsplus_backend.cpp:5243,5245,
// 5292,5295 use, and the one this file's struct-aware-interpolation /
// timebase-cache tests above use). This is the ABI-observable manifestation
// of "segment-backed dynamic write": al_begin_slice_action(WRITE_OP,
// UNDEFINED_TIME) + al_begin_arraystruct_action, called once per new element,
// appends that element's serialized MDSplus::Apd as one more segment onto the
// AOS node (the same per-segment storage readDynamicApd/getApdSliceAt walk on
// read, mdsplus_backend.cpp:3158,3293) -- rather than the whole-container
// rewrite a GLOBAL write does. Confirmed below by appending kN elements one
// session at a time and reading every one of them back independently, in
// contrast to HDF5's scalar slice-append defect
// (Hdf5SliceAppend.OnlyLastAppendedSlicePersists_CurrentBehavior,
// test_capability_gated.cpp) where only the last write survives.
class MdsplusSegmentBackedWrites : public ::testing::Test {
protected:
    static constexpr int kN = 5;

    void SetUp() override { AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED(); }

    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/17,
                   /*run=*/0};
};

TEST_F(MdsplusSegmentBackedWrites, AppendedElementsAllPersist) {
    base_.make_legacy_tree(pulse_);
    const std::string uri =
        al_contract::build_uri(MDSPLUS_BACKEND, base_.str(), pulse_);
    ASSERT_FALSE(uri.empty());
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx));

    double t[kN], v[kN];
    for (int k = 0; k < kN; ++k) {
        t[k] = 1.0 + k;
        v[k] = 10.0 * (k + 1);
    }

    // Element 0 establishes the AOS's very first segment via an ordinary
    // GLOBAL write -- empirically, writeDynamicApd's append=true path (taken
    // for a SLICE_OP write) throws "%TREE-E-NOSEGMENTS" the first time it
    // must create a segment from nothing, the same first-segment fragility
    // as MdsplusSliceWriteKnownDefects below hits for a bare scalar leaf.
    // Once the AOS has its first segment, further SLICE_OP appends succeed.
    {
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op));
        int size = 1;
        int aos  = -1;
        AL_ASSERT_OK(al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kAosTime, {}, {t[0]}));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kIp, {}, {v[0]}));
        AL_ASSERT_OK(al_end_action(aos));
        AL_ASSERT_OK(al_end_action(op));
    }

    // Elements 1..kN-1 are appended one at a time -- WRITE_OP + UNDEFINED_TIME,
    // one al_begin_slice_action/al_begin_arraystruct_action(size=1)/al_end_action
    // set per element, never writing more than one element per session.
    for (int k = 1; k < kN; ++k) {
        int op = -1;
        AL_ASSERT_OK(al_begin_slice_action(pctx, kIds, WRITE_OP, UNDEFINED_TIME,
                                           UNDEFINED_INTERP, &op));
        int size = 1;
        int aos  = -1;
        AL_ASSERT_OK(al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kAosTime, {}, {t[k]}));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kIp, {}, {v[k]}));
        AL_ASSERT_OK(al_end_action(aos));
        AL_ASSERT_OK(al_end_action(op));
    }

    // Every appended element must be independently retrievable by its own
    // time -- unlike HDF5's scalar slice-append defect
    // (Hdf5SliceAppend.OnlyLastAppendedSlicePersists_CurrentBehavior), MDSplus's
    // segment-backed AOS storage keeps all kN, not just the last.
    for (int k = 0; k < kN; ++k) {
        int op = -1;
        AL_ASSERT_OK(al_begin_slice_action(pctx, kIds, READ_OP, t[k],
                                           CLOSEST_INTERP, &op));
        int size = 0;
        int aos  = -1;
        AL_ASSERT_OK(al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos));
        ASSERT_EQ(size, 1);
        std::vector<int> shape;
        std::vector<double> value;
        AL_EXPECT_OK(al_contract::read_data<double>(aos, kIp, 0, &shape, &value));
        ASSERT_EQ(value.size(), 1u);
        EXPECT_EQ(value[0], v[k]) << "element " << k << " (t=" << t[k]
                                  << ") did not persist its own value";
        AL_EXPECT_OK(al_end_action(aos));
        AL_EXPECT_OK(al_end_action(op));
    }

    al_close_pulse(pctx, CLOSE_PULSE);
}

// Backs the claim in AppendedElementsAllPersist's setup comment with its own
// assertion (D2: don't freeze an empirical finding as an unpinned comment) --
// a divergence, not a defect: appending the very first element of a
// brand-new AOS via SLICE_OP has no existing segment to extend, and MDSplus
// refuses gracefully (a caught, returned error, not a crash) rather than
// silently doing something undefined.
TEST_F(MdsplusSegmentBackedWrites, FirstElementCannotBeAppendedViaSliceAction) {
    base_.make_legacy_tree(pulse_);
    const std::string uri =
        al_contract::build_uri(MDSPLUS_BACKEND, base_.str(), pulse_);
    ASSERT_FALSE(uri.empty());
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx));

    int op = -1;
    AL_ASSERT_OK(al_begin_slice_action(pctx, kIds, WRITE_OP, UNDEFINED_TIME,
                                       UNDEFINED_INTERP, &op));
    int size = 1;
    int aos  = -1;
    AL_ASSERT_OK(al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos));
    AL_EXPECT_OK(al_contract::write_data<double>(aos, kAosTime, {}, {1.0}));
    AL_EXPECT_OK(al_contract::write_data<double>(aos, kIp, {}, {10.0}));
    al_status_t s = al_end_action(aos);
    EXPECT_NE(s.code, 0)
        << "expected MDSplus to refuse a SLICE_OP append against a "
           "brand-new, still-empty AOS (no prior segment to extend)";
    al_end_action(op);
    al_close_pulse(pctx, CLOSE_PULSE);
}

// ---------------------------------------------------------------------------
// KNOWN DEFECT (decision D2), discovered while characterizing segment-backed
// writes above: MDSplusBackend::writeData's SLICE_OP branch
// (mdsplus_backend.cpp:4586-4591) dereferences `size[0]` unconditionally --
//     case alconst::slice_op:
//       if(size[0] > 1) writeTimedData(...); else writeSlice(...);
// -- but the C ABI passes `size == nullptr` for any dim=0 (scalar) write
// (al_lowlevel.h; al_contract::write_int_scalar documents this convention).
// Writing ANY scalar field inside a SLICE_OP write action therefore
// segfaults MDSplus, unconditionally -- confirmed empirically the first time
// this file tried the exact idiom Hdf5SliceAppend (test_capability_gated.cpp)
// uses to seed homogeneous_time inside its slice-write loop, which is fine on
// HDF5 but crashes on MDSplus. Not scoped to homogeneous_time specifically:
// any dim=0 field hits the same unconditional dereference.
// ---------------------------------------------------------------------------
al_status_t write_scalar_within_slice_action() {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 17, 0};
    base.make_legacy_tree(pulse);
    const std::string uri =
        al_contract::build_uri(MDSPLUS_BACKEND, base.str(), pulse);
    int pctx = -1;
    EXPECT_EQ(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx)
                  .code,
              0);

    int op = -1;
    EXPECT_EQ(al_begin_slice_action(pctx, "magnetics", WRITE_OP, UNDEFINED_TIME,
                                    UNDEFINED_INTERP, &op)
                  .code,
              0);
    int ht = 1;
    al_status_t s = al_write_data(op, "ids_properties/homogeneous_time", "",
                                  &ht, INTEGER_DATA, 0, nullptr);
    al_end_action(op);
    al_close_pulse(pctx, CLOSE_PULSE);
    return s;
}

TEST(MdsplusSliceWriteKnownDefects, DISABLED_ScalarWriteWithinSliceActionSucceeds) {
    AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED();
    EXPECT_EQ(write_scalar_within_slice_action().code, 0)
        << "a dim=0 (scalar) al_write_data inside a SLICE_OP action must not "
           "crash MDSplus";
}

TEST(MdsplusSliceWriteKnownDefectsDeath,
    ScalarWriteWithinSliceActionCurrentlyCrashes) {
    AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED();
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ASSERT_DEATH(write_scalar_within_slice_action(), ".*")
        << "expected a scalar write inside a SLICE_OP action to crash "
           "MDSplus (mdsplus_backend.cpp:4586-4591 dereferences size[0] "
           "unconditionally, but the C ABI passes size=nullptr for dim=0). "
           "If it no longer crashes, enable "
           "MdsplusSliceWriteKnownDefects.DISABLED_ScalarWriteWithinSliceActionSucceeds";
}

// ===========================================================================
// Timebase cache: correctness across an invalidating write, not staleness.
// ===========================================================================
TEST(MdsplusTimebaseCache, SurvivesInterveningWritesWithoutStaleness) {
    AL_CONTRACT_SKIP_IF_MDSPLUS_UNCONFIGURED();
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 17, 0};
    base.make_legacy_tree(pulse);
    const std::string uri =
        al_contract::build_uri(MDSPLUS_BACKEND, base.str(), pulse);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE, &pctx));

    // --- seed: time_slice[0..1] = {(1.0, 10.0), (2.0, 20.0)} ----------------
    {
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op));
        int size = 2;
        int aos  = -1;
        AL_ASSERT_OK(al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kAosTime, {}, {1.0}));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kIp, {}, {10.0}));
        AL_EXPECT_OK(al_iterate_over_arraystruct(aos, 1));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kAosTime, {}, {2.0}));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kIp, {}, {20.0}));
        AL_ASSERT_OK(al_end_action(aos));
        AL_ASSERT_OK(al_end_action(op));
    }

    auto interp_ip_at = [&](double time, int interp) {
        int op = -1;
        EXPECT_EQ(
            al_begin_slice_action(pctx, kIds, READ_OP, time, interp, &op).code,
            0);
        int size = 0;
        int aos  = -1;
        EXPECT_EQ(
            al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos).code, 0);
        std::vector<int> shape;
        std::vector<double> ip_val;
        EXPECT_EQ(
            al_contract::read_data<double>(aos, kIp, 0, &shape, &ip_val).code,
            0);
        EXPECT_EQ(al_end_action(aos).code, 0);
        EXPECT_EQ(al_end_action(op).code, 0);
        return ip_val.empty() ? -1.0 : ip_val[0];
    };
    auto linear_interp_ip_at = [&](double time) {
        return interp_ip_at(time, LINEAR_INTERP);
    };
    auto closest_interp_ip_at = [&](double time) {
        return interp_ip_at(time, CLOSEST_INTERP);
    };

    // --- warm the timebase/segment cache for the time_slice node ------------
    ASSERT_EQ(linear_interp_ip_at(1.5), 15.0);

    // --- an unrelated GLOBAL write elsewhere in the same pulse clears the
    // whole-map cache unconditionally (MDSplusBackend::writeData,
    // mdsplus_backend.cpp:4577-4578) ------------------------------------------
    {
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op));
        AL_EXPECT_OK(al_contract::write_data<double>(
            op, equilibrium_seed::kScalar, {}, {6.2}));
        AL_ASSERT_OK(al_end_action(op));
    }

    // --- re-read the SAME interpolated point: must reproduce the same
    // value, not a corrupted one, after the cache was invalidated and must
    // rebuild from scratch --------------------------------------------------
    EXPECT_EQ(linear_interp_ip_at(1.5), 15.0)
        << "an unrelated write must not corrupt a subsequent struct-aware "
           "slice interpolation on an already-cached AOS node";

    // --- append a THIRD element via SLICE_OP (the genuinely-additive path
    // MdsplusSegmentBackedWrites above confirms -- a second GLOBAL write of a
    // fresh N-element AOS does NOT replace what is already stored: writeDynamicApd
    // computes its next slice index by walking the existing segments
    // regardless of the `append` flag (mdsplus_backend.cpp:2746-2776), so it
    // would instead produce a non-monotonic five-element timebase
    // {1,2,1,2,3} and corrupt the interpolation oracle below for reasons
    // unrelated to the cache). The append also clears the cache, via
    // MDSplusBackend::beginWriteArraystruct (mdsplus_backend.cpp:4634) and
    // again at the end of the AOS write (mdsplus_backend.cpp:5342) ----------
    {
        int op = -1;
        AL_ASSERT_OK(al_begin_slice_action(pctx, kIds, WRITE_OP, UNDEFINED_TIME,
                                           UNDEFINED_INTERP, &op));
        int size = 1;
        int aos  = -1;
        AL_ASSERT_OK(al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kAosTime, {}, {3.0}));
        AL_EXPECT_OK(al_contract::write_data<double>(aos, kIp, {}, {30.0}));
        AL_ASSERT_OK(al_end_action(aos));
        AL_ASSERT_OK(al_end_action(op));
    }

    // --- the freshly-rebuilt cache must reflect the NEW segment layout, not
    // a stale two-element view -----------------------------------------------
    EXPECT_EQ(closest_interp_ip_at(3.0), 30.0)
        << "a slice read after appending a new element must see it -- the "
           "cache must not still reflect the pre-append segment layout";
    EXPECT_EQ(linear_interp_ip_at(1.5), 15.0)
        << "the original interpolated point must be unaffected by the append";

    al_close_pulse(pctx, CLOSE_PULSE);
}

}  // namespace

#endif  // AL_CONTRACT_HAVE_MDSPLUS
