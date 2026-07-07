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
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <ostream>
#include <string>

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
inline std::string build_uri(int backend_id, const std::string& base,
                             const PulseId& id) {
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

}  // namespace al_contract

#endif  // AL_CONTRACT_H
