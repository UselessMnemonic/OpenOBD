#include <stdint.h>

#pragma once

#define CRC_POLY 0xEDB88320

uint32_t calc_crc32(const uint32_t polynomial, uint8_t *buffer, uint32_t buffer_len);
