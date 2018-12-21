#ifndef _LOG_H_
#define _LOG_H_

#define LOG_PATH         "ux0:data/VitaGrafix/log.txt"
#define LOG_BUFFER_SIZE  256

int vsnprintf(char *s, size_t n, const char *format, va_list arg);

void vgLogPrepare();
void vgLogPrintF(const char *format, ...);

#endif
