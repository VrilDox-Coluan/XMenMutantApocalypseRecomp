#ifndef WOTG_COMPAT_H
#define WOTG_COMPAT_H

#ifdef _WIN32
#include <direct.h>
#include <stdlib.h>

static inline int wotg_setenv(const char *name, const char *value, int overwrite) {
    if (!overwrite && getenv(name)) return 0;
    return _putenv_s(name, value) ? -1 : 0;
}
#define setenv wotg_setenv
#define wotg_mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#define wotg_mkdir(path, mode) mkdir(path, mode)
#endif

#endif
