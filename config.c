#include <vitasdk.h>
#include <taihen.h>

#include "config.h"
#include "main.h"
#include "log.h"


static VG_ConfigSection g_config_section = CONFIG_NONE;

static uint8_t vgConfigParseFeatureState(char chunk[], int pos, int end, const char option[], VG_FeatureState *out) {
    if (!strncmp(&chunk[pos], option, strlen(option))) {
        pos += strlen(option) + 1;
        if (pos >= end)
            return 0;

        *out = (chunk[pos] == '0' || !strncmp(&chunk[pos], "OFF", 3)) ? FT_DISABLED : FT_ENABLED;
        return 1;
    }
    return 0;
}

static uint8_t vgConfigParseResolution(char chunk[], int pos, int end, const char option[], VG_FeatureState *ft, VG_Resolution *res, uint8_t *count) {
    if (!strncmp(&chunk[pos], option, strlen(option))) {
        pos += strlen(option) + 1;
        if (pos >= end)
            return 0;

        *ft = (chunk[pos] == '0' || !strncmp(&chunk[pos], "OFF", 3)) ? FT_DISABLED : FT_ENABLED;
        if (!*ft)
            return 1;

        // Single resolution
        if (count == NULL) {
            res->width = strtol(&chunk[pos], NULL, 10);
            while (pos < end && chunk[pos++] != 'x') {}
            res->height = strtol(&chunk[pos], NULL, 10);
        // Multiple resolutions
        } else {
            while (pos < end-1 && *count < MAX_RES_COUNT) {
                res[*count].width = strtol(&chunk[pos], NULL, 10);
                while (pos < end && chunk[pos++] != 'x') {}
                res[*count].height = strtol(&chunk[pos], NULL, 10);
                while (pos < end && chunk[pos++] != ',') {}
                (*count)++;
            }
        }

        return 1;
    }
    return 0;
}

static uint8_t vgConfigParseFramerate(char chunk[], int pos, int end, const char option[], VG_FeatureState *ft, VG_Fps *fps) {
    if (!strncmp(&chunk[pos], option, strlen(option))) {
        pos += strlen(option) + 1;
        if (pos >= end)
            return 0;

        *ft = (chunk[pos] == '0' || !strncmp(&chunk[pos], "OFF", 3)) ? FT_DISABLED : FT_ENABLED;
        if (!*ft)
            return 1;

        *fps = !strncmp(&chunk[pos], "30", 2) ? FPS_30 : FPS_60;
        return 1;
    }
    return 0;
}

static uint8_t vgConfigParseMain(char chunk[], int pos, int end) {
    if (vgConfigParseFeatureState(chunk, pos, end, "ENABLED", &g_main.config.enabled) ||
        vgConfigParseFeatureState(chunk, pos, end, "OSD", &g_main.config.osd_enabled)) {
            return 1;
    }
    vgLogPrintF("Failed parsing option in [MAIN] section, pos=%d, end=%d, string='%.*s'\n",
            pos, end, ((end - pos) > 128 ? (128) : (end - pos)), &chunk[pos]);
    return 0;
}

static uint8_t vgConfigParseGame(char chunk[], int pos, int end) {
    if (vgConfigParseFeatureState(chunk, pos, end, "ENABLED", &g_main.config.game_enabled) ||
        vgConfigParseFeatureState(chunk, pos, end, "OSD", &g_main.config.game_osd_enabled) ||
        vgConfigParseResolution(chunk, pos, end, "FB", &g_main.config.fb_enabled, &g_main.config.fb, NULL) ||
        vgConfigParseResolution(chunk, pos, end, "IB", &g_main.config.ib_enabled, g_main.config.ib, &g_main.config.ib_count) ||
        vgConfigParseFramerate(chunk, pos, end, "FPS", &g_main.config.fps_enabled, &g_main.config.fps)) {
            return 1;
    }
    vgLogPrintF("Failed parsing option in [%s] section, pos=%d, end=%d, string='%.*s'\n",
            g_main.titleid, pos, end, ((end - pos) > 128 ? (128) : (end - pos)), &chunk[pos]);
    return 0;
}

static uint8_t vgConfigParseLine(char chunk[], int pos, int end) {
    // Ignore comments
    if (chunk[pos] == '#')
        return 1;

    // Ignore spaces and tabs at the beginning
    while (pos < end &&
            (chunk[pos] == ' ' || chunk[pos] == '\t'||
            chunk[pos] == '\r' || chunk[pos] == '\n')) { pos++; }
    if (pos == end)
        return 1; // Empty line

    // Check for new section
    if (!strncmp(&chunk[pos], "[MAIN]", 6)) {
        vgLogPrintF("Found [MAIN] section, pos=%d\n", pos);
        g_config_section = CONFIG_MAIN;
        return 1;
    } else if (chunk[pos] == '[') {
        if ((pos + TITLEID_LEN + 1) < end && // does titleid even fit?
                chunk[pos] == '[' &&
                !strncmp(&chunk[pos + 1], g_main.titleid, TITLEID_LEN) &&
                chunk[pos + TITLEID_LEN + 1] == ']') {
            vgLogPrintF("Found [%s] section, pos=%d\n", g_main.titleid, pos, end);
            g_config_section = CONFIG_GAME;
            return 1;
        } else {
            g_config_section = CONFIG_NONE;
            return 1; // Section for a different game
        }
    }

    // Parse data
    if (g_config_section == CONFIG_MAIN) {
        if (vgConfigParseMain(chunk, pos, end))
            return 1;
    } else if (g_config_section == CONFIG_GAME) {
        if (vgConfigParseGame(chunk, pos, end))
            return 1;
    } else if (g_config_section == CONFIG_NONE) {
        return 1; // Section for a different game
    }

    return 0;
}

void vgConfigParse() {
    // Set defaults
    g_main.config.enabled = FT_ENABLED;
    g_main.config.osd_enabled = FT_ENABLED;
    g_main.config.game_enabled = FT_ENABLED;
    g_main.config.game_osd_enabled = FT_ENABLED;
    g_main.config.fb_enabled = FT_DISABLED;
    g_main.config.ib_enabled = FT_DISABLED;
    g_main.config.ib_count = 0;
    g_main.config.fps_enabled = FT_DISABLED;
    g_main.config_state = CONFIG_OK;

    SceUID fd = sceIoOpen(CONFIG_PATH, SCE_O_RDONLY | SCE_O_CREAT, 0777);
    if (fd < 0) {
        g_main.config_state = CONFIG_OPEN_FAILED;
        goto END;
    }

    int chunk_size = 0;
    char chunk[CONFIG_CHUNK_SIZE] = "";
    int pos = 0, end = 0;

    while ((chunk_size = sceIoRead(fd, chunk, CONFIG_CHUNK_SIZE)) > 1) {
#ifdef ENABLE_VERBOSE_LOGGING
        vgLogPrintF("Parsing new chunk, size=%d\n", chunk_size);
#endif
        pos = 0;

        // Read all lines in chunk
        while (pos < chunk_size) {
            end = -1;

            // Find EOL
            for (int i = pos; i < CONFIG_CHUNK_SIZE; i++) {
                if (chunk[i] == '\n') {
                    end = i;
                    break;
                }
            }

            // Did not find EOL in current chunk? read next sub-chunk
            if (end == -1 && chunk_size == CONFIG_CHUNK_SIZE) {
#ifdef ENABLE_VERBOSE_LOGGING
                vgLogPrintF("Didnt find EOL in this chunk, pos=%d, seek=%d\n",
                        pos, 0 - (CONFIG_CHUNK_SIZE - pos));
#endif
                // Single line is > CONFIG_CHUNK_SIZE chars
                if (pos == 0) {
                    vgLogPrintF("Line is > %d !\n", CONFIG_CHUNK_SIZE);
                    g_main.config_state = CONFIG_BAD;
                    goto END_CLOSE;
                }
                sceIoLseek(fd, 0 - (CONFIG_CHUNK_SIZE - pos), SCE_SEEK_CUR);
                break;
            // Found EOL, parse line
            } else {
                if (end == -1) { // When last line doesn't have EOL char
#ifdef ENABLE_VERBOSE_LOGGING
                    vgLogPrintF("Last line is missing EOL char\n");
#endif
                    end = CONFIG_CHUNK_SIZE;
                }

                if (!vgConfigParseLine(chunk, pos, end)) {
                    vgLogPrintF("Failed to parse line, pos=%d, end=%d\n", pos, end);
                    g_main.config_state = CONFIG_BAD;
                    goto END_CLOSE;
                }
                pos = end + 1;
            }
        }

        // EOF
        if (chunk_size < CONFIG_CHUNK_SIZE)
            break;
    }

END_CLOSE:
    sceIoClose(fd);
END:
    return;
}

void vgConfigSetSupported(
        VG_FeatureState fb,
        VG_FeatureState ib,
        VG_FeatureState fps) {
    if (fb == FT_UNSUPPORTED)
        g_main.config.fb_enabled = fb;
    if (ib == FT_UNSUPPORTED)
        g_main.config.ib_enabled = ib;
    if (fps == FT_UNSUPPORTED)
        g_main.config.fps_enabled = fps;
}
void vgConfigSetSupportedIbCount(uint8_t count) {
    if (count < g_main.config.ib_count) {
        // User specified more res. than is supported
        g_main.config.ib_count = count;
    } else {
        // User did not specify enough res.
        for (uint8_t i = g_main.config.ib_count; i < count; i++) {
            g_main.config.ib[i].width = g_main.config.ib[i - 1].width;
            g_main.config.ib[i].height = g_main.config.ib[i - 1].height;
        }
    }
}

uint8_t vgConfigIsFbEnabled() {
    return g_main.config.enabled == FT_ENABLED &&
            g_main.config.game_enabled == FT_ENABLED &&
            g_main.config.fb_enabled == FT_ENABLED;
}
uint8_t vgConfigIsIbEnabled() {
    return g_main.config.enabled == FT_ENABLED &&
            g_main.config.game_enabled == FT_ENABLED &&
            g_main.config.ib_enabled == FT_ENABLED;
}
uint8_t vgConfigIsFpsEnabled() {
    return g_main.config.enabled == FT_ENABLED &&
            g_main.config.game_enabled == FT_ENABLED &&
            g_main.config.fps_enabled == FT_ENABLED;
}
uint8_t vgConfigIsOsdEnabled() {
    return g_main.config.enabled == FT_ENABLED &&
            g_main.config.osd_enabled == FT_ENABLED &&
            g_main.config.game_osd_enabled == FT_ENABLED;
}
