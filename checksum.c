#include "checksum.h"
#include "murmur3.h"
#include "tests.h"
#include "debug.h"


int checksum_buf_to_32_bit(char *buf, size_t size, void *out)
{
	TESTP(buf, -1);
	TESTP(out, -1);
	if (0 == size) {
		DE("Size of the input buffer == 0\n");
		return -1;
	}

	MurmurHash3_x86_32(buf, size, ZHASH_MURMUR_SEED, out);
	return 0;
}

int checksum_buf_to_64_bit(char *buf, size_t len, void *out)
{
	TESTP(buf, -1);
	TESTP(out, -1);
	if (0 == size) {
		DE("Size of the input buffer == 0\n");
		return -1;
	}

	MurmurHash3_x86_128_to_64(buf, size, ZHASH_MURMUR_SEED, out);
	return 0;
}

int checksum_buf_to_128_bit(char *buf, size_t len, void *out)
{
}

