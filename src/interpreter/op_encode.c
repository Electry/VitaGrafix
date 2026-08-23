#include <string.h>

#include "interpreter.h"
#include "parser.h"
#include "op.h"

static bool op_encode_is_integer(const value_t *value) {
    return value->type == DATA_TYPE_SIGNED || value->type == DATA_TYPE_UNSIGNED;
}

static bool op_encode_is_nonnegative_integer(const value_t *value, uint32_t max) {
    return op_encode_is_integer(value)
        && !(value->type == DATA_TYPE_SIGNED && value->data.int32 < 0)
        && value->data.uint32 <= max;
}

static bool op_encode_is_scalar_bits(const value_t *value, uint32_t max) {
    return value->type != DATA_TYPE_RAW && value->data.uint32 <= max;
}

static uint32_t op_encode_ror(uint32_t value, uint8_t amount) {
    return amount == 0 ? value : (value >> amount) | (value << (32 - amount));
}

static uint32_t op_encode_thumb_expand_imm(uint16_t imm12) {
    uint8_t imm8 = imm12 & 0xFF;
    if ((imm12 & 0xC00) == 0) {
        switch ((imm12 >> 8) & 3) {
            case 0:
                return imm8;
            case 1:
                return ((uint32_t)imm8 << 16) | imm8;
            case 2:
                return ((uint32_t)imm8 << 24) | ((uint32_t)imm8 << 8);
            default:
                return ((uint32_t)imm8 << 24) | ((uint32_t)imm8 << 16) | ((uint32_t)imm8 << 8) | imm8;
        }
    }
    return op_encode_ror(0x80 | (imm12 & 0x7F), imm12 >> 7);
}

static uint32_t op_encode_vfp_expand_imm(uint8_t imm8) {
    uint32_t exponent = (imm8 & 0x40) ? 0x1F : 0x20;
    return ((uint32_t)(imm8 & 0x80) << 24) | (exponent << 25) | ((uint32_t)(imm8 & 0x3F) << 19);
}

static bool op_encode_find_arm_imm(uint32_t value, uint16_t *encoded_imm12) {
    for (uint8_t rotate = 0; rotate < 16; rotate++) {
        uint8_t amount = rotate * 2;
        uint32_t imm8 = op_encode_ror(value, (32 - amount) & 31);
        if (imm8 <= UINT8_MAX) {
            *encoded_imm12 = (rotate << 8) | imm8;
            return true;
        }
    }
    return false;
}

static bool op_encode_find_thumb_imm(uint32_t value, uint16_t *encoded_imm12) {
    for (uint16_t candidate = 0; candidate < 0x1000; candidate++) {
        if (op_encode_thumb_expand_imm(candidate) == value) {
            *encoded_imm12 = candidate;
            return true;
        }
    }
    return false;
}

static bool op_encode_find_vfp_imm(uint32_t value, uint8_t *encoded_imm8) {
    for (uint16_t candidate = 0; candidate < 0x100; candidate++) {
        if (op_encode_vfp_expand_imm(candidate) == value) {
            *encoded_imm8 = candidate;
            return true;
        }
    }
    return false;
}

static uint16_t op_encode_approx_thumb_imm(uint32_t value) {
    if (value < 256)
        return value;

    uint32_t tmp = value;
    uint8_t msb = 0;
    while (tmp != 1 && ++msb) {
        tmp >>= 1;
    }

    uint16_t imm12 = (value >> (msb - 7)) & 0x7F;
    return imm12 | ((32 + 7 - msb) << 7);
}

static uint16_t op_encode_approx_arm_imm(uint32_t value) {
    if (value < 256)
        return value;

    uint32_t tmp = value;
    uint8_t msb = 0;
    while (tmp != 1 && ++msb) {
        tmp >>= 1;
    }

    uint8_t rotation = (msb - 7) + ((msb - 7) % 2);
    return (value >> rotation) | (((32 - rotation) / 2) << 8);
}

bool op_encode_t2_imm(value_t *out) {
    if (!op_encode_is_scalar_bits(out, UINT32_MAX))
        return false;

    out->data.uint32 = op_encode_thumb_expand_imm(op_encode_approx_thumb_imm(out->data.uint32));
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    return true;
}

bool op_encode_a1_imm(value_t *out) {
    if (!op_encode_is_scalar_bits(out, UINT32_MAX))
        return false;

    uint16_t imm12 = op_encode_approx_arm_imm(out->data.uint32);
    out->data.uint32 = op_encode_ror(imm12 & 0xFF, ((imm12 >> 8) & 0xF) * 2);
    out->type = DATA_TYPE_UNSIGNED;
    out->size = 4;
    return true;
}

/*
 * T1 MOVS <Rd>,#<imm8>    # Outside IT block.
 *    MOV<c> <Rd>,#<imm8>  # Inside IT block.
 *
 * 001            00     xxx xxxxxxxx
 * Move immediate OPcode Rdn imm8
 *
 * byte 1   byte 0
 * 00100xxx xxxxxxxx
 */
bool op_encode_t1_mov(value_t *out, value_t *value) {
    if (!op_encode_is_nonnegative_integer(out, 7) || !op_encode_is_scalar_bits(value, UINT8_MAX))
        return false;

    uint8_t reg = (uint8_t)out->data.uint32;
    memset(out->data.raw, 0, 2);
    value_raw(out, 2);

    out->data.raw[1] |= 0b00100000; // Move immediate
    out->data.raw[1] |= reg;        // Rd
    out->data.raw[0] |= (uint8_t)value->data.uint32;
    return true;
}

/*
 * T2 MOV{S}<c>.W <Rd>,#<const>
 *
 * 11110           x 0      0010   x 1111 - 0  xxx  xxxx xxxxxxxx
 * Data processing i 12-bit OPcode S Rn     DP imm3 Rd   imm8
 *
 * byte 1   byte 0     byte 3   byte 2
 * 11110x00 010x1111 - 0xxxxxxx xxxxxxxx
 */
bool op_encode_t2_mov(value_t *out, value_t *reg, value_t *value) {
    if (!op_encode_is_nonnegative_integer(out, 1)
            || !op_encode_is_nonnegative_integer(reg, 15)
            || (out->data.uint32 && reg->data.uint32 == 15)
            || !op_encode_is_scalar_bits(value, UINT32_MAX))
        return false;

    bool setflags = out->data.uint32 != 0;
    uint16_t encoded_imm12;
    if (!op_encode_find_thumb_imm(value->data.uint32, &encoded_imm12))
        return false;

    memset(out->data.raw, 0, 4);
    value_raw(out, 4);


    out->data.raw[1] |= 0b11110000;       // Data processing (12-bit)
    out->data.raw[0] |= 0b01000000;       // OPcode
    out->data.raw[0] |= 0b00001111;       // Rn
    if (setflags)
        out->data.raw[0] |= 0b00010000;   // S
    out->data.raw[3] |= reg->data.uint32; // Rd

    out->data.raw[1] |= (encoded_imm12 & 0b100000000000) >> 9; // i
    out->data.raw[3] |= (encoded_imm12 & 0b011100000000) >> 4; // imm3
    out->data.raw[2] |= (encoded_imm12 & 0b000011111111);      // imm8
    return true;
}

/*
 * T3 MOVW<c> <Rd>,#<imm16>
 *
 * 11110           x 10     0  1    00  xxxx - 0  xxx  xxxx xxxxxxxx
 * Data processing i 16-bit OP Move OP2 imm4   DP imm3 Rd   imm8
 *
 * byte 1   byte 0     byte 3   byte 2
 * 11110x10 0100xxxx - 0xxxxxxx xxxxxxxx
 */
bool op_encode_t3_mov(value_t *out, value_t *value) {
    if (!op_encode_is_nonnegative_integer(out, 14) || !op_encode_is_scalar_bits(value, UINT16_MAX))
        return false;

    uint8_t reg = (uint8_t)out->data.uint32;
    memset(out->data.raw, 0, 4);
    value_raw(out, 4);

    out->data.raw[1] |= 0b11110010; // Data processing (16-bit)
    out->data.raw[0] |= 0b01000000; // Move, plain (16-bit)
    out->data.raw[3] |= reg;        // Rd

    out->data.raw[0] |= (value->data.uint32 & 0b1111000000000000) >> 12; // imm4
    out->data.raw[1] |= (value->data.uint32 & 0b0000100000000000) >> 9;  // i
    out->data.raw[3] |= (value->data.uint32 & 0b0000011100000000) >> 4;  // imm3
    out->data.raw[2] |= (value->data.uint32 & 0b0000000011111111);       // imm8
    return true;
}

/*
 * T1 MOVT<c> <Rd>,#<imm16>
 *
 * 11110           x 10     1  1    00  xxxx - 0  xxx  xxxx xxxxxxxx
 * Data processing i 16-bit OP Move OP2 imm4   DP imm3 Rd   imm8
 *
 * byte 1   byte 0     byte 3   byte 2
 * 11110x10 1100xxxx - 0xxxxxxx xxxxxxxx
 */
bool op_encode_t1_movt(value_t *out, value_t *value) {
    if (!op_encode_is_nonnegative_integer(out, 14) || !op_encode_is_scalar_bits(value, UINT16_MAX))
        return false;

    uint8_t reg = (uint8_t)out->data.uint32;
    memset(out->data.raw, 0, 4);
    value_raw(out, 4);

    out->data.raw[1] |= 0b11110010; // Data processing (16-bit)
    out->data.raw[0] |= 0b11000000; // Move top, plain (16-bit)
    out->data.raw[3] |= reg;        // Rd

    out->data.raw[0] |= (value->data.uint32 & 0b1111000000000000) >> 12; // imm4
    out->data.raw[1] |= (value->data.uint32 & 0b0000100000000000) >> 9;  // i
    out->data.raw[3] |= (value->data.uint32 & 0b0000011100000000) >> 4;  // imm3
    out->data.raw[2] |= (value->data.uint32 & 0b0000000011111111);       // imm8
    return true;
}

/*
 * T2 VMOV<c>.F32 <Sd>, #<imm>
 *
 * 1110      11101  x 11 xxxx  xxxx 101 0  0000 xxxx
 * Condition OPcode D    imm4H Vd       sz      imm4L
 *
 * byte 1   byte 0   byte 3   byte 2
 * 11101110 1x11xxxx xxxx1010 0000xxxx
 */
bool op_encode_t2_vmov_f32(value_t *out, value_t *value) {
    if (!op_encode_is_nonnegative_integer(out, 31) || !op_encode_is_scalar_bits(value, UINT32_MAX))
        return false;

    uint8_t reg = (uint8_t)out->data.uint32;
    uint8_t encoded_imm8;
    if (!op_encode_find_vfp_imm(value->data.uint32, &encoded_imm8))
        return false;

    memset(out->data.raw, 0, 4);
    value_raw(out, 4);

    out->data.raw[1] |= 0b11101110; // Condition + OP
    out->data.raw[0] |= 0b10110000; // OP
    out->data.raw[3] |= 0b00001010; // OP

    // Vd:D
    if (reg & 0b1)
        out->data.raw[0] |= 0b01000000;       // D
    out->data.raw[3] |= (reg & 0b11110) << 3; // Vd

    // imm4H:imm4L
    out->data.raw[0] |= (encoded_imm8 & 0b11110000) >> 4; // imm4H
    out->data.raw[2] |= (encoded_imm8 & 0b00001111);      // imm4L
    return true;
}

/*
 * A1 MOV{S}<c> <Rd>,#<const>
 *
 * 1110      00 1         1101   x 0000 xxxx xxxxxxxxxxxx
 * Condition DP Immediate OPcode S Rn   Rd   imm12
 *
 * byte 3   byte 2   byte 1   byte 0
 * 11100011 101x0000 xxxxxxxx xxxx xxxx
 */
bool op_encode_a1_mov(value_t *out, value_t *reg, value_t *value) {
    if (!op_encode_is_nonnegative_integer(out, 1)
            || !op_encode_is_nonnegative_integer(reg, 15)
            || (out->data.uint32 && reg->data.uint32 == 15)
            || !op_encode_is_scalar_bits(value, UINT32_MAX))
        return false;

    bool setflags = out->data.uint32 != 0;
    uint16_t encoded_imm12;
    if (!op_encode_find_arm_imm(value->data.uint32, &encoded_imm12))
        return false;

    memset(out->data.raw, 0, 4);
    value_raw(out, 4);

    out->data.raw[3] |= 0b11100000;     // Condition
    out->data.raw[3] |= 0b00000010;     // Immediate value
    out->data.raw[3] |= 0b00000001;     // OPcode
    out->data.raw[2] |= 0b10100000;     // OPcode
    if (setflags)
        out->data.raw[2] |= 0b00010000; // S
    out->data.raw[1] |= reg->data.uint32 << 4; // Rd

    out->data.raw[1] |= (encoded_imm12 & 0b111100000000) >> 8;
    out->data.raw[0] |= (encoded_imm12 & 0b000011111111);
    return true;
}

/*
 * A2 MOVW<c> <Rd>,#<imm16>
 *
 * 1110      00110000 xxxx xxxx xxxxxxxxxxxx
 * Condition OPcode   imm4 Rd   imm12
 *
 * byte 3   byte 2   byte 1   byte 0
 * 11100011 0000xxxx xxxxxxxx xxxxxxxx
 */
bool op_encode_a2_mov(value_t *out, value_t *value) {
    if (!op_encode_is_nonnegative_integer(out, 14) || !op_encode_is_scalar_bits(value, UINT16_MAX))
        return false;

    uint8_t reg = (uint8_t)out->data.uint32;
    memset(out->data.raw, 0, 4);
    value_raw(out, 4);

    out->data.raw[3] |= 0b11100000; // Condition
    out->data.raw[3] |= 0b00000010; // Immediate value
    out->data.raw[3] |= 0b00000001; // OPcode
    out->data.raw[1] |= reg << 4;   // Rd

    out->data.raw[2] |= (value->data.uint32 & 0b1111000000000000) >> 12; // imm4
    out->data.raw[1] |= (value->data.uint32 & 0b0000111100000000) >> 8;  // imm12
    out->data.raw[0] |= (value->data.uint32 & 0b0000000011111111);
    return true;
}

bool op_encode_bkpt(value_t *out) {
    value_raw(out, 2);
    out->data.raw[0] = 0x00;
    out->data.raw[1] = 0xBE;
    return true;
}

bool op_encode_nop(value_t *out) {
    value_raw(out, 2);
    out->data.raw[0] = 0x00;
    out->data.raw[1] = 0xBF;
    return true;
}

bool op_encode_unk(value_t *out) {
    uint32_t size = out->data.uint32;
    if ((out->type != DATA_TYPE_SIGNED && out->type != DATA_TYPE_UNSIGNED) || size == 0 || size > MAX_VALUE_SIZE)
        return false;

    value_raw(out, size);
    memset(out->data.raw, 0, size * sizeof(byte_t));
    memset(out->unk, 1, size * sizeof(bool));
    return true;
}

bool op_encode_mov32(value_t *out, value_t *value, value_t *gap) {
    if (!op_encode_is_nonnegative_integer(out, 14) || !op_encode_is_scalar_bits(value, UINT32_MAX))
        return false;

    value_t reg, bottom, top;
    memcpy(&reg, out, sizeof(value_t));
    memcpy(&bottom, value, sizeof(value_t));
    memcpy(&top, value, sizeof(value_t));
    bool ret;

    // Encode MOVW
    bottom.type = DATA_TYPE_UNSIGNED;
    bottom.data.uint32 &= UINT16_MAX;
    ret = op_encode_t3_mov(out, &bottom);
    if (!ret) return ret;

    // Encode MOVT
    top.type = DATA_TYPE_UNSIGNED;
    top.data.uint32 >>= 16; // shift
    ret = op_encode_t1_movt(&reg, &top);
    if (!ret) return ret;

    // Encode byte gap
    if (gap->data.uint32 > 0) {
        ret = op_encode_unk(gap);
        if (!ret) return ret;

        ret = op_datatype_raw_concat(out, gap);
        if (!ret) return ret;
    }

    return op_datatype_raw_concat(out, &reg);
}
