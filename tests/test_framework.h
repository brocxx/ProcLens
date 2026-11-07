#ifndef PROCLENS_TEST_FRAMEWORK_H
#define PROCLENS_TEST_FRAMEWORK_H

#include <stdio.h>

#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return 1; \
        } \
    } while (0)

#endif

