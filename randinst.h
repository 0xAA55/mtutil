#ifndef _RANDINST_H_
#define _RANDINST_H_ 1

#include<stdint.h>

#define RAND_I_MAX 0x7fff

int rand_i(int *holdrand);
uint32_t randi_u32(int *holdrand);

#endif
