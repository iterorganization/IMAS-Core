// HDF5 absolute field-path reads (issue #65).
//
// al_read_data's documented contract (include/al_lowlevel.h, read-data block)
// is that a fieldpath is relative to the passed context, "dataobject absolute
// path can be specified with a prepended '/'". An absolute fieldpath is NOT a
// filesystem path and does NOT carry the IDS name: "/time_slice/.../field"
// addresses the field from the dataobject root.
//
// HDF5Reader::read_ND_Data used to honor only the relative half: it replaced
// every '/' with '&' (the tensorized dataset-name separator) and then
// unconditionally prepended the current array-structure context's tensorized
// path, so an absolute spelling produced a malformed name --
//   time_slice[]&constraints&j_phi[]&&time_slice&constraints&j_phi&reconstructed
// for a dataset actually stored as
//   time_slice[]&constraints&j_phi[]&reconstructed
// H5Lexists could not find it, the reader returned 0, and al_plugin_read_data
// filled the caller's buffer with EMPTY_DOUBLE while reporting code 0. Stored
// data was therefore indistinguishable from genuinely absent data -- a silent
// wrong answer, the failure mode this suite exists to catch.
//
// Every test below drives only the public C ABI (decision D1). The fix is a
// pure path/selection change in the HDF5 read path: no DD-version knowledge,
// no schema lookup, no ABI change -- so these tests read the same values back
// through two *spellings* of one address and assert they agree.
//
// Scope, per the issue: HDF5 *reads*. Writes, al_delete_data and
// al_begin_arraystruct_action are deliberately exercised only in their
// relative form here; whether they honor an absolute path is a separate
// question and must be filed as its own finding, not assumed by these tests.
//
// Fixture shape -- every path is a real equilibrium DD-4.1.1 path (the
// equilibrium_seed.h convention, issue #33), validated with the imas-dd tool
// suite; the core still treats them as opaque strings:
//
//   vacuum_toroidal_field/r0                     FLT_0D   root scalar
//   ids_properties/version_put/data_dictionary   STR_0D   root DD stamp
//   time                                         FLT_1D   homogeneous timebase
//   time_slice                                   AOS      timebasepath "time"
//     time                                       FLT_0D
//     global_quantities/ip                       FLT_0D   ancestor-level leaf
//     profiles_1d/psi                            FLT_1D   ancestor-level array
//     constraints/j_phi                          AOS      nested, multi-segment
//       reconstructed                            FLT_0D   the issue's own leaf
//       measured                                 FLT_0D
//       source                                   STR_0D   array leaf (CHAR 1D)
//
// Both AOS levels hold more than one element and every value is distinct per
// (slice, constraint) pair, so a resolution bug that silently reads element
// zero -- the easy way to "fix" this wrongly -- cannot pass.

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using al_contract::PulseId;

namespace {

constexpr const char* kIds = "equilibrium";

// --- addresses, in both spellings -------------------------------------------
constexpr const char* kR0            = "vacuum_toroidal_field/r0";
constexpr const char* kR0Abs         = "/vacuum_toroidal_field/r0";
constexpr const char* kDdStamp       = "ids_properties/version_put/data_dictionary";
constexpr const char* kDdStampAbs    = "/ids_properties/version_put/data_dictionary";
constexpr const char* kTimebase      = "time";
constexpr const char* kTimeSlice     = "time_slice";
constexpr const char* kJPhi          = "constraints/j_phi";  // multi-segment AOS
constexpr const char* kIp            = "global_quantities/ip";
constexpr const char* kIpAbs         = "/time_slice/global_quantities/ip";
constexpr const char* kPsi           = "profiles_1d/psi";
constexpr const char* kPsiAbs        = "/time_slice/profiles_1d/psi";
constexpr const char* kReconstructed = "reconstructed";
constexpr const char* kReconstructedAbs =
    "/time_slice/constraints/j_phi/reconstructed";
constexpr const char* kMeasuredAbs = "/time_slice/constraints/j_phi/measured";
constexpr const char* kSource      = "source";
constexpr const char* kSourceAbs   = "/time_slice/constraints/j_phi/source";
// Never written by the seed: the "genuinely absent" control.
constexpr const char* kWeightAbs = "/time_slice/constraints/j_phi/weight";

constexpr int kNSlices   = 3;  // > 1 so a nonzero outer cursor is reachable
constexpr int kNJPhi     = 3;  // > 1 so a nonzero inner cursor is reachable
constexpr int kRadialLen = 4;

// --- deterministic content, distinct per (slice, constraint) ----------------
double r0_value() { return 6.2; }
double slice_time(int i) { return 1.0 + 0.5 * i; }
double ip(int i) { return 100.0 + 7.0 * i; }
double reconstructed(int i, int j) { return 47.0 + 10.0 * i + j; }
double measured(int i, int j) { return 200.0 + 10.0 * i + j; }

std::string source(int i, int j) {
    return "src-" + std::to_string(i) + "-" + std::to_string(j);
}
std::string dd_stamp() { return "4.1.1"; }

std::vector<double> timebase() {
    std::vector<double> t;
    for (int i = 0; i < kNSlices; ++i) t.push_back(slice_time(i));
    return t;
}
std::vector<double> psi(int i) {
    std::vector<double> v;
    for (int r = 0; r < kRadialLen; ++r) v.push_back(10.0 * i + r + 0.25);
    return v;
}

// ---------------------------------------------------------------------------
// Fixture: one freshly seeded on-disk HDF5 pulse per test.
// ---------------------------------------------------------------------------
class Hdf5AbsoluteFieldPaths : public ::testing::Test {
protected:
    void SetUp() override {
        base_.make_legacy_tree(pulse_);
        const std::string uri =
            al_contract::build_uri(HDF5_BACKEND, base_.str(), pulse_);
        ASSERT_FALSE(uri.empty());
        AL_ASSERT_OK(al_begin_dataentry_action(uri.c_str(), FORCE_CREATE_PULSE,
                                               &pctx_));
        ASSERT_NO_FATAL_FAILURE(write_seed());
    }

    void TearDown() override {
        if (pctx_ != -1) al_close_pulse(pctx_, CLOSE_PULSE);
    }

    // Writes the fixture shape above. Relative spellings only -- the write
    // side's absolute handling is out of this issue's scope.
    void write_seed() {
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pctx_, kIds, "", WRITE_OP, &op));
        AL_ASSERT_OK(al_contract::write_data<double>(op, kR0, {}, {r0_value()}));
        AL_ASSERT_OK(al_contract::write_char_array(op, kDdStamp, dd_stamp()));
        AL_ASSERT_OK(
            al_contract::write_data<double>(op, kTimebase, {kNSlices}, timebase()));

        int slices = kNSlices;
        int ts     = -1;
        AL_ASSERT_OK(
            al_begin_arraystruct_action(op, kTimeSlice, kTimebase, &slices, &ts));
        for (int i = 0; i < kNSlices; ++i) {
            AL_ASSERT_OK(
                al_contract::write_data<double>(ts, "time", {}, {slice_time(i)}));
            AL_ASSERT_OK(al_contract::write_data<double>(ts, kIp, {}, {ip(i)}));
            AL_ASSERT_OK(
                al_contract::write_data<double>(ts, kPsi, {kRadialLen}, psi(i)));

            int constraints = kNJPhi;
            int jp          = -1;
            AL_ASSERT_OK(
                al_begin_arraystruct_action(ts, kJPhi, "", &constraints, &jp));
            for (int j = 0; j < kNJPhi; ++j) {
                AL_ASSERT_OK(al_contract::write_data<double>(
                    jp, kReconstructed, {}, {reconstructed(i, j)}));
                AL_ASSERT_OK(al_contract::write_data<double>(jp, "measured", {},
                                                             {measured(i, j)}));
                AL_ASSERT_OK(al_contract::write_char_array(jp, kSource,
                                                           source(i, j)));
                if (j + 1 < kNJPhi)
                    AL_ASSERT_OK(al_iterate_over_arraystruct(jp, 1));
            }
            AL_ASSERT_OK(al_end_action(jp));
            if (i + 1 < kNSlices) AL_ASSERT_OK(al_iterate_over_arraystruct(ts, 1));
        }
        AL_ASSERT_OK(al_end_action(ts));
        AL_ASSERT_OK(al_end_action(op));
    }

    // Opens a GLOBAL read, walks the outer AOS to element `slice` and the
    // nested AOS to element `constraint`, then hands the body the three live
    // contexts (operation, time_slice, constraints/j_phi).
    //
    // The cursor is advanced element by element, exactly as an HLI drives it;
    // that is what makes "the current cursor" a real, nonzero position rather
    // than a freshly opened context that happens to sit at zero.
    template <class Body>
    void at(int slice, int constraint, Body body) {
        int op = -1;
        AL_ASSERT_OK(al_begin_global_action(pctx_, kIds, "", READ_OP, &op));

        int slices = 0;
        int ts     = -1;
        AL_ASSERT_OK(
            al_begin_arraystruct_action(op, kTimeSlice, kTimebase, &slices, &ts));
        ASSERT_EQ(slices, kNSlices);
        for (int i = 0; i < slice; ++i)
            AL_ASSERT_OK(al_iterate_over_arraystruct(ts, 1));

        int constraints = 0;
        int jp          = -1;
        AL_ASSERT_OK(al_begin_arraystruct_action(ts, kJPhi, "", &constraints, &jp));
        ASSERT_EQ(constraints, kNJPhi);
        for (int j = 0; j < constraint; ++j)
            AL_ASSERT_OK(al_iterate_over_arraystruct(jp, 1));

        body(op, ts, jp);

        al_end_action(jp);
        al_end_action(ts);
        al_end_action(op);
    }

    // Reads one DOUBLE scalar; returns the status and the value the core left
    // in the caller's buffer (EMPTY_DOUBLE when the field is reported absent).
    static al_status_t read_scalar(int ctx, const char* field, double* out) {
        *out = al_contract::kEmptyDouble;
        int   size[MAXDIM] = {0};
        void* buf          = out;
        return al_read_data(ctx, field, "", &buf, DOUBLE_DATA, 0, size);
    }

    // The same read through the plugin entry point. al_read_data delegates to
    // al_plugin_read_data whenever no plugin is bound to the field
    // (al_lowlevel.cpp), so the two normally share this code path -- but the
    // issue's independent reproducer reached the core through this symbol
    // directly, and a caller that interposes the plugin layer sees only this
    // one. Asserting both keeps the pair from silently diverging.
    static al_status_t plugin_read_scalar(int ctx, const char* field, double* out) {
        *out = al_contract::kEmptyDouble;
        int   size[MAXDIM] = {0};
        void* buf          = out;
        return al_plugin_read_data(ctx, field, "", &buf, DOUBLE_DATA, 0, size);
    }

    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/65, /*run=*/0};
    int     pctx_ = -1;
};

// ===========================================================================
// The issue's headline case: a scalar leaf beneath a nested AOS, addressed
// absolutely from inside that same AOS.
// ===========================================================================
TEST_F(Hdf5AbsoluteFieldPaths, ScalarUnderNestedAosAgreesWithRelative) {
    // Nonzero on both AOS levels: an implementation that ignores the cursor
    // and reads element (0,0) reads 47.0 and would pass at (0,0) alone.
    constexpr int kSlice = 1, kConstraint = 2;
    at(kSlice, kConstraint, [&](int, int, int jp) {
        double relative = 0.0, absolute = 0.0;
        AL_EXPECT_OK(read_scalar(jp, kReconstructed, &relative));
        AL_EXPECT_OK(read_scalar(jp, kReconstructedAbs, &absolute));

        EXPECT_DOUBLE_EQ(relative, reconstructed(kSlice, kConstraint))
            << "the relative spelling is the control -- if this fails the "
               "fixture, not the absolute-path resolution, is wrong";
        EXPECT_DOUBLE_EQ(absolute, reconstructed(kSlice, kConstraint))
            << "an absolute fieldpath must address the same stored datum as "
               "the relative one (al_lowlevel.h read-data contract); "
               "EMPTY_DOUBLE here is the issue-#65 silent-absence bug";
    });
}

// The same assertion through al_plugin_read_data, the entry point the issue's
// independent baseline resolved with dlsym.
TEST_F(Hdf5AbsoluteFieldPaths, ScalarUnderNestedAosAgreesThroughPluginEntryPoint) {
    constexpr int kSlice = 1, kConstraint = 2;
    at(kSlice, kConstraint, [&](int, int, int jp) {
        double relative = 0.0, absolute = 0.0;
        AL_EXPECT_OK(plugin_read_scalar(jp, kReconstructed, &relative));
        AL_EXPECT_OK(plugin_read_scalar(jp, kReconstructedAbs, &absolute));
        EXPECT_DOUBLE_EQ(relative, reconstructed(kSlice, kConstraint));
        EXPECT_DOUBLE_EQ(absolute, reconstructed(kSlice, kConstraint))
            << "al_plugin_read_data must resolve an absolute fieldpath exactly "
               "as al_read_data does";
    });
}

// Every (slice, constraint) pair, so the resolved AOS selection is shown to
// track the live cursor on both levels rather than matching at one position.
TEST_F(Hdf5AbsoluteFieldPaths, AbsoluteReadTracksBothCursors) {
    for (int i = 0; i < kNSlices; ++i) {
        for (int j = 0; j < kNJPhi; ++j) {
            at(i, j, [&](int, int, int jp) {
                double absolute = 0.0, absolute_measured = 0.0;
                AL_EXPECT_OK(read_scalar(jp, kReconstructedAbs, &absolute));
                AL_EXPECT_OK(read_scalar(jp, kMeasuredAbs, &absolute_measured));
                EXPECT_DOUBLE_EQ(absolute, reconstructed(i, j))
                    << "slice=" << i << " constraint=" << j;
                EXPECT_DOUBLE_EQ(absolute_measured, measured(i, j))
                    << "slice=" << i << " constraint=" << j;
            });
        }
    }
}

// ===========================================================================
// Array leaf beneath the nested AOS: rank and extent must survive resolution,
// not just the first element.
// ===========================================================================
TEST_F(Hdf5AbsoluteFieldPaths, ArrayLeafUnderNestedAosAgreesWithRelative) {
    constexpr int kSlice = 2, kConstraint = 1;
    at(kSlice, kConstraint, [&](int, int, int jp) {
        std::string relative, absolute;
        AL_EXPECT_OK(al_contract::read_char_array(jp, kSource, &relative));
        AL_EXPECT_OK(al_contract::read_char_array(jp, kSourceAbs, &absolute));

        EXPECT_EQ(relative, source(kSlice, kConstraint));
        EXPECT_EQ(absolute, source(kSlice, kConstraint))
            << "a CHAR array leaf must resolve identically through both "
               "spellings, extent included";
    });
}

// ===========================================================================
// Ancestor targets: the retained selection must be the ancestor's, not the
// child's. Applying the full child cursor to an ancestor-level dataset is the
// other way to get this wrong.
// ===========================================================================
TEST_F(Hdf5AbsoluteFieldPaths, AncestorScalarKeepsOnlyTheAncestorCursor) {
    constexpr int kSlice = 2, kConstraint = 1;
    at(kSlice, kConstraint, [&](int, int ts, int jp) {
        double from_parent = 0.0, from_child = 0.0;
        // Same datum, reached two ways: relatively from the time_slice
        // context, and absolutely from one level deeper.
        AL_EXPECT_OK(read_scalar(ts, kIp, &from_parent));
        AL_EXPECT_OK(read_scalar(jp, kIpAbs, &from_child));

        EXPECT_DOUBLE_EQ(from_parent, ip(kSlice));
        EXPECT_DOUBLE_EQ(from_child, ip(kSlice))
            << "an ancestor-level field addressed from a child context must "
               "keep the time_slice index and drop the constraints/j_phi one";
    });
}

TEST_F(Hdf5AbsoluteFieldPaths, AncestorArrayKeepsOnlyTheAncestorCursor) {
    constexpr int kSlice = 1, kConstraint = 2;
    at(kSlice, kConstraint, [&](int, int ts, int jp) {
        std::vector<int>    parent_shape, child_shape;
        std::vector<double> parent_data, child_data;
        AL_EXPECT_OK(
            al_contract::read_data<double>(ts, kPsi, 1, &parent_shape, &parent_data));
        AL_EXPECT_OK(al_contract::read_data<double>(jp, kPsiAbs, 1, &child_shape,
                                                    &child_data));

        ASSERT_EQ(parent_shape, std::vector<int>{kRadialLen});
        EXPECT_EQ(parent_data, psi(kSlice));
        EXPECT_EQ(child_shape, std::vector<int>{kRadialLen})
            << "the absolute read must report the ancestor array's real extent";
        EXPECT_EQ(child_data, psi(kSlice));
    });
}

// ===========================================================================
// Root targets: no AOS index applies at all. Reached from the deepest context
// (both cursors live) and from the plain operation context.
// ===========================================================================
TEST_F(Hdf5AbsoluteFieldPaths, RootScalarReadableFromNestedContext) {
    constexpr int kSlice = 2, kConstraint = 2;
    at(kSlice, kConstraint, [&](int op, int ts, int jp) {
        double from_op = 0.0, from_slice = 0.0, from_constraint = 0.0;
        AL_EXPECT_OK(read_scalar(op, kR0, &from_op));
        AL_EXPECT_OK(read_scalar(ts, kR0Abs, &from_slice));
        AL_EXPECT_OK(read_scalar(jp, kR0Abs, &from_constraint));

        EXPECT_DOUBLE_EQ(from_op, r0_value());
        EXPECT_DOUBLE_EQ(from_slice, r0_value())
            << "a root field must stay readable from inside an AOS";
        EXPECT_DOUBLE_EQ(from_constraint, r0_value())
            << "... and from inside a nested AOS, with neither index applied";
    });
}

// The DD stamp is an ordinary root string field (the core attaches no meaning
// to it) -- and it is exactly what a version-agnostic reader asks for first,
// from whatever context it happens to hold.
TEST_F(Hdf5AbsoluteFieldPaths, RootDdStampReadableFromNestedContext) {
    at(1, 1, [&](int op, int, int jp) {
        std::string from_op, from_constraint;
        AL_EXPECT_OK(al_contract::read_char_array(op, kDdStamp, &from_op));
        AL_EXPECT_OK(al_contract::read_char_array(jp, kDdStampAbs, &from_constraint));
        EXPECT_EQ(from_op, dd_stamp());
        EXPECT_EQ(from_constraint, dd_stamp());
    });
}

// An absolute spelling used from the operation context itself: no AOS is open,
// so the leading '/' is the only thing to strip. This is the same defect with
// an empty context chain, and it must not regress the relative form.
TEST_F(Hdf5AbsoluteFieldPaths, AbsoluteAndRelativeAgreeAtRootContext) {
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx_, kIds, "", READ_OP, &op));

    double relative = 0.0, absolute = 0.0;
    AL_EXPECT_OK(read_scalar(op, kR0, &relative));
    AL_EXPECT_OK(read_scalar(op, kR0Abs, &absolute));
    EXPECT_DOUBLE_EQ(relative, r0_value());
    EXPECT_DOUBLE_EQ(absolute, r0_value())
        << "at the operation context an absolute fieldpath differs from the "
           "relative one only by its leading '/'";

    std::vector<int>    shape;
    std::vector<double> data;
    AL_EXPECT_OK(
        al_contract::read_data<double>(op, "/time", 1, &shape, &data));
    EXPECT_EQ(shape, std::vector<int>{kNSlices});
    EXPECT_EQ(data, timebase()) << "the root timebase array too";

    al_end_action(op);
}

// ===========================================================================
// Ordinary missing-data semantics must be untouched: the fix makes stored data
// findable, it must not make absent data look present.
// ===========================================================================
TEST_F(Hdf5AbsoluteFieldPaths, GenuinelyAbsentFieldsStayAbsent) {
    constexpr int kSlice = 1, kConstraint = 1;
    at(kSlice, kConstraint, [&](int op, int, int jp) {
        // Never written, addressed absolutely...
        double absent_abs = 0.0;
        AL_EXPECT_OK(read_scalar(jp, kWeightAbs, &absent_abs));
        EXPECT_DOUBLE_EQ(absent_abs, al_contract::kEmptyDouble)
            << "an unwritten field must still report the absent sentinel";

        // ... and relatively, the control.
        double absent_rel = 0.0;
        AL_EXPECT_OK(read_scalar(jp, "weight", &absent_rel));
        EXPECT_DOUBLE_EQ(absent_rel, al_contract::kEmptyDouble);

        // A root-level path that does not exist at all.
        double absent_root = 0.0;
        AL_EXPECT_OK(read_scalar(jp, "/no_such_structure/no_such_field",
                                 &absent_root));
        EXPECT_DOUBLE_EQ(absent_root, al_contract::kEmptyDouble);

        // An absolute path whose leading segment matches the open AOS but
        // whose remainder does not exist: this must resolve and come back
        // absent, not accidentally alias a sibling that does exist.
        double absent_sibling = 0.0;
        AL_EXPECT_OK(read_scalar(jp, "/time_slice/global_quantities/no_such_leaf",
                                 &absent_sibling));
        EXPECT_DOUBLE_EQ(absent_sibling, al_contract::kEmptyDouble);

        double still_there = 0.0;
        AL_EXPECT_OK(read_scalar(op, kR0, &still_there));
        EXPECT_DOUBLE_EQ(still_there, r0_value())
            << "the absent lookups must not poison neighbouring reads";
    });
}

// ===========================================================================
// The decided limit of the rule, pinned so it reads as a decision rather than
// an accident (docs/adr/0002-absolute-field-path-resolution.md).
// ===========================================================================
// An absolute path is resolved against the AOS chain the caller has actually
// opened. A target that crosses an AOS the caller never entered has no cursor
// to be read at, and the core holds no schema that would even tell it that
// segment IS an array of structures -- so the name resolves without that
// level's "[]", finds nothing, and is reported absent. The alternative would be
// to invent an index, which is how a wrong number gets returned instead of no
// number at all.
TEST_F(Hdf5AbsoluteFieldPaths, TargetBeyondAnUnopenedAosIsReportedAbsent) {
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx_, kIds, "", READ_OP, &op));
    int slices = 0, ts = -1;
    AL_ASSERT_OK(
        al_begin_arraystruct_action(op, kTimeSlice, kTimebase, &slices, &ts));
    ASSERT_EQ(slices, kNSlices);

    // constraints/j_phi is never opened here, so this names no single datum.
    double beyond = 0.0;
    AL_EXPECT_OK(read_scalar(ts, kReconstructedAbs, &beyond));
    EXPECT_DOUBLE_EQ(beyond, al_contract::kEmptyDouble)
        << "with no cursor for constraints/j_phi the read must report absent, "
           "not silently pick an element";

    // The ancestor level that *is* open still resolves, at its own cursor.
    double ancestor = 0.0;
    AL_EXPECT_OK(read_scalar(ts, kIpAbs, &ancestor));
    EXPECT_DOUBLE_EQ(ancestor, ip(0));

    al_end_action(ts);
    al_end_action(op);
}

// ===========================================================================
// Dataset-name caching: the reader memoizes both "this dataset exists" and the
// open dataset handle under the resolved name. Repeating the reads in mixed
// order, and re-reading after the cursor moves, proves the cache is keyed on
// the *resolved* name and not on the spelling the caller used.
// ===========================================================================
TEST_F(Hdf5AbsoluteFieldPaths, RepeatedMixedSpellingsStayConsistent) {
    constexpr int kSlice = 2, kConstraint = 0;
    at(kSlice, kConstraint, [&](int, int, int jp) {
        const double expected = reconstructed(kSlice, kConstraint);
        for (int repeat = 0; repeat < 3; ++repeat) {
            double absolute = 0.0, relative = 0.0;
            AL_EXPECT_OK(read_scalar(jp, kReconstructedAbs, &absolute));
            AL_EXPECT_OK(read_scalar(jp, kReconstructed, &relative));
            EXPECT_DOUBLE_EQ(absolute, expected) << "repeat " << repeat;
            EXPECT_DOUBLE_EQ(relative, expected) << "repeat " << repeat;
        }

        // An absent absolute lookup is cached too; it must not turn a later
        // present one into a miss.
        double absent = 0.0;
        AL_EXPECT_OK(read_scalar(jp, kWeightAbs, &absent));
        EXPECT_DOUBLE_EQ(absent, al_contract::kEmptyDouble);

        double after = 0.0;
        AL_EXPECT_OK(read_scalar(jp, kReconstructedAbs, &after));
        EXPECT_DOUBLE_EQ(after, expected);
    });
}

// Within one open AOS context, advancing the cursor must move what an absolute
// path resolves to -- the cached dataset handle is shared across iterations,
// so this is where a name/selection mix-up would surface.
TEST_F(Hdf5AbsoluteFieldPaths, AbsoluteReadFollowsIterationWithinOneContext) {
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx_, kIds, "", READ_OP, &op));
    int slices = 0, ts = -1;
    AL_ASSERT_OK(
        al_begin_arraystruct_action(op, kTimeSlice, kTimebase, &slices, &ts));
    ASSERT_EQ(slices, kNSlices);

    for (int i = 0; i < kNSlices; ++i) {
        int constraints = 0, jp = -1;
        AL_ASSERT_OK(al_begin_arraystruct_action(ts, kJPhi, "", &constraints, &jp));
        ASSERT_EQ(constraints, kNJPhi);
        for (int j = 0; j < kNJPhi; ++j) {
            double absolute = 0.0, ancestor = 0.0;
            AL_EXPECT_OK(read_scalar(jp, kReconstructedAbs, &absolute));
            AL_EXPECT_OK(read_scalar(jp, kIpAbs, &ancestor));
            EXPECT_DOUBLE_EQ(absolute, reconstructed(i, j))
                << "slice=" << i << " constraint=" << j;
            EXPECT_DOUBLE_EQ(ancestor, ip(i))
                << "slice=" << i << " constraint=" << j;
            if (j + 1 < kNJPhi) AL_EXPECT_OK(al_iterate_over_arraystruct(jp, 1));
        }
        al_end_action(jp);
        if (i + 1 < kNSlices) AL_EXPECT_OK(al_iterate_over_arraystruct(ts, 1));
    }
    al_end_action(ts);
    al_end_action(op);
}

}  // namespace
