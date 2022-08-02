#ifndef CHECKSUM_H_
#define CHECKSUM_H_

/* Used for Murmur calculation */
#define ZHASH_MURMUR_SEED (17)

int checksum_buf_to_32_bit(char *buf, size_t len, void *out);
int checksum_buf_to_64_bit(char *buf, size_t len, void *out);
int checksum_buf_to_128_bit(char *buf, size_t len, void *out);

#endif /* CHECKSUM_H_ */
