// mdsplus_fixture_tool — out-of-band stored-backend-version mutation for the
// MDSplus drift fixture (issue #35).
//
// The contract suite's seam is the public C ABI (issues #1/#12); preparing a
// MISMATCHED pulse, however, requires overwriting the VERSION:BACK_MAJOR node
// the backend wrote into the tree at pulse creation — reachable only through
// the raw MDSplus C++ API (<mdsobjects.h>). This standalone, build-gated tool
// owns ALL direct MDSplus API access, so `contract_tests` itself neither
// includes <mdsobjects.h> nor links the MDSplus C++ libraries: it is a
// fixture producer, not part of the asserted contract.
//
// Usage: mdsplus_fixture_tool <pulse_dir> <back-major>
//   <pulse_dir>   directory holding the pulse's "ids" tree files (created
//                 beforehand through the C ABI by the caller; shot 1)
//   <back-major>  integer to store into VERSION:BACK_MAJOR, e.g. 99
//
// MDSPLUS_MODELS_PATH must point at the baked model tree, exactly as for the
// backend itself. Fixtures are regenerated from pinned DD/model inputs at
// test run time — nothing opaque or host-specific is ever committed.

#include <mdsobjects.h>

#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <pulse_dir> <back-major>\n", argv[0]);
        return 2;
    }
    const std::string pulse_dir = argv[1];
    const int back_major = std::atoi(argv[2]);

    const char* models_path = std::getenv("MDSPLUS_MODELS_PATH");
    if (models_path == nullptr || *models_path == '\0') {
        std::fprintf(stderr, "MDSPLUS_MODELS_PATH is not set\n");
        return 2;
    }

    // Reproduces MDSplusBackend::setDataEnv's "ids_path" env-var dance
    // (mdsplus_backend.cpp) well enough to open the same tree the backend
    // itself opened through the C ABI.
    const std::string ids_path = pulse_dir + ";" + models_path;
    if (setenv("ids_path", ids_path.c_str(), 1) != 0) {
        std::fprintf(stderr, "cannot set ids_path\n");
        return 1;
    }

    try {
        MDSplus::Tree tree("ids", /*shot=*/1, "NORMAL");
        MDSplus::TreeNode* node = tree.getNode("VERSION:BACK_MAJOR");
        MDSplus::Int32* value = new MDSplus::Int32(back_major);
        node->putData(value);
        MDSplus::deleteData(value);
        delete node;
    } catch (MDSplus::MdsException& exc) {
        std::fprintf(stderr,
                     "could not poke VERSION:BACK_MAJOR via the MDSplus tree "
                     "API: %s\n",
                     exc.what());
        return 1;
    }

    std::printf("stored VERSION:BACK_MAJOR=%d in %s\n", back_major,
                pulse_dir.c_str());
    return 0;
}
