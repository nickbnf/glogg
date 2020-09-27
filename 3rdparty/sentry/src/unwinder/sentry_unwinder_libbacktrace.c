#include "sentry_boot.h"

#include <execinfo.h>

size_t
sentry__unwind_stack_libbacktrace(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (addr) {
#ifdef MAC_OS_X_VERSION_10_14
        return backtrace_from_fp(addr, ptrs, max_frames);
#endif
        return 0;
    } else if (uctx) {
        return 0;
    } else {
        return backtrace(ptrs, max_frames);
    }
}
