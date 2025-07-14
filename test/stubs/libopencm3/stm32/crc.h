#ifndef LIBOPENCM3_STM32_CRC_H
#define LIBOPENCM3_STM32_CRC_H
#include <stdint.h>
static inline void crc_reset(void) {}
static inline uint32_t crc_calculate_block(const uint32_t *buf, uint32_t len)
{
    (void)buf; (void)len; return 0;
}
#endif
