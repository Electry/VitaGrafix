#include <vitasdk.h>
#include <taihen.h>
#include <stdio.h>

#include "config.h"
#include "io.h"
#include "main.h"
#include "log.h"

static uint32_t g_log_buffer_size = 0;
static char     g_log_buffer[LOG_BUFFER_SIZE];

void vg_log_prepare() {
#ifndef ENABLE_VERBOSE_LOGGING
    if (g_main.config.log_enabled != FT_ENABLED)
        return;
#endif

    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd >= 0)
        sceIoClose(fd);
}

void vg_log_flush() {
#ifndef ENABLE_VERBOSE_LOGGING
    if (g_main.config.log_enabled != FT_ENABLED)
        return;
#endif
    if (!g_log_buffer_size)
        return;

    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd < 0)
        return;

    sceIoWrite(fd, g_log_buffer, g_log_buffer_size);
    sceIoClose(fd);

    g_log_buffer_size = 0;
    memset(g_log_buffer, 0, LOG_BUFFER_SIZE);
}

void vg_log_printf(const char *format, ...) {
#ifndef ENABLE_VERBOSE_LOGGING
    if (g_main.config.log_enabled != FT_ENABLED)
        return;
#endif

    va_list args;
    va_start(args, format);

    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);

    size_t print_len = strlen(buffer);
    if (g_log_buffer_size + print_len >= LOG_BUFFER_SIZE)
        vg_log_flush(); // Flush buffer

    strncpy(&g_log_buffer[g_log_buffer_size], buffer, print_len);
    g_log_buffer_size += print_len;

    va_end(args);
}

void vg_log_read(char *dest, int size) {
#ifndef ENABLE_VERBOSE_LOGGING
    if (g_main.config.log_enabled != FT_ENABLED)
        return;
#endif
    vg_log_flush();

    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_RDONLY | SCE_O_CREAT, 0777);
    if (fd < 0) {
        return;
    }

    long long fsize = sceIoLseek(fd, 0, SCE_SEEK_END);
    if (fsize > size)
        sceIoLseek(fd, -size, SCE_SEEK_CUR);
    else
        sceIoLseek(fd, 0, SCE_SEEK_SET);

    int read = sceIoRead(fd, dest, size - 1);
    dest[read] = '\0';

    sceIoClose(fd);
}
