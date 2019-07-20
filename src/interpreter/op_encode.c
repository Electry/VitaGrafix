#include <string.h>

#include "interpreter.h"
#include "parser.h"
#include "op.h"

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
    uint8_t reg = (uint8_t)out->data.uint32;
    memset(out->data.raw, 0, 2);
    value_raw(out, 2);

    out->data.raw[1] |= 0b00100000;   // Move immediate
    out->data.raw[1] |= reg;          // Rd
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
    bool setflags = out->data.uint32 > 0;
    memset(out->data.raw, 0, 4);
    value_raw(out, 4);


    out->data.raw[1] |= 0b11110000;       // Data processing (12-bit)
    out->data.raw[0] |= 0b01000000;       // OPcode
    out->data.raw[0] |= 0b00001111;       // Rn
    if (setflags)
        out->data.raw[0] |= 0b00010000;   // S
    out->data.raw[3] |= reg->data.uint32; // Rd

    uint16_t imm12 = 0;

    if (value->data.uint32 < 256) {
        imm12 = value->data.uint32;
    } else {
        uint32_t tmp = value->data.uint32;
        uint8_t msb = 0;
        while (tmp != 1 && ++msb) {
            tmp = tmp >> 1;
        }
        imm12 = (value->data.uint32 >> (msb - 7)) & 0b000001111111; // rotated value as bit[6:0]
        imm12 |= (32 + 7 - msb) << 7; // rotation as bit[11:7]
    }

    out->data.raw[1] |= (imm12 & 0b100000000000) >> 9;    // i
    out->data.raw[3] |= (imm12 & 0b011100000000) >> 4;    // imm3
    out->data.raw[2] |= (imm12 & 0b000011111111);         // imm8
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
    uint8_t reg = (uint8_t)out->data.uint32;
    memset(out->data.raw, 0, 4);
    value_raw(out, 4);

    out->data.raw[1] |= 0b11110010; // Data processing (16-bit)
    out->data.raw[0] |= 0b01000000; // Move, plain (16-bit)
    out->data.raw[3] |= reg;        // Rd

    out->data.raw[0] |= (value->data.uint32 & 0b1111000000000000) >> 12;   // imm4
    out->data.raw[1] |= (value->data.uint32 & 0b0000100000000000) >> 9;    // i
    out->data.raw[3] |= (value->data.uint32 & 0b0000011100000000) >> 4;    // imm3
    out->data.raw[2] |= (value->data.uint32 & 0b0000000011111111);         // imm8
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
    uint8_t reg = (uint8_t)out->data.uint32;
    memset(out->data.raw, 0, 4);
    value_raw(out, 4);

    out->data.raw[1] |= 0b11110010; // Data processing (16-bit)
    out->data.raw[0] |= 0b11000000; // Move top, plain (16-bit)
    out->data.raw[3] |= reg;        // Rd

    out->data.raw[0] |= (value->data.uint32 & 0b1111000000000000) >> 12;   // imm4
    out->data.raw[1] |= (value->data.uint32 & 0b0000100000000000) >> 9;    // i
    out->data.raw[3] |= (value->data.uint32 & 0b0000011100000000) >> 4;    // imm3
    out->data.raw[2] |= (value->data.uint32 & 0b0000000011111111);         // imm8
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
    uint8_t reg = (uint8_t)out->data.uint32;
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
    uint8_t imm8 = 0;
    if (value->data.uint32 & (1 << 31))
        imm8 |= 1 << 7; // sign bit
    if (((value->data.uint32 >> 25) & 0b111111) < 0b100000)
        imm8 |= 1 << 6; // < 2.0
    imm8 |= (value->data.uint32 >> 19) & 0b111111;

    out->data.raw[0] |= (imm8 & 0b11110000) >> 4; // imm4H
    out->data.raw[2] |= (imm8 & 0b00001111);      // imm4L
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
    bool setflags = out->data.uint32 > 0;
    memset(out->data.raw, 0, 4);
    value_raw(out, 4);

    out->data.raw[3] |= 0b11100000;       // Condition
    out->data.raw[3] |= 0b00000010;       // Immediate value
    out->data.raw[3] |= 0b00000001;       // OPcode
    out->data.raw[2] |= 0b10100000;       // OPcode
    if (setflags)
        out->data.raw[2] |= 0b00010000;   // S
    out->data.raw[1] |= reg->data.uint32 << 4; // Rd

    uint16_t imm12 = 0;

    if (value->data.uint32 < 256) {
        imm12 = value->data.uint32;
    } else {
        uint32_t tmp = value->data.uint32;
        uint8_t msb = 0;
        while (tmp != 1 && ++msb) {
            tmp = tmp >> 1;
        }
        uint8_t rotation = (msb - 7) + ((msb - 7) % 2); // must be even
        imm12 |= (value->data.uint32 >> rotation);        // rotated value as bit[7:0]
        imm12 |= ((32 - rotation) / 2) << 8; // rotation / 2 as bit[11:8]
    }

    out->data.raw[1] |= (imm12 & 0b111100000000) >> 8;
    out->data.raw[0] |= (imm12 & 0b000011111111);
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
    uint8_t reg = (uint8_t)out->data.uint32;
    memset(out->data.raw, 0, 4);
    value_raw(out, 4);

    out->data.raw[3] |= 0b11100000;       // Condition
    out->data.raw[3] |= 0b00000010;       // Immediate value
    out->data.raw[3] |= 0b00000001;       // OPcode
    out->data.raw[1] |= reg << 4;         // Rd

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
