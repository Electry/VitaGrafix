#include <vitasdk.h>
#include <taihen.h>
#include <stdlib.h>
#include <stdbool.h>

#include "io.h"
#include "config.h"
#include "main.h"
#include "log.h"

static vg_config_section_t g_config_section = CONFIG_SECTION_NONE;

#define OPTIONS_TOTAL 7
static const vg_config_parse_option_t _OPTIONS[OPTIONS_TOTAL] = {
    {"ENABLED", CONFIG_OPTION_FEATURE_STATE, &g_main.config.enabled,      {NULL},                NULL},
    {"OSD",     CONFIG_OPTION_FEATURE_STATE, &g_main.config.osd_enabled,  {NULL},                NULL},
    {"LOG",     CONFIG_OPTION_FEATURE_STATE, &g_main.config.log_enabled,  {NULL},                NULL},
    {"FB",      CONFIG_OPTION_RESOLUTION,    &g_main.config.fb_enabled,   {(void *)&g_main.config.fb},   NULL},
    {"IB",      CONFIG_OPTION_RESOLUTION,    &g_main.config.ib_enabled,   {(void *)g_main.config.ib},    &g_main.config.ib_count},
    {"FPS",     CONFIG_OPTION_FRAMERATE,     &g_main.config.fps_enabled,  {(void *)&g_main.config.fps},  NULL},
    {"MSAA",    CONFIG_OPTION_MSAA,          &g_main.config.msaa_enabled, {(void *)&g_main.config.msaa}, NULL},
};

static vg_io_status_t vg_config_parse_feature_state(
            const char line[], int pos, vg_feature_state_t *out) {
    // Enabled
    if (line[pos] == '1'
            || !strncasecmp(&line[pos], "on", 3)
            || !strncasecmp(&line[pos], "true", 3)) {
        *out = FT_ENABLED;
        __ret_status(IO_OK, 0, 0);
    }
    // Disabled
    if (line[pos] == '0'
            || !strncasecmp(&line[pos], "off", 3)
            || !strncasecmp(&line[pos], "false", 3)) {
        *out = FT_DISABLED;
        __ret_status(IO_OK, 0, 0);
    }

    __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
}

static vg_io_status_t vg_config_parse_resolution(
            const char line[], int pos, vg_feature_state_t *ft,
            vg_res_t *res, uint8_t *count) {
    char *endptr = NULL;
    *ft = FT_ENABLED;

    // Disabled
    if (line[pos] == '0'
            || !strncasecmp(&line[pos], "off", 3)
            || !strncasecmp(&line[pos], "false", 3)) {
        *ft = FT_DISABLED;
        __ret_status(IO_OK, 0, 0);
    }

    // Single resolution
    if (count == NULL) {
        // W
        res->width = strtol(&line[pos], &endptr, 10);
        if (&line[pos] == endptr)
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
        pos += endptr - &line[pos];

        // x
        while (isdigit(line[pos])) { pos++; }
        if (line[pos] != 'x' && line[pos] != 'X')
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
        pos++;

        // H
        res->height = strtol(&line[pos], &endptr, 10);
        if (&line[pos] == endptr)
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
        pos += endptr - &line[pos];
    }
    // Multiple resolutions
    else {
        while (line[pos] != '\0' && *count < MAX_RES_COUNT) {
            // W
            res[*count].width = strtol(&line[pos], &endptr, 10);
            if (&line[pos] == endptr)
                __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
            pos += endptr - &line[pos];

            // x
            while (isspace(line[pos])) { pos++; }
            if (line[pos] != 'x' && line[pos] != 'X')
                __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
            pos++;

            // H
            res[*count].height = strtol(&line[pos], &endptr, 10);
            if (&line[pos] == endptr)
                __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
            pos += endptr - &line[pos];

            (*count)++;

            // ,
            while (isspace(line[pos])) { pos++; }
            if (line[pos] != ',')
                break;
            pos++;
        }
    }

    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_config_parse_framerate(
            const char line[], int pos, vg_feature_state_t *ft, vg_fps_t *fps) {
    *ft = FT_ENABLED;

    // Disabled
    if (line[pos] == '0'
            || !strncasecmp(&line[pos], "off", 3)
            || !strncasecmp(&line[pos], "false", 3)) {
        *ft = FT_DISABLED;
        __ret_status(IO_OK, 0, 0);
    }

    if (!strncasecmp(&line[pos], "60", 2)) {
        *fps = FPS_60;
        __ret_status(IO_OK, 0, 0);
    }
    if (!strncasecmp(&line[pos], "30", 2)) {
        *fps = FPS_30;
        __ret_status(IO_OK, 0, 0);
    }
    if (!strncasecmp(&line[pos], "20", 2)) {
        *fps = FPS_20;
        __ret_status(IO_OK, 0, 0);
    }

    __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
}

static vg_io_status_t vg_config_parse_msaa(
            const char line[], int pos, vg_feature_state_t *ft, vg_msaa_t *msaa) {
    *ft = FT_ENABLED;

    // Disabled
    if (line[pos] == '0'
            || !strncasecmp(&line[pos], "off", 3)
            || !strncasecmp(&line[pos], "false", 3)) {
        *ft = FT_DISABLED;
        __ret_status(IO_OK, 0, 0);
    }

    if (line[pos] == '4') {
        *msaa = MSAA_4X;
        __ret_status(IO_OK, 0, 0);
    }
    if (line[pos] == '2') {
        *msaa = MSAA_2X;
        __ret_status(IO_OK, 0, 0);
    }
    if (line[pos] == '1') {
        *msaa = MSAA_NONE;
        __ret_status(IO_OK, 0, 0);
    }

    __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
}

static vg_io_status_t vg_config_parse_option(const char line[]) {
    vg_io_status_t ret = {IO_ERROR_PARSE_INVALID_TOKEN, 0, 0};
    int pos = 0;

    for (int i = 0; i < OPTIONS_TOTAL; i++) {
        size_t len = strlen(_OPTIONS[i].name);
        int pos_rhs = pos + len;

        if (!strncasecmp(&line[pos], _OPTIONS[i].name, len)) {
            // Ignore [MAIN] if game-specific option is already set
            if (g_config_section == CONFIG_SECTION_MAIN && *(_OPTIONS[i].ft_state) != FT_UNSPECIFIED)
                __ret_status(IO_OK, 0, 0);

            while (line[pos_rhs] != '\0' && isspace(line[pos_rhs])) { pos_rhs++; }

            // Check '=' char
            if (line[pos_rhs] != '=') {
                ret.pos_line = pos_rhs;
                return ret;
            }
            pos_rhs++;

            switch (_OPTIONS[i].type) {
                case CONFIG_OPTION_FEATURE_STATE:
                    return vg_config_parse_feature_state(line, pos_rhs, _OPTIONS[i].ft_state);
                case CONFIG_OPTION_RESOLUTION:
                    return vg_config_parse_resolution(line, pos_rhs, _OPTIONS[i].ft_state, _OPTIONS[i].res, _OPTIONS[i].count);
                case CONFIG_OPTION_FRAMERATE:
                    return vg_config_parse_framerate(line, pos_rhs, _OPTIONS[i].ft_state, _OPTIONS[i].fps);
                case CONFIG_OPTION_MSAA:
                    return vg_config_parse_msaa(line, pos_rhs, _OPTIONS[i].ft_state, _OPTIONS[i].msaa);
            }
        }
    }

    return ret;
}

static vg_io_status_t vg_config_parse_line(const char line[]) {
    vg_io_status_t ret = {IO_OK, 0, 0};

    // Check for a new section
    if (line[0] == '[') {
        // [MAIN]
        if (!strncasecmp(line, "[MAIN]", 6)) {
            g_config_section = CONFIG_SECTION_MAIN;
            return ret;
        }

        // [TITLEID,SELF,NID]
        char titleid[TITLEID_LEN + 1] = TITLEID_ANY;
        char self[SELF_LEN_MAX + 1] = SELF_ANY;
        uint32_t nid = NID_ANY;

        ret = vg_io_parse_section_header(line, titleid, self, &nid);
        if (ret.code != IO_OK)
            return ret;

        if (vg_main_is_game(titleid, self, nid)) {
            g_config_section = CONFIG_SECTION_GAME;
            return ret;
        }

        g_config_section = CONFIG_SECTION_NONE;
        return ret;
    }

    // Parse option
    if (g_config_section != CONFIG_SECTION_NONE)
        return vg_config_parse_option(line);

    return ret;
}

void vg_config_set_defaults() {
    if (g_main.config.enabled == FT_UNSPECIFIED)
        g_main.config.enabled       = FT_ENABLED;
    if (g_main.config.osd_enabled == FT_UNSPECIFIED)
        g_main.config.osd_enabled   = FT_ENABLED;
    if (g_main.config.log_enabled == FT_UNSPECIFIED)
        g_main.config.log_enabled   = FT_ENABLED;
    if (g_main.config.fb_enabled == FT_UNSPECIFIED)
        g_main.config.fb_enabled    = FT_DISABLED;
    if (g_main.config.ib_enabled == FT_UNSPECIFIED)
        g_main.config.ib_enabled    = FT_DISABLED;
    if (g_main.config.fps_enabled == FT_UNSPECIFIED)
        g_main.config.fps_enabled   = FT_DISABLED;
    if (g_main.config.msaa_enabled == FT_UNSPECIFIED)
        g_main.config.msaa_enabled  = FT_DISABLED;
}

void vg_config_propagate_ib() {
    for (uint8_t i = g_main.config.ib_count; i < MAX_RES_COUNT; i++) {
        g_main.config.ib[i].width = g_main.config.ib[i - 1].width;
        g_main.config.ib[i].height = g_main.config.ib[i - 1].height;
    }
}

void vg_config_parse() {
    // Reset
    g_main.config.enabled      = FT_UNSPECIFIED;
    g_main.config.osd_enabled  = FT_UNSPECIFIED;
    g_main.config.log_enabled  = FT_UNSPECIFIED;
    g_main.config.fb_enabled   = FT_UNSPECIFIED;
    g_main.config.ib_enabled   = FT_UNSPECIFIED;
    g_main.config.ib_count     = 0;
    g_main.config.fps_enabled  = FT_UNSPECIFIED;
    g_main.config.msaa_enabled = FT_UNSPECIFIED;

    // Parse config.txt
    g_main.config_status = vg_io_parse(CONFIG_PATH, vg_config_parse_line, true);

    // Set unset options to their default values
    vg_config_set_defaults();

    // Propagate last specified IB res. (for multires patches)
    vg_config_propagate_ib();

#ifdef ENABLE_VERBOSE_LOGGING
    vg_log_printf("[CONFIG] Config:\n");
    vg_log_printf("[CONFIG] ENABLED: %d\n", g_main.config.enabled);
    vg_log_printf("[CONFIG] OSD: %d\n", g_main.config.osd_enabled);
    vg_log_printf("[CONFIG] LOG: %d\n", g_main.config.log_enabled);
    vg_log_printf("[CONFIG] FB: %d %dx%d\n", g_main.config.fb_enabled, g_main.config.fb.width, g_main.config.fb.height);
    vg_log_printf("[CONFIG] IB: %d #%d 1st:%dx%d\n", g_main.config.ib_enabled, g_main.config.ib_count, g_main.config.ib[0].width, g_main.config.ib[0].height);
    vg_log_printf("[CONFIG] FPS: %d %d\n", g_main.config.fps_enabled, g_main.config.fps);
    vg_log_printf("[CONFIG] MSAA: %d %d\n", g_main.config.msaa_enabled, g_main.config.msaa);
#endif
}

bool vg_config_is_feature_enabled(vg_feature_t feature) {
    if (!g_main.config.enabled)
        return false;

    switch (feature) {
        case FEATURE_FB:   return g_main.config.fb_enabled == FT_ENABLED;
        case FEATURE_IB:   return g_main.config.ib_enabled == FT_ENABLED;
        case FEATURE_FPS:  return g_main.config.fps_enabled == FT_ENABLED;
        case FEATURE_MSAA: return g_main.config.msaa_enabled == FT_ENABLED;
        default: return false;
    }

    return false;
}

void vg_config_set_unsupported_features(vg_feature_state_t states[]) {
    if (states[FEATURE_FB] == FT_UNSUPPORTED)   g_main.config.fb_enabled = FT_UNSUPPORTED;
    if (states[FEATURE_IB] == FT_UNSUPPORTED)   g_main.config.ib_enabled = FT_UNSUPPORTED;
    if (states[FEATURE_FPS] == FT_UNSUPPORTED)  g_main.config.fps_enabled = FT_UNSUPPORTED;
    if (states[FEATURE_MSAA] == FT_UNSUPPORTED) g_main.config.msaa_enabled = FT_UNSUPPORTED;
}
