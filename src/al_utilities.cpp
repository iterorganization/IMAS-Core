#include "al_utilities.h"

#include <cstdlib>
#include <cstring>

namespace utilities {

void copy_stringvector_to_c_list(const std::vector<std::string>& paths, char*** path_list, int* size) {
    *size = static_cast<int>(paths.size());
    
    if (*size == 0) {
        *path_list = nullptr;
        return;
    }
    
    *path_list = static_cast<char**>(malloc(*size * sizeof(char*)));
    for (int i = 0; i < *size; ++i) {
        (*path_list)[i] = strdup(paths[i].c_str());
    }
}
} // namespace utilities