// Deterministic single-version equilibrium-seed generator (issue #4).
//
// TEST_STRATEGY.md decision D5: "one realistic single-version equilibrium
// seed — a curated subset (scalar, a profiles_1d array with a timebase, a
// constraints AOS) from a deterministic in-repo generator (generator + hash,
// not committed blobs) — to exercise real AOS/timebase/HDF5-tensorization
// shapes." This header IS the generator: no binary fixture is checked in, the
// shape is produced by code, and the round-trip oracle is a content hash
// rather than a byte-for-byte vector compare (the synthetic-matrix machinery
// in al_contract.h already covers exhaustive byte-exact round trips; this
// seed's job is a *realistic composite shape*, not another datatype sweep).
//
// The three pieces mirror a real equilibrium IDS closely enough to exercise
// each shape class the migration cares about (NORTH_STAR.md's equilibrium
// pilot), while staying opaque strings through the C ABI (decision D1: the
// core attaches no DD semantics to any of these paths):
//   - a scalar                      -> "vacuum_toroidal_field/r0"
//   - a timebase-carrying 2-D array -> "time" (1-D) + "profiles_1d/psi"
//     (2-D: [time][radial], tensorized on HDF5 the same way a real
//     equilibrium/profiles_1d/psi(:,:) would be)
//   - an AOS                        -> "constraints", each element holding
//     two scalar fields ("measured", "weight")

#ifndef AL_CONTRACT_EQUILIBRIUM_SEED_H
#define AL_CONTRACT_EQUILIBRIUM_SEED_H

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <cstdint>
#include <string>
#include <vector>

namespace equilibrium_seed {

constexpr const char* kIds       = "equilibrium";
constexpr const char* kScalar    = "vacuum_toroidal_field/r0";
constexpr const char* kTimebase  = "time";
constexpr const char* kProfile   = "profiles_1d/psi";
constexpr const char* kAos       = "constraints";
constexpr const char* kMeasured  = "measured";
constexpr const char* kWeight    = "weight";

constexpr int kTimeLen     = 3;   // time slices
constexpr int kRadialLen   = 4;   // radial grid points per slice
constexpr int kNConstraints = 3;  // AOS elements

// --- deterministic content -------------------------------------------------
// Plain arithmetic progressions, distinct per role so a transposed or
// swapped-field bug is caught by the hash. Kept clear of the write-path
// "absent" sentinels documented in al_contract.h (kEmptyDouble).
inline double scalar_r0() { return 6.2; }

inline std::vector<double> timebase_values() {
    std::vector<double> t;
    for (int i = 0; i < kTimeLen; ++i) t.push_back(1.0 + 0.5 * i);
    return t;
}

// Row-major [time][radial], matching the (dim=2, size={kTimeLen,kRadialLen})
// shape handed to al_write_data.
inline std::vector<double> profile_values() {
    std::vector<double> v;
    v.reserve(kTimeLen * kRadialLen);
    for (int t = 0; t < kTimeLen; ++t)
        for (int r = 0; r < kRadialLen; ++r)
            v.push_back(10.0 * t + r + 0.25);
    return v;
}

inline double constraint_measured(int i) { return 100.0 + 2.0 * i; }
inline double constraint_weight(int i) { return 0.5 + 0.1 * i; }

// --- FNV-1a over the seed's full content, in a fixed field order -----------
// A hash, not a byte-exact compare, is the oracle here (decision D5): it lets
// the read-back side recompute over whatever shapes it actually observed
// (see verify_and_hash below) without re-deriving a parallel comparison
// structure for every field.
inline uint64_t fnv1a(uint64_t h, const void* data, size_t n) {
    const unsigned char* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}
inline uint64_t fnv1a(uint64_t h, double v) { return fnv1a(h, &v, sizeof(v)); }
inline uint64_t fnv1a(uint64_t h, const std::vector<double>& v) {
    return v.empty() ? h : fnv1a(h, v.data(), v.size() * sizeof(double));
}

constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;

// The expected hash, computed directly from the generator functions above —
// this is the "generator", not a committed golden value: change any generator
// function and this recomputes automatically.
inline uint64_t expected_hash() {
    uint64_t h = kFnvOffsetBasis;
    h = fnv1a(h, scalar_r0());
    h = fnv1a(h, timebase_values());
    h = fnv1a(h, profile_values());
    for (int i = 0; i < kNConstraints; ++i) {
        h = fnv1a(h, constraint_measured(i));
        h = fnv1a(h, constraint_weight(i));
    }
    return h;
}

// --- write the seed into an already-open pulse (its own GLOBAL write) ------
inline al_status_t write(int pulse_ctx) {
    int op = -1;
    al_status_t s = al_begin_global_action(pulse_ctx, kIds, "", WRITE_OP, &op);
    if (s.code != 0) return s;

    s = al_contract::write_data<double>(op, kScalar, {}, {scalar_r0()});
    if (s.code != 0) { al_end_action(op); return s; }

    s = al_contract::write_data<double>(op, kTimebase, {kTimeLen},
                                        timebase_values());
    if (s.code != 0) { al_end_action(op); return s; }

    {
        int size[2] = {kTimeLen, kRadialLen};
        std::vector<double> data = profile_values();
        s = al_write_data(op, kProfile, kTimebase, data.data(), DOUBLE_DATA, 2,
                          size);
        if (s.code != 0) { al_end_action(op); return s; }
    }

    int size = kNConstraints;
    int aos_ctx = -1;
    s = al_begin_arraystruct_action(op, kAos, "", &size, &aos_ctx);
    if (s.code != 0) { al_end_action(op); return s; }
    for (int i = 0; i < kNConstraints; ++i) {
        s = al_contract::write_data<double>(aos_ctx, kMeasured, {},
                                            {constraint_measured(i)});
        if (s.code != 0) break;
        s = al_contract::write_data<double>(aos_ctx, kWeight, {},
                                            {constraint_weight(i)});
        if (s.code != 0) break;
        if (i + 1 < kNConstraints) {
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

// --- read the seed back and recompute the same hash over what was read -----
// Separate from write() (rather than assuming shapes) so any shape drift on
// read (wrong rank, wrong AOS size) shows up as a hash mismatch instead of
// being silently reconciled away.
inline al_status_t read_and_hash(int pulse_ctx, uint64_t* out_hash) {
    int op = -1;
    al_status_t s = al_begin_global_action(pulse_ctx, kIds, "", READ_OP, &op);
    if (s.code != 0) return s;

    uint64_t h = kFnvOffsetBasis;

    std::vector<int> shape;
    std::vector<double> scalar;
    s = al_contract::read_data<double>(op, kScalar, 0, &shape, &scalar);
    if (s.code != 0) { al_end_action(op); return s; }
    h = fnv1a(h, scalar.at(0));

    std::vector<double> tb;
    s = al_contract::read_data<double>(op, kTimebase, 1, &shape, &tb);
    if (s.code != 0) { al_end_action(op); return s; }
    h = fnv1a(h, tb);

    {
        void* buf = nullptr;
        int size[MAXDIM] = {0};
        s = al_read_data(op, kProfile, kTimebase, &buf, DOUBLE_DATA, 2, size);
        if (s.code != 0) { al_end_action(op); return s; }
        const std::size_t n =
            static_cast<std::size_t>(size[0]) * static_cast<std::size_t>(size[1]);
        std::vector<double> profile(static_cast<double*>(buf),
                                    static_cast<double*>(buf) + n);
        free(buf);
        h = fnv1a(h, profile);
    }

    int size = 0;
    int aos_ctx = -1;
    s = al_begin_arraystruct_action(op, kAos, "", &size, &aos_ctx);
    if (s.code != 0) { al_end_action(op); return s; }
    for (int i = 0; i < size; ++i) {
        std::vector<double> measured, weight;
        s = al_contract::read_data<double>(aos_ctx, kMeasured, 0, &shape,
                                           &measured);
        if (s.code != 0) break;
        s = al_contract::read_data<double>(aos_ctx, kWeight, 0, &shape,
                                           &weight);
        if (s.code != 0) break;
        h = fnv1a(h, measured.at(0));
        h = fnv1a(h, weight.at(0));
        if (i + 1 < size) {
            s = al_iterate_over_arraystruct(aos_ctx, 1);
            if (s.code != 0) break;
        }
    }
    al_status_t end_aos = al_end_action(aos_ctx);
    if (s.code == 0) s = end_aos;

    al_status_t end_op = al_end_action(op);
    if (s.code == 0) s = end_op;

    if (s.code == 0) *out_hash = h;
    return s;
}

}  // namespace equilibrium_seed

#endif  // AL_CONTRACT_EQUILIBRIUM_SEED_H
