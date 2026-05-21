#ifndef AL_UTILITIES_H
#define AL_UTILITIES_H

/**
 * General-purpose utility functions for the Access Layer.
*/

#include <vector>
#include <string>

namespace utilities {

/**
 * Convert a vector of strings into a C-style array of strings.
 *
 * @param paths the vector of strings to convert
 * @param path_list a pointer to the C-style array to create
 * @param size a pointer to an integer to store the size of the created array
 */
void copy_stringvector_to_c_list(const std::vector<std::string>& paths, char*** path_list, int* size);

} // namespace utilities

#endif // AL_UTILITIES_H
