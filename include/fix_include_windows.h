/*
https://stackoverflow.com/questions/5004858/why-is-stdmin-failing-when-windows-h-is-included

To fix std::min() after any #include "microsoft-mega-api.h"

*/

#undef max
#undef min

#undef MAX
#undef MIN

#define MAX max
#define MIN min

// use the Standard C++ std::min() and std::max() and ensure to #include <algorithm>
#include <stdlib.h>
#include <algorithm>
using std::max;
using std::min;