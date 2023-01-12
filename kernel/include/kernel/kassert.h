#ifndef K_ASSERT_H
#define K_ASSERT_H

// Kernel equivalent of c-macro
#define kassert(expr)                                                                 \
    if (!(expr)) {                                                                    \
        printf("Kernel assertion '%s' failed at %s:%u\n", #expr, __FILE__, __LINE__); \
        abort();                                                                      \
    }
#endif
