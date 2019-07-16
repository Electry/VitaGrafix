#ifndef _LOG_H_
#define _LOG_H_

#define LOG_PATH         "ux0:data/VitaGrafix/log.txt"
#define LOG_BUFFER_SIZE  256

void vg_log_prepare();
void vg_log_printf(const char *format, ...);
void vg_log_read(char *dest, int size);

#endif
