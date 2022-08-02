#ifndef CHECKSUM_H_
#define CHECKSUM_H_

#include <stddef.h> /* For size_t */
#include "optimization.h"

/* Used for Murmur calculation */
#define ZHASH_MURMUR_SEED (17)

int checksum_buf_to_32_bit(const char *buf_input, const size_t buf_input_size, void *output_32);

int checksum_buf_to_64_bit(const char *buf_input, const size_t buf_input_size, void *output_64);

int checksum_buf_to_128_bit(const char *buf_input, const size_t buf_input_size, void *output_128);

#endif /* CHECKSUM_H_ */
