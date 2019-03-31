#pragma once

#include<stdarg.h>

#define TO_ARGS_1(arr, base) arr[(base) + 0]
#define TO_ARGS_2(arr, base)    TO_ARGS_1(arr, (base) + 0),  TO_ARGS_1(arr, (base) +  1)
#define TO_ARGS_4(arr, base)    TO_ARGS_2(arr, (base) + 0),  TO_ARGS_2(arr, (base) +  2)
#define TO_ARGS_8(arr, base)    TO_ARGS_4(arr, (base) + 0),  TO_ARGS_4(arr, (base) +  4)
#define TO_ARGS_16(arr, base)   TO_ARGS_8(arr, (base) + 0),  TO_ARGS_8(arr, (base) +  8)
#define TO_ARGS_32(arr, base)  TO_ARGS_16(arr, (base) + 0), TO_ARGS_16(arr, (base) + 16)
#define TO_ARGS_64(arr, base)  TO_ARGS_32(arr, (base) + 0), TO_ARGS_32(arr, (base) + 32)
#define TO_ARGS_128(arr, base) TO_ARGS_64(arr, (base) + 0), TO_ARGS_64(arr, (base) + 64)


