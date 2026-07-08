// Contract-test harness for the public C ABI (include/al_lowlevel.h).
//
// This header is the fixture/generator layer of the Task-2 contract suite
// (see TEST_STRATEGY.md). It exists to make the *machine* — temp pulse dirs,
// URI construction, data-entry lifecycle, and typed round-trip helpers —
// trivial and reusable, so the tests themselves read as behavior assertions
// against the ABI rather than boilerplate.
//
// Everything here binds only to the `extern "C"` surface (al_lowlevel.h +
// al_const.h), per decision D1: no C++ internals, no Python bindings.

#ifndef AL_CONTRACT_H
#define AL_CONTRACT_H

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <atomic>
#include <complex>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#define AL_CONTRACT_GETPID _getpid
#else
#include <unistd.h>
#define AL_CONTRACT_GETPID getpid
#endif

namespace al_contract {

// ---------------------------------------------------------------------------
// Status assertion helpers
// ---------------------------------------------------------------------------
// al_status_t carries {code, message}; code == 0 means success. These wrap the
// pair so a failing ABI call reports its own diagnostic message, not just a
// bare code mismatch.

inline ::testing::AssertionResult IsOk(const al_status_t& s) {
    if (s.code == 0) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure()
           << "al_status_t.code=" << s.code
           << " message=" << (s.message ? s.message : "(null)");
}

#define AL_ASSERT_OK(expr) ASSERT_TRUE(::al_contract::IsOk(expr))
#define AL_EXPECT_OK(expr) EXPECT_TRUE(::al_contract::IsOk(expr))

// ---------------------------------------------------------------------------
// BackendCase descriptor for value-parametrized tests (TEST_P).
// ---------------------------------------------------------------------------
struct BackendCase {
    int         id;         // BACKEND enum value from al_const.h
    const char* name;       // human-readable label for the ctest name suffix
    bool        on_disk;    // needs the legacy <base>/<db>/<ver>/<pulse>/<run> tree
};

// GoogleTest prints this for the parametrized test-name suffix.
inline void PrintTo(const BackendCase& b, std::ostream* os) { *os << b.name; }

// ---------------------------------------------------------------------------
// The legacy address of a pulse: (database, version, pulse, run). These four
// always travel together — into the URI and into the on-disk tree path — so
// they are one value, not four loose args.
// ---------------------------------------------------------------------------
struct PulseId {
    std::string database;
    std::string version;
    int         pulse;
    int         run;
};

// ---------------------------------------------------------------------------
// Unique temp base directory, RAII-cleaned.
// ---------------------------------------------------------------------------
// The legacy pulse path is <base>/<database>/<version>/<pulse>/<run>; the core
// derives it when the base dir is supplied as the "user" field of the legacy
// URI (exactly how tests/CMakeLists.txt drives the existing smoke tests). Each
// fixture instance gets its own base dir so parallel ctest runs never collide:
// name = <temp>/al_contract_<pid>_<counter>.
class TempBase {
public:
    TempBase() {
        static std::atomic<unsigned> counter{0};
        namespace fs = std::filesystem;
        const unsigned n = counter.fetch_add(1);
        path_ = fs::temp_directory_path() /
                ("al_contract_" + std::to_string(AL_CONTRACT_GETPID()) +
                 "_" + std::to_string(n));
        std::error_code ec;
        fs::remove_all(path_, ec);
        fs::create_directories(path_, ec);
    }

    ~TempBase() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    TempBase(const TempBase&) = delete;
    TempBase& operator=(const TempBase&) = delete;

    std::string str() const { return path_.string(); }

    // Pre-create the legacy pulse subtree an on-disk backend expects for
    // FORCE_CREATE_PULSE.
    void make_legacy_tree(const PulseId& id) const {
        std::error_code ec;
        std::filesystem::create_directories(
            path_ / id.database / id.version / std::to_string(id.pulse) /
                std::to_string(id.run),
            ec);
    }

private:
    std::filesystem::path path_;
};

// ---------------------------------------------------------------------------
// URI construction through the legacy-parameter ABI.
// ---------------------------------------------------------------------------
// Uses al_build_uri_from_legacy_parameters so the URI is whatever the core
// itself produces for a given backend id — we don't hand-format schemes.
//
// Exception: the always-on FLEXBUFFERS backend. al_build_uri_from_legacy_parameters
// -> getURIBackend (src/al_context.cpp:280) has no FLEXBUFFERS_BACKEND case and
// throws "not yet implemented", even though the *parse* side accepts the
// "flexbuffers" scheme (al_context.cpp:214). So for that one backend we build
// the same legacy query URI directly — mirroring the exact format
// build_uri_from_legacy_parameters emits (al_context.cpp:252), including its
// version[0] single-char quirk — so the storage round trip is still exercised.
// The builder gap itself is a genuine defect, tracked separately as a
// known-defect test (see test_known_defects.cpp); it is NOT papered over here.
inline std::string build_uri(int backend_id, const std::string& base,
                             const PulseId& id) {
    if (backend_id == FLEXBUFFERS_BACKEND) {
        return "imas:flexbuffers?user=" + base + ";pulse=" +
               std::to_string(id.pulse) + ";run=" + std::to_string(id.run) +
               ";database=" + id.database + ";version=" +
               (id.version.empty() ? std::string() : id.version.substr(0, 1));
    }
    char* uri = nullptr;
    al_status_t s = al_build_uri_from_legacy_parameters(
        backend_id, id.pulse, id.run, base.c_str(), id.database.c_str(),
        id.version.c_str(), "", &uri);
    EXPECT_TRUE(IsOk(s)) << "al_build_uri_from_legacy_parameters failed";
    std::string out = (uri ? uri : "");
    free(uri);
    return out;
}

// ---------------------------------------------------------------------------
// Typed round-trip helpers over al_write_data / al_read_data.
// ---------------------------------------------------------------------------
inline al_status_t write_int_scalar(int ctx, const char* field, int value) {
    // Scalar: dim=0, size=NULL (mirrors testlowlevel.cpp).
    return al_write_data(ctx, field, "", &value, INTEGER_DATA, 0, nullptr);
}

// Reads an INTEGER scalar into `out`. For dim=0 the core fills the caller's
// buffer, so we pass a pointer to our own int (no free needed).
inline al_status_t read_int_scalar(int ctx, const char* field, int* out) {
    int size[MAXDIM] = {0};
    void* buf = out;
    return al_read_data(ctx, field, "", &buf, INTEGER_DATA, 0, size);
}

inline al_status_t write_char_array(int ctx, const char* field,
                                    const std::string& value) {
    int size[1] = {static_cast<int>(value.size())};
    return al_write_data(ctx, field, "",
                         const_cast<char*>(value.c_str()), CHAR_DATA, 1, size);
}

// Reads a CHAR array. For dim>=1 the core allocates the buffer and returns it
// via *data; the caller owns it, so we copy into std::string and free.
inline al_status_t read_char_array(int ctx, const char* field,
                                   std::string* out) {
    int size[MAXDIM] = {0};
    char* buf = nullptr;
    al_status_t s = al_read_data(ctx, field, "", reinterpret_cast<void**>(&buf),
                                 CHAR_DATA, 1, size);
    if (s.code == 0 && buf != nullptr) {
        out->assign(buf, static_cast<size_t>(size[0]));
    }
    free(buf);
    return s;
}

// ===========================================================================
// Synthetic-data generator + generic typed round-trip (issue #3).
// ===========================================================================
// The bulk of the storage contract is proven by *round-trip self-consistency*
// over synthetic opaque data (decision D5): write a deterministic buffer, read
// it back, assert byte-for-byte equality. It needs zero DD artifacts, which is
// itself the proof of the version-free property. This block generalizes the
// two hand-written scalar/char helpers above to the whole
// datatype × shape space.

// --- datatype descriptor: element C++ type -> ABI datatype id + label -------
template <class T>
struct DType;
template <>
struct DType<char> {
    static constexpr int         value = CHAR_DATA;
    static constexpr const char* name  = "CHAR";
};
template <>
struct DType<int> {
    static constexpr int         value = INTEGER_DATA;
    static constexpr const char* name  = "INTEGER";
};
template <>
struct DType<double> {
    static constexpr int         value = DOUBLE_DATA;
    static constexpr const char* name  = "DOUBLE";
};
template <>
struct DType<std::complex<double>> {
    static constexpr int         value = COMPLEX_DATA;
    static constexpr const char* name  = "COMPLEX";
};

// The core's "this value means absent" sentinels (src/al_lowlevel.cpp:41-44).
// A *scalar* write whose value equals its sentinel is silently dropped by
// Lowlevel::data_has_non_zero_shape (al_lowlevel.cpp:759) — the generator must
// never emit one, or the round trip would compare against a default-filled
// read and give a false failure. (Arrays are gated on shape, not value, so
// element values there are unrestricted; we stay clear for all ranks anyway.)
inline constexpr int    kEmptyInt    = -999999999;
inline constexpr double kEmptyDouble = -9.0E40;

// --- deterministic synthetic value for flat element index i -----------------
// Distinct per index so a transposed / off-by-one round trip is caught, and
// guaranteed clear of every sentinel above.
template <class T>
T synth_value(std::size_t i);

template <>
inline char synth_value<char>(std::size_t i) {
    // Printable ASCII 33..122, never '\0' (EMPTY_CHAR).
    return static_cast<char>('!' + static_cast<int>(i % 90));
}
template <>
inline int synth_value<int>(std::size_t i) {
    int v = static_cast<int>(i) * 7 - 11;  // spans negatives and positives
    return v == kEmptyInt ? 1 : v;
}
template <>
inline double synth_value<double>(std::size_t i) {
    double v = static_cast<double>(i) * 1.5 - 3.25;  // exactly representable
    return v == kEmptyDouble ? 1.0 : v;
}
template <>
inline std::complex<double> synth_value<std::complex<double>>(std::size_t i) {
    double re = static_cast<double>(i) * 1.5 - 3.25;
    double im = static_cast<double>(i) * -0.75 + 2.0;
    if (re == kEmptyDouble) re = 1.0;
    if (im == kEmptyDouble) im = 1.0;
    return {re, im};  // clear of EMPTY_COMPLEX == (-9e40,-9e40)
}

// --- shape for a given rank -------------------------------------------------
// rank 0 -> scalar (empty shape, one element). rank R -> R dims sized 2,3,2,3…
// so every dimension is >= 1 (a zero dim would itself read as "absent") while
// the 7-D total stays small (2*3*2*3*2*3*2 = 432 elements).
inline std::vector<int> shape_for_rank(int rank) {
    std::vector<int> s;
    s.reserve(static_cast<std::size_t>(rank));
    for (int i = 0; i < rank; ++i) s.push_back((i % 2 == 0) ? 2 : 3);
    return s;
}

inline std::size_t element_count(const std::vector<int>& shape) {
    std::size_t n = 1;
    for (int d : shape) n *= static_cast<std::size_t>(d);
    return n;
}

template <class T>
std::vector<T> synth_buffer(std::size_t n) {
    std::vector<T> v;
    v.reserve(n);
    for (std::size_t i = 0; i < n; ++i) v.push_back(synth_value<T>(i));
    return v;
}

// --- generic write ----------------------------------------------------------
// al_write_data takes a non-const void* and int* size but does not mutate
// them for a write; the const_casts keep the caller's data/shape const.
template <class T>
al_status_t write_data(int ctx, const char* field, const std::vector<int>& shape,
                       const std::vector<T>& data) {
    const int dim  = static_cast<int>(shape.size());
    int*      size = dim ? const_cast<int*>(shape.data()) : nullptr;
    return al_write_data(ctx, field, "", const_cast<T*>(data.data()),
                         DType<T>::value, dim, size);
}

// --- generic read -----------------------------------------------------------
// Honors the two ownership regimes the ABI uses (mirrors testlowlevel.cpp):
//   dim == 0 : the core writes into the caller's buffer; nothing to free.
//   dim >= 1 : the core malloc's the buffer and returns it via *data; the
//              caller owns it, so we copy out and free.
// On success out_shape is the shape the core reported (scalar -> empty) and
// out_data holds element_count(out_shape) elements.
template <class T>
al_status_t read_data(int ctx, const char* field, int expected_rank,
                      std::vector<int>* out_shape, std::vector<T>* out_data) {
    int size[MAXDIM] = {0};

    if (expected_rank == 0) {
        T     scalar{};
        void* buf = &scalar;
        al_status_t s =
            al_read_data(ctx, field, "", &buf, DType<T>::value, 0, size);
        if (s.code == 0) {
            out_shape->clear();
            out_data->assign(1, scalar);
        }
        return s;
    }

    void*       buf = nullptr;
    al_status_t s   = al_read_data(ctx, field, "", &buf, DType<T>::value,
                                   expected_rank, size);
    if (s.code == 0 && buf != nullptr) {
        out_shape->assign(size, size + expected_rank);
        const std::size_t n = element_count(*out_shape);
        T*                p = static_cast<T*>(buf);
        out_data->assign(p, p + n);
    }
    free(buf);
    return s;
}

}  // namespace al_contract

#endif  // AL_CONTRACT_H
