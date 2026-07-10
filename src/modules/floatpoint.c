#include "floatpoint.h"

union c64float {
    struct {
        uint8_t exponent;
        uint8_t mantissa[4];
        uint8_t sign;
    } c64float_t;
    uint8_t bytes[6];
};

float c64_fac_to_ieee(const uint8_t* ram) {
    uint8_t exp  = ram[0x61]; 
    uint8_t s_byte = ram[0x66]; 
    
    
    if (exp == 0) return 0.0f;

    
    uint32_t mantissa = ((uint32_t)ram[0x62] << 24) |
                        ((uint32_t)ram[0x63] << 16) |
                        ((uint32_t)ram[0x64] << 8)  |
                        ((uint32_t)ram[0x65]);

    
    int32_t true_exp = (int32_t)exp - 128;

    
    
    
    uint32_t ieee_mantissa = (mantissa << 1) & 0x007FFFFF; 
    true_exp--; 

    
    int32_t ieee_exp = true_exp + 127;
    
    
    if (ieee_exp <= 0) return 0.0f;
    if (ieee_exp >= 255) ieee_exp = 254; 

    
    uint32_t sign_bit = (s_byte & 0x80) ? 0x80000000 : 0x00000000;
    uint32_t ieee_bits = sign_bit | ((uint32_t)ieee_exp << 23) | ieee_mantissa;

    float result;
    memcpy(&result, &ieee_bits, 4);
    return result;
}

void ieee_to_c64_fac(uint8_t* ram, float value) {
    uint32_t ieee_bits;
    memcpy(&ieee_bits, &value, 4);

    uint32_t sign_bit = ieee_bits & 0x80000000;
    int32_t ieee_exp  = (ieee_bits >> 23) & 0xFF;
    uint32_t mantissa = ieee_bits & 0x007FFFFF;

    
    if (ieee_exp == 0) {
        memset(&ram[0x61], 0, 6); 
        return;
    }

    
    mantissa |= 0x00800000;

    
    int32_t true_exp = ieee_exp - 127 + 1;
    int32_t c64_exp = true_exp + 128;

    
    if (c64_exp <= 0) {
        memset(&ram[0x61], 0, 6);
        return;
    }
    if (c64_exp > 255) c64_exp = 255;

    
    uint32_t c64_mantissa = mantissa << 8;

    
    ram[0x61] = (uint8_t)c64_exp;
    ram[0x62] = (uint8_t)(c64_mantissa >> 24);
    ram[0x63] = (uint8_t)(c64_mantissa >> 16);
    ram[0x64] = (uint8_t)(c64_mantissa >> 8);
    ram[0x65] = (uint8_t)(c64_mantissa);
    ram[0x66] = (sign_bit) ? 0x80 : 0x00;
}
