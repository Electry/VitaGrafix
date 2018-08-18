#ifndef _TOOLS_H_
#define _TOOLS_H_

#define REGISTER_LR 14

void make_thumb_t1_mov(uint8_t reg, uint8_t value, uint8_t out[2]);
void make_thumb2_t2_mov(uint8_t reg, uint8_t setflags, uint32_t value, uint8_t out[4]);

void make_arm_a1_mov(uint8_t reg, uint8_t setflags, uint32_t value, uint8_t out[4]);

#endif
