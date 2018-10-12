#ifndef _TOOLS_H_
#define _TOOLS_H_

#define REGISTER_LR 14

void vgMakeThumb_T1_MOV(uint8_t reg, uint8_t value, uint8_t out[2]);
void vgMakeThumb2_T2_MOV(uint8_t reg, uint8_t setflags, uint32_t value, uint8_t out[4]);

void vgMakeArm_A1_MOV(uint8_t reg, uint8_t setflags, uint32_t value, uint8_t out[4]);

#endif
