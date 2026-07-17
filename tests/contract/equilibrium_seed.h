// Deterministic single-version equilibrium-seed generator (issues #4, #33).
//
// TEST_STRATEGY.md decision D5: "one realistic single-version equilibrium
// seed — a curated subset (scalar, a profiles_1d array with a timebase, a
// constraints AOS) from a deterministic in-repo generator (generator + hash,
// not committed blobs) — to exercise real AOS/timebase/HDF5-tensorization
// shapes." This header IS the generator: no binary fixture is checked in, the
// shape is produced by code, and the round-trip oracle is a *structural* hash
// (issue #33).
//
// DD-4.1.1-conformant shape (issue #33 — the earlier version modelled
// profiles_1d/psi as a flat top-level leaf and constraints/{measured,weight}
// as a generic top-level AOS; neither is a valid equilibrium shape, so the two
// DD-coupled backends could only skip the seed). Every path below is a real
// equilibrium DD-4.1.1 path, validated against the baked-from artifact
// build-mdsplus/_deps/data-dictionary-src/IDSDef.xml (the method
// real_dd_path_catalog.h documents; the imas-dd MCP tool CLAUDE.md names is not
// in this session's MCP config). It stays opaque strings through the C ABI
// (decision D1: the core attaches no DD semantics to any of these paths):
//   - a static scalar          -> vacuum_toroidal_field/r0 (FLT_0D)
//   - a top-level timebase array-> time (FLT_1D, the IDS homogeneous timebase)
//   - a real dynamic AOS        -> time_slice (struct_array, timebasepath
//     "time"), multiple elements, each carrying:
//       * time                       (FLT_0D)          -- per-slice coordinate
//       * profiles_1d/psi            (FLT_1D)          -- radial profile
//       * global_quantities/ip       (FLT_0D)
//       * constraints/ip/measured    (FLT_0D)          -- a real constraints
//       * constraints/ip/weight      (FLT_0D)             subtree, not generic
//
// Structural hash (issue #33): the oracle is a hash over a canonical stream
// that includes, per field, its path identity, datatype, rank, every extent,
// and (for AOS elements) the array size and element index, then the values.
// Hashing values alone let a 3x4->4x3 extent transpose with unchanged bytes,
// or a dropped/reordered AOS element, pass silently; the structural stream
// closes that. The read side rebuilds the same stream from what it *actually*
// observed (real ranks/extents/AOS sizes), so any shape drift shows up as a
// hash (and explicit rank/extent) mismatch instead of being reconciled away.
#ifndef AL_CONTRACT_EQUILIBRIUM_SEED_H
#define AL_CONTRACT_EQUILIBRIUM_SEED_H

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <cstdint>
#include <string>
#include <vector>

namespace equilibrium_seed {

constexpr const char* kIds      = "equilibrium";
// static scalar + top-level timebase array
constexpr const char* kScalar   = "vacuum_toroidal_field/r0";  // FLT_0D
constexpr const char* kTimebase = "time";                      // FLT_1D
// the real dynamic AOS and its per-element leaves
constexpr const char* kAos       = "time_slice";               // struct_array
constexpr const char* kAosTime   = "time";                     // timebasepath
constexpr const char* kSliceTime = "time";                     // FLT_0D leaf
constexpr const char* kPsi       = "profiles_1d/psi";          // FLT_1D
constexpr const char* kIp        = "global_quantities/ip";     // FLT_0D
constexpr const char* kMeasured  = "constraints/ip/measured";  // FLT_0D
constexpr const char* kWeight    = "constraints/ip/weight";    // FLT_0D

constexpr int kNSlices   = 2;  // time_slice AOS elements (>1: real AOS)
constexpr int kRadialLen = 4;  // profiles_1d/psi grid points per slice

// A "not a real datatype" marker used only for the synthetic AOS-size record in
// the canonical stream (so array size participates in the hash).
constexpr int kAosSizeMarker = -1;

// --- deterministic content -------------------------------------------------
// Distinct arithmetic progressions per role and per slice, so a transposed,
// dropped, or reordered element is caught by the structural hash. Kept clear of
// the write-path "absent" sentinels documented in al_contract.h (kEmptyDouble).
inline double scalar_r0() { return 6.2; }

inline std::vector<double> timebase_values() {
    std::vector<double> t;
    for (int i = 0; i < kNSlices; ++i) t.push_back(1.0 + 0.5 * i);
    return t;
}

inline double slice_time(int i) { return 1.0 + 0.5 * i; }
inline double slice_ip(int i) { return 100.0 + 7.0 * i; }
inline double slice_measured(int i) { return 200.0 + 3.0 * i; }
inline double slice_weight(int i) { return 0.5 + 0.1 * i; }

// Per-slice radial profile: distinct per (slice, radial index).
inline std::vector<double> slice_psi(int i) {
    std::vector<double> v;
    v.reserve(kRadialLen);
    for (int r = 0; r < kRadialLen; ++r) v.push_back(10.0 * i + r + 0.25);
    return v;
}

// --- a single observed/expected field record -------------------------------
struct Obs {
    std::string         field;     // canonical identity, e.g.
                                   // "time_slice[1]/profiles_1d/psi"
    int                 datatype;  // DOUBLE_DATA, or kAosSizeMarker
    int                 rank;
    std::vector<int>    extents;   // actual dims (empty for rank 0)
    std::vector<double> values;

    bool same_structure(const Obs& o) const {
        return field == o.field && datatype == o.datatype && rank == o.rank &&
               extents == o.extents;
    }
    bool operator==(const Obs& o) const {
        return same_structure(o) && values == o.values;
    }
};

inline std::string slice_field(int i, const char* leaf) {
    return std::string(kAos) + "[" + std::to_string(i) + "]/" + leaf;
}

// The expected canonical stream, built straight from the generator functions
// above (this is the "generator", not a committed golden value: change any
// generator function and the expected records — and the expected hash —
// recompute automatically).
inline std::vector<Obs> expected_records() {
    std::vector<Obs> r;
    r.push_back({kScalar, DOUBLE_DATA, 0, {}, {scalar_r0()}});
    r.push_back({kTimebase, DOUBLE_DATA, 1, {kNSlices}, timebase_values()});
    r.push_back({std::string(kAos) + ".size", kAosSizeMarker, 0, {},
                 {static_cast<double>(kNSlices)}});
    for (int i = 0; i < kNSlices; ++i) {
        r.push_back({slice_field(i, kSliceTime), DOUBLE_DATA, 0, {},
                     {slice_time(i)}});
        r.push_back({slice_field(i, kPsi), DOUBLE_DATA, 1, {kRadialLen},
                     slice_psi(i)});
        r.push_back({slice_field(i, kIp), DOUBLE_DATA, 0, {}, {slice_ip(i)}});
        r.push_back({slice_field(i, kMeasured), DOUBLE_DATA, 0, {},
                     {slice_measured(i)}});
        r.push_back({slice_field(i, kWeight), DOUBLE_DATA, 0, {},
                     {slice_weight(i)}});
    }
    return r;
}

// --- FNV-1a over the canonical structural stream ----------------------------
inline uint64_t fnv1a(uint64_t h, const void* data, size_t n) {
    const unsigned char* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}
inline uint64_t fnv1a(uint64_t h, int v) { return fnv1a(h, &v, sizeof(v)); }
inline uint64_t fnv1a(uint64_t h, double v) { return fnv1a(h, &v, sizeof(v)); }

constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;

// Canonical hash: every structural attribute participates, with lengths
// interleaved so no field's bytes can slide into the next (path "ab"+"c" must
// not collide with "a"+"bc"; extents {3,4} must not collide with {4,3}).
inline uint64_t canonical_hash(const std::vector<Obs>& records) {
    uint64_t h = kFnvOffsetBasis;
    h = fnv1a(h, static_cast<int>(records.size()));
    for (const Obs& o : records) {
        h = fnv1a(h, static_cast<int>(o.field.size()));
        h = fnv1a(h, o.field.data(), o.field.size());
        h = fnv1a(h, o.datatype);
        h = fnv1a(h, o.rank);
        h = fnv1a(h, static_cast<int>(o.extents.size()));
        for (int e : o.extents) h = fnv1a(h, e);
        h = fnv1a(h, static_cast<int>(o.values.size()));
        for (double v : o.values) h = fnv1a(h, v);
    }
    return h;
}

inline uint64_t expected_hash() { return canonical_hash(expected_records()); }

// --- write the seed into an already-open pulse (its own GLOBAL write) ------
inline al_status_t write(int pulse_ctx) {
    int         op = -1;
    al_status_t s = al_begin_global_action(pulse_ctx, kIds, "", WRITE_OP, &op);
    if (s.code != 0) return s;

    s = al_contract::write_data<double>(op, kScalar, {}, {scalar_r0()});
    if (s.code != 0) { al_end_action(op); return s; }

    s = al_contract::write_data<double>(op, kTimebase, {kNSlices},
                                        timebase_values());
    if (s.code != 0) { al_end_action(op); return s; }

    int size    = kNSlices;
    int aos_ctx = -1;
    s = al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos_ctx);
    if (s.code != 0) { al_end_action(op); return s; }
    for (int i = 0; i < kNSlices; ++i) {
        s = al_contract::write_data<double>(aos_ctx, kSliceTime, {},
                                            {slice_time(i)});
        if (s.code != 0) break;
        s = al_contract::write_data<double>(aos_ctx, kPsi, {kRadialLen},
                                            slice_psi(i));
        if (s.code != 0) break;
        s = al_contract::write_data<double>(aos_ctx, kIp, {}, {slice_ip(i)});
        if (s.code != 0) break;
        s = al_contract::write_data<double>(aos_ctx, kMeasured, {},
                                            {slice_measured(i)});
        if (s.code != 0) break;
        s = al_contract::write_data<double>(aos_ctx, kWeight, {},
                                            {slice_weight(i)});
        if (s.code != 0) break;
        if (i + 1 < kNSlices) {
            s = al_iterate_over_arraystruct(aos_ctx, 1);
            if (s.code != 0) break;
        }
    }
    al_status_t end_aos = al_end_action(aos_ctx);
    if (s.code == 0) s = end_aos;

    al_status_t end_op = al_end_action(op);
    if (s.code == 0) s = end_op;
    return s;
}

// --- read the seed back, rebuilding the observed canonical stream ----------
// Records exactly what the backend returned (path identity, datatype, real
// rank/extents, AOS size/index, values). No assertions here -- the caller
// compares the observed records against expected_records() structurally and by
// hash, so shape drift on read surfaces as a mismatch, not a silent
// reconciliation. Returns the first non-OK status encountered (with whatever
// records were gathered up to that point), or OK.
inline al_status_t read_back(int pulse_ctx, std::vector<Obs>* out) {
    out->clear();
    int         op = -1;
    al_status_t s = al_begin_global_action(pulse_ctx, kIds, "", READ_OP, &op);
    if (s.code != 0) return s;

    auto read_leaf = [&](const char* path, const std::string& field, int rank,
                         Obs* obs) -> al_status_t {
        std::vector<int>    shape;
        std::vector<double> data;
        al_status_t st =
            al_contract::read_data<double>(op, path, rank, &shape, &data);
        if (st.code != 0) return st;
        *obs = Obs{field, DOUBLE_DATA, rank, shape, data};
        return st;
    };

    Obs obs;
    s = read_leaf(kScalar, kScalar, 0, &obs);
    if (s.code != 0) { al_end_action(op); return s; }
    out->push_back(obs);

    s = read_leaf(kTimebase, kTimebase, 1, &obs);
    if (s.code != 0) { al_end_action(op); return s; }
    out->push_back(obs);

    int size    = 0;
    int aos_ctx = -1;
    s = al_begin_arraystruct_action(op, kAos, kAosTime, &size, &aos_ctx);
    if (s.code != 0) { al_end_action(op); return s; }
    out->push_back(Obs{std::string(kAos) + ".size", kAosSizeMarker, 0, {},
                       {static_cast<double>(size)}});

    for (int i = 0; i < size; ++i) {
        // path is per-element ("time"); the recorded field carries the index.
        auto read_slice = [&](const char* path, const char* leaf,
                              int rank) -> al_status_t {
            std::vector<int>    shape;
            std::vector<double> data;
            al_status_t st =
                al_contract::read_data<double>(aos_ctx, path, rank, &shape,
                                               &data);
            if (st.code != 0) return st;
            out->push_back(Obs{slice_field(i, leaf), DOUBLE_DATA, rank, shape,
                               data});
            return st;
        };
        s = read_slice(kSliceTime, kSliceTime, 0);
        if (s.code != 0) break;
        s = read_slice(kPsi, kPsi, 1);
        if (s.code != 0) break;
        s = read_slice(kIp, kIp, 0);
        if (s.code != 0) break;
        s = read_slice(kMeasured, kMeasured, 0);
        if (s.code != 0) break;
        s = read_slice(kWeight, kWeight, 0);
        if (s.code != 0) break;
        if (i + 1 < size) {
            s = al_iterate_over_arraystruct(aos_ctx, 1);
            if (s.code != 0) break;
        }
    }
    al_status_t end_aos = al_end_action(aos_ctx);
    if (s.code == 0) s = end_aos;

    al_status_t end_op = al_end_action(op);
    if (s.code == 0) s = end_op;
    return s;
}

// Convenience for the always-on round-trip sites: read back and hash. The
// structural asserts live in the test bodies (they need gtest EXPECT_EQ);
// this just yields the observed hash.
inline al_status_t read_and_hash(int pulse_ctx, uint64_t* out_hash) {
    std::vector<Obs> obs;
    al_status_t      s = read_back(pulse_ctx, &obs);
    if (s.code == 0) *out_hash = canonical_hash(obs);
    return s;
}

}  // namespace equilibrium_seed

#endif  // AL_CONTRACT_EQUILIBRIUM_SEED_H
