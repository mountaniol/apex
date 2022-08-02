#include "checksum.h"
#include "murmur3.h"
#include "tests.h"
#include "debug.h"

int checksum_buf_to_32_bit(const char *buf_input, const size_t buf_input_size, void *output_32)
{
	TESTP(buf_input, -1);
	TESTP(output_32, -1);
	if (0 == buf_input_size) {
		DE("Size of the input buffer == 0\n");
		return -1;
	}

	DDD("Going to calculate checksum: buf %p, size %zu, output %p\n", buf_input, buf_input_size, output_32);
	MurmurHash3_x86_32(buf_input, buf_input_size, ZHASH_MURMUR_SEED, output_32);
	DDD("Calculated checksum: %X\n", *((uint32_t *) output_32));
	return 0;
}

int checksum_buf_to_64_bit(const char *buf_input, const size_t buf_input_size, void *output_64)
{
	TESTP(buf_input, -1);
	TESTP(output_64, -1);
	if (0 == buf_input_size) {
		DE("Size of the input buffer == 0\n");
		return -1;
	}

	MurmurHash3_x86_128_to_64(buf_input, buf_input_size, ZHASH_MURMUR_SEED, output_64);
	return 0;
}

int checksum_buf_to_128_bit(const char *buf_input, const size_t buf_input_size, void *output_128)
{
	TESTP(buf_input, -1);
	TESTP(output_128, -1);
	if (0 == buf_input_size) {
		DE("Size of the input buffer == 0\n");
		return -1;
	}

	MurmurHash3_x86_128(buf_input, buf_input_size, ZHASH_MURMUR_SEED, output_128);
	return 0;
}

