#include "crypto.h"

#pragma GCC push_options
#pragma GCC optimize ("unroll-loops")

uint32_t calc_crc32(const uint32_t polynomial, uint8_t *buffer, uint32_t buffer_len)  {
  uint32_t remainder = 0xFFFFFFFF; //Prevents run of 0's from outputting 0.

  for (uint32_t i = 0; i < buffer_len; ++i) {
    uint8_t current_byte = *(buffer + i);
    remainder ^= current_byte; //Adjust remainder for current byte
    for (uint8_t bit_counter = 0; bit_counter < 8; ++bit_counter) {
      //For each bit, we need to perform the XOR
      remainder = (remainder >> 1) ^ (polynomial & -(remainder & 1));
    }
  }
  return remainder;
}
#pragma GCC pop_options
