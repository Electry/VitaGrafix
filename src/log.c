#include <vitasdk.h>
#include <taihen.h>
#include <stdio.h>

#include "config.h"
#include "io.h"
#include "main.h"
#include "log.h"

void vg_log_prepare() {
#ifndef ENABLE_VERBOSE_LOGGING
    if (g_main.config.log_enabled != FT_ENABLED)
        return;
#endif

    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd >= 0)
        sceIoClose(fd);
}

void vg_log_printf(const char *format, ...) {
#ifndef ENABLE_VERBOSE_LOGGING
    if (g_main.config.log_enabled != FT_ENABLED)
        return;
#endif

    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd < 0)
        return;

    va_list args;
    va_start(args, format);

    char buffer[LOG_BUFFER_SIZE];
    vsnprintf(buffer, sizeof(buffer), format, args);

    sceIoWrite(fd, buffer, strlen(buffer));
    sceIoClose(fd);

    va_end(args);
}

void vg_log_read(char *dest, int size) {
#ifndef ENABLE_VERBOSE_LOGGING
    if (g_main.config.log_enabled != FT_ENABLED)
        return;
#endif

    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_RDONLY | SCE_O_CREAT, 0777);
    if (fd < 0) {
        return;
    }

    long long fsize = sceIoLseek(fd, 0, SCE_SEEK_END);
    if (fsize > size)
        sceIoLseek(fd, -size, SCE_SEEK_CUR);
    else
        sceIoLseek(fd, 0, SCE_SEEK_SET);

    sceIoRead(fd, dest, size);
    sceIoClose(fd);
}
