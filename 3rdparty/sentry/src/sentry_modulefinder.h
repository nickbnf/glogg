#ifndef SENTRY_MODULEFINDER_H_INCLUDED
#define SENTRY_MODULEFINDER_H_INCLUDED

#include "sentry_boot.h"

/**
 * This will lazily load and cache a list of all the loaded modules.
 * The returned value is borrowed.
 */
sentry_value_t sentry__modules_get_list(void);

/**
 * This will clean up the internal module cache. The list will be lazily loaded
 * again on the next call to `sentry__modules_get_list`.
 */
void sentry__modulefinder_cleanup(void);

#endif
