#include <vitasdk.h>
#include <taihen.h>

#include "io.h"
#include "log.h"
#include "config.h"
#include "main.h"

int vgIoFindEOL(const char chunk[], int pos, int end) {
    for (int i = pos; i < end; i++) {
        if (chunk[i] == '\n') {
            return i;
        }
    }

    return -1;
}

VG_IoParseState vgIoParse(const char *path, uint8_t (*parseLineFn)(const char chunk[], int pos, int end)) {
    VG_IoParseState ret = IO_OK;
    
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY | SCE_O_CREAT, 0777);
    if (fd < 0) {
        ret = IO_OPEN_FAILED;
        goto END;
    }

    int chunk_size = 0;
    char chunk[IO_CHUNK_SIZE] = "";
    int pos = 0, end = 0;
    int line_counter = 0;

    while ((chunk_size = sceIoRead(fd, chunk, IO_CHUNK_SIZE)) > 1) {
#ifdef ENABLE_VERBOSE_LOGGING
        vgLogPrintF("[IO] Parsing new chunk, size=%d\n", chunk_size);
#endif
        pos = 0;

        // Read all lines in chunk
        while (pos < chunk_size) {
            // Get next EOL
            end = vgIoFindEOL(chunk, pos, chunk_size);

            // Did not find EOL in current chunk & there is more to read?
            // Proceed by reading next sub-chunk
            if (end == -1 && chunk_size == IO_CHUNK_SIZE) {
#ifdef ENABLE_VERBOSE_LOGGING
                vgLogPrintF("[IO] Didnt find EOL in this chunk, pos=%d, seek=%d\n",
                        pos, 0 - (IO_CHUNK_SIZE - pos));
#endif
                // Single line is > CONFIG_CHUNK_SIZE chars
                if (pos == 0) {
                    vgLogPrintF("[IO] ERROR: Line has > %d characters!\n", IO_CHUNK_SIZE);
                    ret = IO_BAD;
                    goto END_CLOSE;
                }

                // Seek back to where unfinished line started
                sceIoLseek(fd, 0 - (IO_CHUNK_SIZE - pos), SCE_SEEK_CUR);
                break;
            }
            // Found EOL, parse line
            else {
                line_counter++;

                // Last chunk, last line, no EOL?
                if (end == -1) {
                    vgLogPrintF("[IO] WARN: Last line is missing EOL char!\n");
                    end = chunk_size;
                }

                // Ignore comments
                if (chunk[pos] == '#')
                    goto NEXT_LINE;

                // Ignore leading spaces and tabs
                while (pos < end && isspace(chunk[pos])) { pos++; }
                if (pos == end)
                    goto NEXT_LINE; // Empty line

                // Parse valid line
                if (parseLineFn(chunk, pos, end) != IO_OK) {
                    vgLogPrintF("[IO] ERROR: Failed to parse line %d, pos=%d, end=%d\n", line_counter, pos, end);
                    ret = IO_BAD;
                    goto END_CLOSE;
                }
NEXT_LINE:
                pos = end + 1;
            }
        }

        // EOF
        if (chunk_size < IO_CHUNK_SIZE)
            break;
    }

END_CLOSE:
    sceIoClose(fd);
END:
    return ret;
}
