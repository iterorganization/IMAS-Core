// hdf5_fixture_tool — out-of-band stored-backend-version mutation for the
// HDF5 drift fixtures (issues #36 and #39).
//
// The contract suite asserts version-drift behavior exclusively through the
// public C ABI; preparing a MISMATCHED pulse, however, requires rewriting the
// HDF5_BACKEND_VERSION attribute the backend wrote into the master file — an
// operation no public API offers (by design). This standalone tool owns that
// single out-of-band step, so `contract_tests` itself never includes or links
// the HDF5 C API: the same producer/observer split the MDSplus drift tier
// uses (mdsplus_fixture_tool.cpp, issue #35).
//
// Usage: hdf5_fixture_tool <pulse_dir> <version-string>
//   <pulse_dir>       directory holding an existing pulse's master.h5
//                     (created beforehand through the C ABI by the caller)
//   <version-string>  value to store, e.g. "999.0" or "1.999"
//
// Fixtures are always regenerated from the current backend at test run time —
// nothing opaque or host-specific is ever committed.

#include <hdf5.h>

#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr,
                     "usage: %s <pulse_dir> <version-string>\n", argv[0]);
        return 2;
    }
    const std::string master = std::string(argv[1]) + "/master.h5";
    const std::string version = argv[2];
    const char* attr_name = "HDF5_BACKEND_VERSION";

    hid_t file_id = H5Fopen(master.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    if (file_id < 0) {
        std::fprintf(stderr, "cannot open %s for writing\n", master.c_str());
        return 1;
    }

    // The stored attribute's size is baked into its datatype, so replace the
    // attribute wholesale instead of writing into the old one.
    if (H5Aexists(file_id, attr_name) > 0 &&
        H5Adelete(file_id, attr_name) < 0) {
        std::fprintf(stderr, "cannot delete existing %s\n", attr_name);
        H5Fclose(file_id);
        return 1;
    }

    hid_t dataspace_id = H5Screate(H5S_SCALAR);
    hid_t dtype_id = H5Tcopy(H5T_C_S1);
    H5Tset_size(dtype_id, version.length() + 1);
    H5Tset_cset(dtype_id, H5T_CSET_UTF8);
    hid_t att_id = H5Acreate2(file_id, attr_name, dtype_id, dataspace_id,
                              H5P_DEFAULT, H5P_DEFAULT);
    int rc = 0;
    if (att_id < 0 || H5Awrite(att_id, dtype_id, version.c_str()) < 0) {
        std::fprintf(stderr, "cannot write %s\n", attr_name);
        rc = 1;
    }
    if (att_id >= 0) H5Aclose(att_id);
    H5Tclose(dtype_id);
    H5Sclose(dataspace_id);
    H5Fclose(file_id);

    if (rc == 0) {
        std::printf("stored %s=\"%s\" in %s\n", attr_name, version.c_str(),
                    master.c_str());
    }
    return rc;
}
