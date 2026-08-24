#include <vitasdk.h>
#include <taihen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "config.h"
#include "main.h"
#include "log.h"

static vg_config_section_t g_config_section = CONFIG_SECTION_NONE;
static vg_config_t g_config = {0};
static vg_io_status_t g_config_status = {0};

#define CONFIG_PATH_SIZE 128

const vg_res_t vg_config_framebuffer_resolutions[FRAMEBUFFER_RESOLUTION_COUNT] = {
    {960, 544},
    {720, 408},
    {640, 368},
    {480, 272},
};

#define OPTIONS_TOTAL 7
static const vg_config_parse_option_t _OPTIONS[OPTIONS_TOTAL] = {
    {"ENABLED", CONFIG_OPTION_FEATURE_STATE,              &g_config.enabled,      {NULL},                   NULL},
    {"OSD",     CONFIG_OPTION_FEATURE_STATE,              &g_config.osd_enabled,  {NULL},                   NULL},
    {"LOG",     CONFIG_OPTION_FEATURE_STATE,              &g_config.log_enabled,  {NULL},                   NULL},
    {"FB",      CONFIG_OPTION_FRAMEBUFFER_RESOLUTION,     &g_config.fb_enabled,   {(void *)&g_config.fb},   NULL},
    {"IB",      CONFIG_OPTION_INTERNAL_BUFFER_RESOLUTION, &g_config.ib_enabled,   {(void *)g_config.ib},    &g_config.ib_count},
    {"FPS",     CONFIG_OPTION_FRAMERATE,                  &g_config.fps_enabled,  {(void *)&g_config.fps},  NULL},
    {"MSAA",    CONFIG_OPTION_MSAA,                       &g_config.msaa_enabled, {(void *)&g_config.msaa}, NULL},
};

static bool vg_config_token_matches(const char line[], int pos, const char token[]) {
    size_t token_length = strlen(token);
    return !strncasecmp(&line[pos], token, token_length) && vg_io_is_line_end(line, pos + token_length);
}

static vg_io_status_t vg_config_parse_feature_state(const char line[], int pos, vg_feature_state_t *out) {
    while (isspace(line[pos])) { pos++; }

    // Enabled
    if (vg_config_token_matches(line, pos, "1")
            || vg_config_token_matches(line, pos, "on")
            || vg_config_token_matches(line, pos, "true")) {
        *out = FT_ENABLED;
        __ret_status(IO_OK, 0, 0);
    }
    // Disabled
    if (vg_config_token_matches(line, pos, "0")
            || vg_config_token_matches(line, pos, "off")
            || vg_config_token_matches(line, pos, "false")) {
        *out = FT_DISABLED;
        __ret_status(IO_OK, 0, 0);
    }

    __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
}

static vg_io_status_t vg_config_parse_dimension(const char line[], int *pos, uint16_t *out) {
    if (!isdigit(line[*pos]))
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, *pos);

    char *end = NULL;
    unsigned long value = strtoul(&line[*pos], &end, 10);
    if (value == 0 || value > UINT16_MAX)
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, *pos);

    *out = value;
    *pos = end - line;
    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_config_parse_resolution_value(const char line[], int *pos, vg_res_t *res) {
    int resolution_pos = *pos;
    vg_io_status_t ret = vg_config_parse_dimension(line, pos, &res->width);
    if (ret.code != IO_OK)
        return ret;

    while (isspace(line[*pos])) { (*pos)++; }
    if (line[*pos] != 'x' && line[*pos] != 'X')
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, *pos);
    (*pos)++;

    while (isspace(line[*pos])) { (*pos)++; }
    ret = vg_config_parse_dimension(line, pos, &res->height);
    if (ret.code != IO_OK)
        return ret;

    vg_res_t original = *res;
    res->width &= ~3u;
    res->height &= ~3u;
    if (res->width == 0 || res->height == 0)
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, resolution_pos);
    if (original.width != res->width || original.height != res->height) {
        vg_log_printf("[CONFIG] Resolution %dx%d aligned down to %dx%d\n",
                original.width, original.height, res->width, res->height);
    }

    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_config_parse_framebuffer_resolution(const char line[], int pos,
        vg_feature_state_t *ft, vg_res_t *res) {
    *ft = FT_ENABLED;
    while (isspace(line[pos])) { pos++; }

    // Disabled
    if (vg_config_token_matches(line, pos, "0")
            || vg_config_token_matches(line, pos, "off")
            || vg_config_token_matches(line, pos, "false")) {
        *ft = FT_DISABLED;
        __ret_status(IO_OK, 0, 0);
    }

    vg_io_status_t ret = vg_config_parse_resolution_value(line, &pos, res);
    if (ret.code != IO_OK)
        return ret;

    bool is_valid = false;
    for (int i = 0; i < FRAMEBUFFER_RESOLUTION_COUNT; i++) {
        if (res->width == vg_config_framebuffer_resolutions[i].width
                && res->height == vg_config_framebuffer_resolutions[i].height) {
            is_valid = true;
            break;
        }
    }
    if (!is_valid)
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
    if (!vg_io_is_line_end(line, pos))
        __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);

    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_config_parse_internal_buffer_resolution(const char line[], int pos,
        vg_feature_state_t *ft, vg_res_t *res, uint8_t *count) {
    *ft = FT_ENABLED;
    while (isspace(line[pos])) { pos++; }

    if (vg_config_token_matches(line, pos, "0")
            || vg_config_token_matches(line, pos, "off")
            || vg_config_token_matches(line, pos, "false")) {
        *ft = FT_DISABLED;
        __ret_status(IO_OK, 0, 0);
    }

    *count = 0;
    while (true) {
        if (*count >= MAX_RES_COUNT)
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);

        vg_io_status_t ret = vg_config_parse_resolution_value(line, &pos, &res[*count]);
        if (ret.code != IO_OK)
            return ret;
        (*count)++;

        while (isspace(line[pos])) { pos++; }
        if (vg_io_is_line_end(line, pos))
            break;
        if (line[pos] != ',')
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
        pos++;
        while (isspace(line[pos])) { pos++; }
        if (vg_io_is_line_end(line, pos))
            __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
    }

    __ret_status(IO_OK, 0, 0);
}

static vg_io_status_t vg_config_parse_framerate(const char line[], int pos, vg_feature_state_t *ft, vg_fps_t *fps) {
    *ft = FT_ENABLED;
    while (isspace(line[pos])) { pos++; }

    // Disabled
    if (vg_config_token_matches(line, pos, "0")
            || vg_config_token_matches(line, pos, "off")
            || vg_config_token_matches(line, pos, "false")) {
        *ft = FT_DISABLED;
        __ret_status(IO_OK, 0, 0);
    }

    if (vg_config_token_matches(line, pos, "60")) {
        *fps = FPS_60;
        __ret_status(IO_OK, 0, 0);
    }
    if (vg_config_token_matches(line, pos, "30")) {
        *fps = FPS_30;
        __ret_status(IO_OK, 0, 0);
    }
    if (vg_config_token_matches(line, pos, "20")) {
        *fps = FPS_20;
        __ret_status(IO_OK, 0, 0);
    }

    __ret_status(IO_ERROR_PARSE_INVALID_TOKEN, 0, pos);
}

static vg_io_status_t vg_config_parse_msaa(const char line[], int pos, vg_feature_state_t *ft, vg_msaa_t *msaa) {
    *ft = FT_ENABLED;
    while (isspace(line[pos])) { pos++; }

    // Disabled
    if (vg_config_token_matches(line, pos, "0")
            || vg_config_token_matches(line, pos, "off")
            || vg_config_token_matches(line, pos, "false")) {
        *ft = FT_DISABLED;
        __ret_status(IO_OK, 0, 0);
    }

    if (vg_config_token_matches(line, pos, "4")) {
        *msaa = MSAA_4X;
        __ret_status(IO_OK, 0, 0);
    }
    if (vg_config_token_matches(line, pos, "2")) {
        *msaa = MSAA_2X;
        __ret_status(IO_OK, 0, 0);
    }
    if (vg_config_token_matches(line, pos, "1")) {
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
            if (g_config_section == CONFIG_SECTION_MAIN && *(_OPTIONS[i].ft_state) != FT_UNSPECIFIED) {
                __ret_status(IO_OK, 0, 0);
            }

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
                case CONFIG_OPTION_FRAMEBUFFER_RESOLUTION:
                    return vg_config_parse_framebuffer_resolution(line, pos_rhs,
                            _OPTIONS[i].ft_state, _OPTIONS[i].res);
                case CONFIG_OPTION_INTERNAL_BUFFER_RESOLUTION:
                    return vg_config_parse_internal_buffer_resolution(line, pos_rhs,
                            _OPTIONS[i].ft_state, _OPTIONS[i].res, _OPTIONS[i].count);
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
        if (!strncasecmp(line, "[MAIN]", 6) && vg_io_is_line_end(line, 6)) {
            g_config_section = CONFIG_SECTION_MAIN;
            return ret;
        }

        // [TITLEID,SELF,NID]
        vg_io_section_header_t header;
        ret = vg_io_parse_section_header(line, &header);
        if (ret.code != IO_OK) {
            return ret;
        }

        g_config_section = vg_main_match_current_module(header.titleid, header.self, header.nid, false) == MODULE_MATCH
            ? CONFIG_SECTION_GAME : CONFIG_SECTION_NONE;
        return ret;
    }

    // Parse option
    if (g_config_section != CONFIG_SECTION_NONE) {
        return vg_config_parse_option(line);
    }

    return ret;
}

void vg_config_set_unspecified_to_defaults() {
    if (g_config.enabled == FT_UNSPECIFIED) {
        g_config.enabled = FT_ENABLED;
    }

    if (g_config.osd_enabled == FT_UNSPECIFIED) {
        g_config.osd_enabled = FT_ENABLED;
    }

    if (g_config.log_enabled == FT_UNSPECIFIED) {
        g_config.log_enabled = FT_ENABLED;
    }

    if (g_config.fb_enabled == FT_UNSPECIFIED) {
        g_config.fb_enabled = FT_DISABLED;
        g_config.fb.width = 960;
        g_config.fb.height = 544;
    }

    if (g_config.ib_enabled == FT_UNSPECIFIED) {
        g_config.ib_enabled = FT_DISABLED;
        g_config.ib[0].width = 960;
        g_config.ib[0].height = 544;
        g_config.ib_count = 1;
    }

    if (g_config.fps_enabled == FT_UNSPECIFIED) {
        g_config.fps_enabled = FT_DISABLED;
        g_config.fps = FPS_60;
    }

    if (g_config.msaa_enabled == FT_UNSPECIFIED) {
        g_config.msaa_enabled = FT_DISABLED;
        g_config.msaa = MSAA_4X;
    }
}

void vg_config_propagate_ib() {
    if (g_config.ib_count == 0) {
        return;
    }
    for (uint8_t i = g_config.ib_count; i < MAX_RES_COUNT; i++) {
        g_config.ib[i].width = g_config.ib[i - 1].width;
        g_config.ib[i].height = g_config.ib[i - 1].height;
    }
}

static bool vg_config_write_section_separator(SceUID fd, bool output_has_data, char last_output_char) {
    if (!output_has_data) {
        return true;
    }

    const char *separator = last_output_char == '\n' ? "\n" : "\n\n";
    return sceIoWrite(fd, separator, strlen(separator)) == (int)strlen(separator);
}

static bool vg_config_buffer_append(char buffer[], size_t size, int *length, const char format[], ...) {
    if (*length < 0 || (size_t)*length >= size) {
        return false;
    }

    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer + *length, size - *length, format, args);
    va_end(args);
    if (written < 0 || written >= (int)(size - *length)) {
        return false;
    }

    *length += written;
    return true;
}

static bool vg_config_format_current_title_section(char buffer[], size_t size, int *length) {
    *length = 0;
    const char *off = "off";

    if (!vg_config_buffer_append(buffer, size, length, "[%s,%s,0x%X]\n",
            g_main.titleid, vg_main_get_self_filename(), g_main.tai_info.module_nid)) {
        return false;
    }

    if (g_config.fb_enabled != FT_UNSUPPORTED) {
        if (g_config.fb_enabled == FT_ENABLED) {
            if (!vg_config_buffer_append(buffer, size, length, "FB=%dx%d\n", g_config.fb.width, g_config.fb.height)) {
                return false;
            }
        } else if (!vg_config_buffer_append(buffer, size, length, "FB=%s\n", off)) {
            return false;
        }
    }

    if (g_config.ib_enabled != FT_UNSUPPORTED) {
        if (g_config.ib_enabled == FT_ENABLED && g_config.ib_count > 0) {
            if (!vg_config_buffer_append(buffer, size, length, "IB=")) {
                return false;
            }
            for (uint8_t i = 0; i < g_config.ib_count; i++) {
                if (!vg_config_buffer_append(buffer, size, length, "%s%dx%d",
                        i > 0 ? "," : "", g_config.ib[i].width, g_config.ib[i].height)) {
                    return false;
                }
            }
            if (!vg_config_buffer_append(buffer, size, length, "\n")) {
                return false;
            }
        } else if (!vg_config_buffer_append(buffer, size, length, "IB=%s\n", off)) {
            return false;
        }
    }

    if (g_config.fps_enabled != FT_UNSUPPORTED) {
        if (g_config.fps_enabled == FT_ENABLED) {
            if (!vg_config_buffer_append(buffer, size, length, "FPS=%s\n",
                    g_config.fps == FPS_20 ? "20" : g_config.fps == FPS_30 ? "30" : "60")) {
                return false;
            }
        } else if (!vg_config_buffer_append(buffer, size, length, "FPS=%s\n", off)) {
            return false;
        }
    }

    if (g_config.msaa_enabled != FT_UNSUPPORTED) {
        if (g_config.msaa_enabled == FT_ENABLED) {
            if (!vg_config_buffer_append(buffer, size, length, "MSAA=%s\n",
                    g_config.msaa == MSAA_NONE ? "1" : g_config.msaa == MSAA_2X ? "2" : "4")) {
                return false;
            }
        } else if (!vg_config_buffer_append(buffer, size, length, "MSAA=%s\n", off)) {
            return false;
        }
    }

    return vg_config_buffer_append(buffer, size, length, "\n");
}

static bool vg_config_replace_title_section(const char path[], const char section[], int section_length) {
    char temp_path[CONFIG_PATH_SIZE + sizeof(".tmp")];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
    sceIoRemove(temp_path);

    SceUID input = -1;
    SceUID output = sceIoOpen(temp_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (output < 0) {
        goto SAVE_FAILURE;
    }

    input = sceIoOpen(path, SCE_O_RDONLY, 0777);
    bool had_input = input >= 0;
    bool section_written = false;
    bool skip_section = false;
    bool output_has_data = false;
    char last_output_char = '\0';
    char line[IO_CHUNK_SIZE + 1];
    int line_length = 0;

    if (input >= 0) {
        char c;
        while (true) {
            int read = sceIoRead(input, &c, 1);
            if (read == 1) {
                if (line_length >= IO_CHUNK_SIZE) {
                    goto SAVE_FAILURE;
                }
                line[line_length++] = c;
            } else if (read < 0) {
                goto SAVE_FAILURE;
            } else if (line_length == 0) {
                break;
            }

            if (read == 1 && c != '\n') {
                continue;
            }

            line[line_length] = '\0';

            int pos = 0;
            while (isspace(line[pos])) { pos++; }
            if (line[pos] == '[') {
                skip_section = false;

                vg_io_section_header_t header;
                if (vg_io_parse_section_header(&line[pos], &header).code != IO_OK) {
                    goto SAVE_FAILURE;
                }

                skip_section = vg_main_match_current_module(header.titleid, header.self, header.nid, true)
                    == MODULE_MATCH;

                if (skip_section && !section_written) {
                    if (!vg_config_write_section_separator(output, output_has_data, last_output_char)
                            || sceIoWrite(output, section, section_length) != section_length) {
                        goto SAVE_FAILURE;
                    }

                    section_written = true;
                    output_has_data = true;
                    last_output_char = section[section_length - 1];
                }
            }

            if (!skip_section) {
                if (sceIoWrite(output, line, line_length) != line_length) {
                    goto SAVE_FAILURE;
                }

                output_has_data = true;
                last_output_char = line[line_length - 1];
            }

            line_length = 0;
            if (read != 1) {
                break;
            }
        }

        sceIoClose(input);
        input = -1;
    }

    if (!section_written && (!vg_config_write_section_separator(output, output_has_data, last_output_char)
            || sceIoWrite(output, section, section_length) != section_length)) {
        goto SAVE_FAILURE;
    }

    if (sceIoClose(output) < 0) {
        output = -1;
        goto SAVE_FAILURE;
    }

    output = -1;
    if ((had_input && sceIoRemove(path) < 0) || sceIoRename(temp_path, path) < 0) {
        goto SAVE_FAILURE;
    }

    return true;

SAVE_FAILURE:
    if (input >= 0) {
        sceIoClose(input);
    }
    if (output >= 0) {
        sceIoClose(output);
    }
    sceIoRemove(temp_path);
    return false;
}

bool vg_config_save_current_title_override() {
    char section[512];
    int section_length;
    if (!vg_config_format_current_title_section(section, sizeof(section), &section_length)) {
        return false;
    }

    sceIoMkdir(CONFIG_DIR, 0777);

    char path[CONFIG_PATH_SIZE];
    snprintf(path, sizeof(path), "%s%s.txt", CONFIG_DIR, g_main.titleid);
    return vg_config_replace_title_section(path, section, section_length);
}

vg_io_status_t vg_config_parse() {
    // Reset
    g_config.enabled      = FT_UNSPECIFIED;
    g_config.osd_enabled  = FT_UNSPECIFIED;
    g_config.log_enabled  = FT_UNSPECIFIED;
    g_config.fb_enabled   = FT_UNSPECIFIED;
    g_config.ib_enabled   = FT_UNSPECIFIED;
    g_config.fps_enabled  = FT_UNSPECIFIED;
    g_config.msaa_enabled = FT_UNSPECIFIED;

    char path[CONFIG_PATH_SIZE];
    snprintf(path, sizeof(path), "%s%s.txt", CONFIG_DIR, g_main.titleid);

    // Prefer a complete title-specific configuration over config.txt
    g_config_section = CONFIG_SECTION_NONE;
    g_config_status = vg_io_parse(path, vg_config_parse_line, false);

    // If does not exist, parse global config.txt
    if (g_config_status.code == IO_ERROR_OPEN_FAILED) {
        g_config_section = CONFIG_SECTION_NONE;
        g_config_status = vg_io_parse(CONFIG_PATH, vg_config_parse_line, true);
    }

    // Set unset options to their default values
    vg_config_set_unspecified_to_defaults();

    // Propagate last specified IB res. (for multires patches)
    vg_config_propagate_ib();

#ifdef ENABLE_VERBOSE_LOGGING
    vg_log_printf("[CONFIG] Config:\n");
    vg_log_printf("[CONFIG] ENABLED: %d\n", g_config.enabled);
    vg_log_printf("[CONFIG] OSD: %d\n", g_config.osd_enabled);
    vg_log_printf("[CONFIG] LOG: %d\n", g_config.log_enabled);
    vg_log_printf("[CONFIG] FB: %d %dx%d\n", g_config.fb_enabled, g_config.fb.width, g_config.fb.height);
    vg_log_printf("[CONFIG] IB: %d #%d 1st:%dx%d\n", g_config.ib_enabled, g_config.ib_count, g_config.ib[0].width, g_config.ib[0].height);
    vg_log_printf("[CONFIG] FPS: %d %d\n", g_config.fps_enabled, g_config.fps);
    vg_log_printf("[CONFIG] MSAA: %d %d\n", g_config.msaa_enabled, g_config.msaa);
#endif

    return g_config_status;
}

bool vg_config_is_feature_enabled(vg_feature_t feature) {
    if (!g_config.enabled) {
        return false;
    }

    switch (feature) {
        case FEATURE_FB:   return g_config.fb_enabled == FT_ENABLED;
        case FEATURE_IB:   return g_config.ib_enabled == FT_ENABLED;
        case FEATURE_FPS:  return g_config.fps_enabled == FT_ENABLED;
        case FEATURE_MSAA: return g_config.msaa_enabled == FT_ENABLED;
        default: return false;
    }

    return false;
}

bool vg_config_is_feature_supported(vg_feature_t feature) {
    if (!g_config.enabled) {
        return false;
    }

    switch (feature) {
        case FEATURE_FB:   return g_config.fb_enabled != FT_UNSUPPORTED;
        case FEATURE_IB:   return g_config.ib_enabled != FT_UNSUPPORTED;
        case FEATURE_FPS:  return g_config.fps_enabled != FT_UNSUPPORTED;
        case FEATURE_MSAA: return g_config.msaa_enabled != FT_UNSUPPORTED;
        default: return false;
    }

    return false;
}

vg_config_t *vg_config_get() {
    return &g_config;
}

const vg_io_status_t *vg_config_get_status() {
    return &g_config_status;
}

void vg_config_apply_patch_capabilities(vg_feature_state_t states[]) {
    if (states[FEATURE_FB] == FT_UNSUPPORTED)   g_config.fb_enabled = FT_UNSUPPORTED;
    if (states[FEATURE_IB] == FT_UNSUPPORTED)   g_config.ib_enabled = FT_UNSUPPORTED;
    if (states[FEATURE_FPS] == FT_UNSUPPORTED)  g_config.fps_enabled = FT_UNSUPPORTED;
    if (states[FEATURE_MSAA] == FT_UNSUPPORTED) g_config.msaa_enabled = FT_UNSUPPORTED;
}
