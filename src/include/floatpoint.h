
#ifndef FLOATPOINT_H
#define FLOATPOINT_H

#include <stdint.h>
#include <string.h>

float c64_fac_to_ieee(const uint8_t* ram);
void ieee_to_c64_fac(uint8_t* ram, float value);

#endif // FLOATPOINT_H