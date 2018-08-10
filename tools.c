#include <psp2/types.h>
#include <libk/string.h>

#include "tools.h"

/*
 * T2 MOV{S}<c>.W <Rd>,#<const>
 *
 * 11110           i 0      0010   S 1111 - 0  000  0000 00000000
 * Data processing i 12-bit OPcode S Rn     DP imm3 Rd   imm8
 *
 * byte 1   byte 0     byte 3   byte 2
 * 11110i00 010S1111 - 00000000 00000000
 */
void make_thumb2_t2_mov(uint8_t reg, uint8_t setflags, uint32_t value, uint8_t out[4]) {
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
