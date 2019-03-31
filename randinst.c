#include"randinst.h"

int rand_i(int *holdrand)
{
	*holdrand = *holdrand * 0x000343FD + 0x00269EC3;
	return(*holdrand >> 0x10) & 0x7FFF;
}

uint32_t randi_u32(int *holdrand)
{
	uint32_t a = rand_i(holdrand);
	uint32_t b = rand_i(holdrand);
	uint32_t c = rand_i(holdrand);
	return a | (b << 15) | (c << 31);
}
