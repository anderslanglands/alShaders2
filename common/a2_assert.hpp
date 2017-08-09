#pragma once

#include "config/config.h"

#if defined(ALS_ASSERTIONS_ENABLED) && !defined(NDEBUG)
#if defined(ALS_ASSERTIONS_ABORT)
#include <spdlog/fmt/fmt.h>
#define a2assert(e, ...)                                                       \
    if (!(e)) {                                                                \
        fmt::print(stderr, "ASSERTION FAILED: \"{}\" at {}:{}\n", #e,          \
                   __FILE__, __LINE__);                                        \
        fmt::print(stderr, __VA_ARGS__);                                       \
        fmt::print(stderr, "\n");                                              \
        std::abort();                                                          \
    }
#else

#include <spdlog/fmt/fmt.h>
#define a2assert(e, ...)                                                       \
    if (!(e)) {                                                                \
        fmt::print(stderr, "ASSERTION FAILED: \"{}\" at {}:{}\n", #e,          \
                   __FILE__, __LINE__);                                        \
        fmt::print(stderr, __VA_ARGS__);                                       \
        fmt::print(stderr, "\n");                                              \
    }
#endif
#else

#define a2assert(...)

#endif
