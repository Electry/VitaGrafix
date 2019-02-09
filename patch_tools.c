#include <vitasdk.h>

#include "patch_tools.h"

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
void vgMakeThumb_T1_MOV(uint8_t reg, uint8_t value, uint8_t out[2]) {
    memset(out, 0, 2);

    out[1] |= 0b00100000;   // Move immediate
    out[1] |= reg;          // Rd
    out[0] |= value;
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
void vgMakeThumb2_T2_MOV(uint8_t reg, uint8_t setflags, uint32_t value, uint8_t out[4]) {
    memset(out, 0, 4);

    out[1] |= 0b11110000;       // Data processing (12-bit)
    out[0] |= 0b01000000;       // OPcode
    out[0] |= 0b00001111;       // Rn
    if (setflags)
        out[0] |= 0b00010000;   // S
    out[3] |= reg;              // Rd

    uint16_t imm12 = 0;

    if (value < 256) {
        imm12 = value;
    } else {
        uint32_t tmp = value;
        uint8_t msb = 0;
        while (tmp != 1 && ++msb) {
            tmp = tmp >> 1;
        }
        imm12 = (value >> (msb - 7)) & 0b000001111111;  // rotated value as bit[6:0]
        imm12 |= (32 + 7 - msb) << 7;                   // rotation as bit[11:7]
    }

    out[1] |= (imm12 & 0b100000000000) >> 9;    // i
    out[3] |= (imm12 & 0b011100000000) >> 4;    // imm3
    out[2] |= (imm12 & 0b000011111111);         // imm8
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
void vgMakeThumb2_T3_MOV(uint8_t reg, uint16_t value, uint8_t out[4]) {
    memset(out, 0, 4);

    out[1] |= 0b11110010; // Data processing (16-bit)
    out[0] |= 0b01000000; // Move, plain (16-bit)
    out[3] |= reg;        // Rd

    out[0] |= (value & 0b1111000000000000) >> 12;   // imm4
    out[1] |= (value & 0b0000100000000000) >> 9;    // i
    out[3] |= (value & 0b0000011100000000) >> 4;    // imm3
    out[2] |= (value & 0b0000000011111111);         // imm8
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
void vgMakeThumb2_T1_MOVT(uint8_t reg, uint16_t value, uint8_t out[4]) {
    memset(out, 0, 4);

    out[1] |= 0b11110010; // Data processing (16-bit)
    out[0] |= 0b11000000; // Move top, plain (16-bit)
    out[3] |= reg;        // Rd

    out[0] |= (value & 0b1111000000000000) >> 12;   // imm4
    out[1] |= (value & 0b0000100000000000) >> 9;    // i
    out[3] |= (value & 0b0000011100000000) >> 4;    // imm3
    out[2] |= (value & 0b0000000011111111);         // imm8
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
void vgMakeArm_A1_MOV(uint8_t reg, uint8_t setflags, uint32_t value, uint8_t out[4]) {
    memset(out, 0, 4);

    out[3] |= 0b11100000;       // Condition
    out[3] |= 0b00000010;       // Immediate value
    out[3] |= 0b00000001;       // OPcode
    out[2] |= 0b10100000;       // OPcode
    if (setflags)
        out[2] |= 0b00010000;   // S
    out[1] |= reg << 4;         // Rd

    uint16_t imm12 = 0;

    if (value < 256) {
        imm12 = value;
    } else {
        uint32_t tmp = value;
        uint8_t msb = 0;
        while (tmp != 1 && ++msb) {
            tmp = tmp >> 1;
        }
        uint8_t rotation = (msb - 7) + ((msb - 7) % 2); // must be even
        imm12 |= (value >> rotation);        // rotated value as bit[7:0]
        imm12 |= ((32 - rotation) / 2) << 8; // rotation / 2 as bit[11:8]
    }

    out[1] |= (imm12 & 0b111100000000) >> 8;
    out[0] |= (imm12 & 0b000011111111);
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
void vgMakeArm_A2_MOV(uint8_t reg, uint16_t value, uint8_t out[4]) {
    memset(out, 0, 4);

    out[3] |= 0b11100000;       // Condition
    out[3] |= 0b00000010;       // Immediate value
    out[3] |= 0b00000001;       // OPcode
    out[1] |= reg << 4;         // Rd

    out[2] |= (value & 0b1111000000000000) >> 12; // imm4
    out[1] |= (value & 0b0000111100000000) >> 8;  // imm12
    out[0] |= (value & 0b0000000011111111);
}
