#ifndef ASM_H
#define ASM_H

void hcf(void);
void rdmsr(uint32_t msr, uint32_t *low, uint32_t *high);

#endif