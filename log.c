#include <vitasdk.h>
#include <taihen.h>

#include "config.h"
#include "main.h"
#include "log.h"

void vgLogPrepare() {
    sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
}

void vgLogPrintF(const char *format, ...) {
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
