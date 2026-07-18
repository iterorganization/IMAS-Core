// Generic typed round-trip helpers over al_write_data / al_read_data
// (issue #51). Factored out of tests/contract/al_contract.h so a build target
// outside tests/contract/ -- notably a future benchmarks/ tree (PRD #50, issue
// #52) -- can link the exact same write_data/read_data machinery
// equilibrium_seed.h uses, without also pulling in GoogleTest. tests/contract/
// al_contract.h includes this header for its own copy of these helpers, so
// there is exactly one definition either way.
//
// Binds only to the `extern "C"` surface (al_lowlevel.h + al_const.h), per
// TEST_STRATEGY.md decision D1: no C++ internals, no Python bindings.

#ifndef AL_CONTRACT_ABI_H
#define AL_CONTRACT_ABI_H

#include <al_lowlevel.h>
#include <al_const.h>

#include <complex>
#include <cstddef>
#include <cstdlib>
#include <vector>

namespace al_contract {

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

inline std::size_t element_count(const std::vector<int>& shape) {
    std::size_t n = 1;
    for (int d : shape) n *= static_cast<std::size_t>(d);
    return n;
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

#endif  // AL_CONTRACT_ABI_H
