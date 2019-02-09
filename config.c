#include <vitasdk.h>
#include <taihen.h>

#include "io.h"
#include "config.h"
#include "main.h"
#include "log.h"


static VG_ConfigSection g_config_section = CONFIG_NONE;

static VG_IoParseState vgConfigParseFeatureState(
            const char chunk[], int pos, int end, int is_main,
            const char option[], VG_FeatureState *out) {

    if (is_main && *out != FT_UNSPECIFIED)
        return IO_OK; // Ignore [MAIN] if game-specific option is already set

    if (!strncasecmp(&chunk[pos], option, strlen(option))) {
        pos += strlen(option) + 1;
        if (pos >= end)
            return IO_BAD;

        *out = (chunk[pos] == '0' || !strncasecmp(&chunk[pos], "OFF", 3)) ? FT_DISABLED : FT_ENABLED;
        return IO_OK;
    }

    return IO_BAD;
}

static VG_IoParseState vgConfigParseResolution(
            const char chunk[], int pos, int end, int is_main,
            const char option[], VG_FeatureState *ft,
            VG_Resolution *res, uint8_t *count) {

    if (is_main && *ft != FT_UNSPECIFIED)
        return IO_OK; // Ignore [MAIN] if game-specific option is already set

    if (!strncasecmp(&chunk[pos], option, strlen(option))) {
        pos += strlen(option) + 1;
        if (pos >= end)
            return IO_BAD;

        *ft = (chunk[pos] == '0' || !strncasecmp(&chunk[pos], "OFF", 3)) ? FT_DISABLED : FT_ENABLED;
        if (!*ft)
            return IO_OK;

        // Single resolution
        if (count == NULL) {
            res->width = strtol(&chunk[pos], NULL, 10);
            while (pos < end && chunk[pos++] != 'x') {}
            res->height = strtol(&chunk[pos], NULL, 10);
        }
        // Multiple resolutions
        else {
            while (pos < end-1 && *count < MAX_RES_COUNT) {
                res[*count].width = strtol(&chunk[pos], NULL, 10);
                while (pos < end && chunk[pos++] != 'x') {}
                res[*count].height = strtol(&chunk[pos], NULL, 10);
                while (pos < end && chunk[pos++] != ',') {}
                (*count)++;
            }
        }

        return IO_OK;
    }

    return IO_BAD;
}

static VG_IoParseState vgConfigParseFramerate(
            const char chunk[], int pos, int end, int is_main,
            const char option[], VG_FeatureState *ft,
            VG_Fps *fps) {

    if (is_main && *ft != FT_UNSPECIFIED)
        return IO_OK; // Ignore [MAIN] if game-specific option is already set

    if (!strncasecmp(&chunk[pos], option, strlen(option))) {
        pos += strlen(option) + 1;
        if (pos >= end)
            return IO_BAD;

        *ft = (chunk[pos] == '0' || !strncasecmp(&chunk[pos], "OFF", 3)) ? FT_DISABLED : FT_ENABLED;
        if (!*ft)
            return IO_OK;

        *fps = !strncasecmp(&chunk[pos], "30", 2) ? FPS_30 : FPS_60;
        return IO_OK;
    }

    return IO_BAD;
}

static VG_IoParseState vgConfigParseMsaa(
            const char chunk[], int pos, int end, int is_main,
            const char option[], VG_FeatureState *ft,
            VG_Msaa *msaa) {

    if (is_main && *ft != FT_UNSPECIFIED)
        return IO_OK; // Ignore [MAIN] if game-specific option is already set

    if (!strncasecmp(&chunk[pos], option, strlen(option))) {
        pos += strlen(option) + 1;
        if (pos >= end)
            return IO_BAD;

        *ft = !strncasecmp(&chunk[pos], "OFF", 3) ? FT_DISABLED : FT_ENABLED;
        if (!*ft)
            return IO_OK;

        *msaa = chunk[pos] == '4' ? MSAA_4X : (chunk[pos] == '2' ? MSAA_2X : MSAA_NONE);
        return IO_OK;
    }

    return IO_BAD;
}

static VG_IoParseState vgConfigParseSection(const char chunk[], int pos, int end, int is_main) {

    if (!vgConfigParseFeatureState(chunk, pos, end, is_main, "ENABLED", &g_main.config.enabled) ||
        !vgConfigParseFeatureState(chunk, pos, end, is_main, "OSD", &g_main.config.osd_enabled) ||
        !vgConfigParseResolution(chunk, pos, end, is_main, "FB", &g_main.config.fb_enabled, &g_main.config.fb, NULL) ||
        !vgConfigParseResolution(chunk, pos, end, is_main, "IB", &g_main.config.ib_enabled, g_main.config.ib, &g_main.config.ib_count) ||
        !vgConfigParseFramerate(chunk, pos, end, is_main, "FPS", &g_main.config.fps_enabled, &g_main.config.fps) ||
        !vgConfigParseMsaa(chunk, pos, end, is_main, "MSAA", &g_main.config.msaa_enabled, &g_main.config.msaa)) {
            return IO_OK;
    }

    vgLogPrintF("[CONFIG] ERROR: Failed parsing option in [%s] section, pos=%d, end=%d, string='%.*s'\n",
            is_main ? "MAIN" : g_main.titleid, pos, end, ((end - pos) > 128 ? (128) : (end - pos)), &chunk[pos]);

    return IO_BAD;
}

static VG_IoParseState vgConfigParseLine(const char chunk[], int pos, int end) {

    // Check for [MAIN] section
    if (!strncasecmp(&chunk[pos], "[MAIN]", 6)) {
        vgLogPrintF("[CONFIG] Found [MAIN] section, pos=%d\n", pos);
        g_config_section = CONFIG_MAIN;
        return IO_OK;
    }

    // Check for [TITLEID] section
    if (chunk[pos] == '[') {
        if ((pos + TITLEID_LEN + 1) < end && // does titleid even fit?
                !strncasecmp(&chunk[pos + 1], g_main.titleid, TITLEID_LEN) &&
                chunk[pos + TITLEID_LEN + 1] == ']') {
            vgLogPrintF("[CONFIG] Found [%s] section, pos=%d\n", g_main.titleid, pos, end);
            g_config_section = CONFIG_GAME;
            return IO_OK;
        }

        // Section for a different game
        g_config_section = CONFIG_NONE;
        return IO_OK;
    }

    // Parse data
    if (g_config_section == CONFIG_MAIN) {
        if (!vgConfigParseSection(chunk, pos, end, 1))
            return IO_OK;
    } else if (g_config_section == CONFIG_GAME) {
        if (!vgConfigParseSection(chunk, pos, end, 0))
            return IO_OK;
    } else if (g_config_section == CONFIG_NONE) {
        return IO_OK; // Section for a different game
    }

    return IO_BAD;
}

void vgConfigParse() {
    // Reset
    g_main.config.enabled      = FT_UNSPECIFIED;
    g_main.config.osd_enabled  = FT_UNSPECIFIED;
    g_main.config.fb_enabled   = FT_UNSPECIFIED;
    g_main.config.ib_enabled   = FT_UNSPECIFIED;
    g_main.config.ib_count     = 0;
    g_main.config.fps_enabled  = FT_UNSPECIFIED;
    g_main.config.msaa_enabled = FT_UNSPECIFIED;

    vgLogPrintF("[CONFIG] Parsing config.txt\n");
    g_main.config_state = vgIoParse(CONFIG_PATH, vgConfigParseLine);

    // Set defaults
    if (g_main.config.enabled == FT_UNSPECIFIED)
        g_main.config.enabled       = FT_ENABLED;
    if (g_main.config.osd_enabled == FT_UNSPECIFIED)
        g_main.config.osd_enabled   = FT_ENABLED;
    if (g_main.config.fb_enabled == FT_UNSPECIFIED)
        g_main.config.fb_enabled    = FT_DISABLED;
    if (g_main.config.ib_enabled == FT_UNSPECIFIED)
        g_main.config.ib_enabled    = FT_DISABLED;
    if (g_main.config.fps_enabled == FT_UNSPECIFIED)
        g_main.config.fps_enabled   = FT_DISABLED;
    if (g_main.config.msaa_enabled == FT_UNSPECIFIED)
        g_main.config.msaa_enabled  = FT_DISABLED;
}

void vgConfigSetSupported(
        VG_FeatureState fb,
        VG_FeatureState ib,
        VG_FeatureState fps,
        VG_FeatureState msaa) {
    if (fb == FT_UNSUPPORTED)
        g_main.config.fb_enabled = fb;
    if (ib == FT_UNSUPPORTED)
        g_main.config.ib_enabled = ib;
    if (fps == FT_UNSUPPORTED)
        g_main.config.fps_enabled = fps;
    if (msaa == FT_UNSUPPORTED)
        g_main.config.msaa_enabled = msaa;
}
void vgConfigSetSupportedIbCount(uint8_t count) {
    if (g_main.config.ib_count > 0 && count > g_main.config.ib_count) {
        // User did not specify enough res.
        for (uint8_t i = g_main.config.ib_count; i < count; i++) {
            g_main.config.ib[i].width = g_main.config.ib[i - 1].width;
            g_main.config.ib[i].height = g_main.config.ib[i - 1].height;
        }
    }
}

uint8_t vgConfigIsFbEnabled() {
    return g_main.config.enabled == FT_ENABLED &&
            g_main.config.fb_enabled == FT_ENABLED;
}
uint8_t vgConfigIsIbEnabled() {
    return g_main.config.enabled == FT_ENABLED &&
            g_main.config.ib_enabled == FT_ENABLED;
}
uint8_t vgConfigIsFpsEnabled() {
    return g_main.config.enabled == FT_ENABLED &&
            g_main.config.fps_enabled == FT_ENABLED;
}
uint8_t vgConfigIsMsaaEnabled() {
    return g_main.config.enabled == FT_ENABLED &&
            g_main.config.msaa_enabled == FT_ENABLED;
}
uint8_t vgConfigIsOsdEnabled() {
    return g_main.config.enabled == FT_ENABLED &&
            g_main.config.osd_enabled == FT_ENABLED;
}
