#include <vitasdk.h>

#include "tools.h"

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

    out[1] |= 0b00100000;	// Move immediate
    out[1] |= reg;			// Rd
    out[0] |= value;
}

/*
 * T2 MOV{S}<c>.W <Rd>,#<const>
 *
 * 11110           i 0      0010   S 1111 - 0  000  0000 00000000
 * Data processing i 12-bit OPcode S Rn     DP imm3 Rd   imm8
 *
 * byte 1   byte 0     byte 3   byte 2
 * 11110i00 010S1111 - 00000000 00000000
 */
void vgMakeThumb2_T2_MOV(uint8_t reg, uint8_t setflags, uint32_t value, uint8_t out[4]) {
    memset(out, 0, 4);

    out[1] |= 0b11110000;		// Data processing (12-bit)
    out[0] |= 0b01000000;		// OPcode
    out[0] |= 0b00001111;		// Rn
    if (setflags)
        out[0] |= 0b00010000;	// S
    out[3] |= reg;			// Rd

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
 * A1 MOV{S}<c> <Rd>,#<const>
 *
 * 1110      00 1         1101   S 0000 xxxx xxxxxxxxxxxx
 * Condition DP Immediate OPcode s Rn   Rd   imm12
 *
 * byte 3   byte 2   byte 1   byte 0
 * 11100011 101s0000 rrrrxxxx xxxx xxxx
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
