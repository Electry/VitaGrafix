#ifndef _IO_H_
#define _IO_H_

#define IO_CHUNK_SIZE  1024

typedef enum {
    IO_OK,
    IO_BAD,
    IO_OPEN_FAILED
} VG_IoParseState;

VG_IoParseState vgIoParse(const char *path, uint8_t (*parseLineFn)(const char chunk[], int pos, int end));

#endif
