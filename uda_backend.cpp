#include "uda_backend.h"

#include <cstdarg>

std::string UDABackend::format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    size_t len = static_cast<size_t>(vsnprintf(NULL, 0, fmt, args));

    char* str = (char*)malloc(len + 1);

    vsnprintf(str, len, fmt, args);
    str[len] = '\0';

    std::string result(str);
    free(str);

    va_end(args);

    return result;
}