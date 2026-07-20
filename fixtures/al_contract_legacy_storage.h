// Shared filesystem support for contract fixtures and benchmarks that exercise
// the legacy data-entry URI layout. This header deliberately has no GoogleTest
// dependency: callers choose how to report URI or ABI failures.
#ifndef AL_CONTRACT_LEGACY_STORAGE_H
#define AL_CONTRACT_LEGACY_STORAGE_H

#include <atomic>
#include <filesystem>
#include <string>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace al_contract {

// The legacy address of a pulse: (database, version, pulse, run). These four
// values always travel together — into the URI and into the on-disk tree path.
struct PulseId {
    std::string database;
    std::string version;
    int         pulse;
    int         run;
};

// Owns a unique temporary base directory and prepares the legacy pulse layout
// required by on-disk backends using FORCE_CREATE_PULSE.
class LegacyPulseDirectory {
public:
    LegacyPulseDirectory() {
        static std::atomic<unsigned> counter{0};
        const unsigned n = counter.fetch_add(1);
        path_ = std::filesystem::temp_directory_path() /
                ("al_contract_" + std::to_string(process_id()) + "_" +
                 std::to_string(n));
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
        std::filesystem::create_directories(path_, ec);
    }

    ~LegacyPulseDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    LegacyPulseDirectory(const LegacyPulseDirectory&) = delete;
    LegacyPulseDirectory& operator=(const LegacyPulseDirectory&) = delete;

    const std::filesystem::path& path() const { return path_; }
    std::string                  str() const { return path_.string(); }

    void make_legacy_tree(const PulseId& id) const {
        std::error_code ec;
        std::filesystem::create_directories(
            path_ / id.database / id.version / std::to_string(id.pulse) /
                std::to_string(id.run),
            ec);
    }

private:
    static int process_id() {
#if defined(_WIN32)
        return _getpid();
#else
        return getpid();
#endif
    }

    std::filesystem::path path_;
};

}  // namespace al_contract

#endif  // AL_CONTRACT_LEGACY_STORAGE_H
