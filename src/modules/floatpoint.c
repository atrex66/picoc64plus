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
    uint8_t exp  = ram[0x61]; // Exponent
    uint8_t s_byte = ram[0x66]; // Sign byte
    
    // C64 Zero representations handle an exponent of 0
    if (exp == 0) return 0.0f;

    // Pull the 32-bit mantissa from RAM
    uint32_t mantissa = ((uint32_t)ram[0x62] << 24) |
                        ((uint32_t)ram[0x63] << 16) |
                        ((uint32_t)ram[0x64] << 8)  |
                        ((uint32_t)ram[0x65]);

    // Unbias exponent: C64 bias is 128
    int32_t true_exp = (int32_t)exp - 128;

    // C64 stores mantissa as normalized to 0.5 (implied "0.1xxxxx")
    // IEEE single precision requires normalization to 1.0 (implied "1.xxxxxx")
    // Shift left to drop the hidden bit and adjust the exponent
    uint32_t ieee_mantissa = (mantissa << 1) & 0x007FFFFF; 
    true_exp--; 

    // Re-bias for IEEE single precision (127)
    int32_t ieee_exp = true_exp + 127;
    
    // Handle underflow/overflow safety caps
    if (ieee_exp <= 0) return 0.0f;
    if (ieee_exp >= 255) ieee_exp = 254; 

    // Construct the binary representation
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

    // Handle zero
    if (ieee_exp == 0) {
        memset(&ram[0x61], 0, 6); // Clear exp, mantissa, sign
        return;
    }

    // Restore IEEE implied leading 1
    mantissa |= 0x00800000;

    // Unbias IEEE exponent and adjust for C64's 0.5 normalization point
    int32_t true_exp = ieee_exp - 127 + 1;
    int32_t c64_exp = true_exp + 128;

    // Clamp values to prevent out of bounds errors
    if (c64_exp <= 0) {
        memset(&ram[0x61], 0, 6);
        return;
    }
    if (c64_exp > 255) c64_exp = 255;

    // Convert mantissa from 24-bit to C64 32-bit width
    uint32_t c64_mantissa = mantissa << 8;

    // Write back into Zero Page
    ram[0x61] = (uint8_t)c64_exp;
    ram[0x62] = (uint8_t)(c64_mantissa >> 24);
    ram[0x63] = (uint8_t)(c64_mantissa >> 16);
    ram[0x64] = (uint8_t)(c64_mantissa >> 8);
    ram[0x65] = (uint8_t)(c64_mantissa);
    ram[0x66] = (sign_bit) ? 0x80 : 0x00;
}
