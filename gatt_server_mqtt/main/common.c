#include "common.h"

static int16_t rawParams[32]; 

void setparam(int i, int16_t value)
{
    rawParams[i] = value;
}

int16_t getparam(int i)
{
    return rawParams[i];
}
