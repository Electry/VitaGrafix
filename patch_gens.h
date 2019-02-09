#ifndef _PATCH_GENS_H_
#define _PATCH_GENS_H_

VG_IoParseState vgPatchParseGenValue(
        const char chunk[], int pos, int end,
        uint32_t *value);

VG_IoParseState vgPatchParseGen(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_uint16(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_uint32(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_fl32(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_bytes(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_nop(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_bkpt(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_a1_mov(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_a2_mov(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_t1_mov(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_t2_mov(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_t3_mov(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

VG_IoParseState vgPatchParseGen_t1_movt(
        const char chunk[], int pos, int end,
        uint8_t patch_data[], uint8_t *patch_data_len);

#endif