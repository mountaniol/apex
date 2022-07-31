#include <string.h>
#include <stdlib.h>

#include "debug.h"
#include "tests.h"
#include "zhash3.h"
#include "murmur3.h"

/* possible sizes for hash table; must be prime numbers */
static const size_t hash_sizes[] = {
	53, 101, 211, 503, 1553, 3407, 6803, 12503, 25013, 50261,
	104729, 250007, 500009, 1000003, 2000029, 4000037, 10000019,
	25000009, 50000047, 104395301, 217645177, 512927357, 1000000007
};

/*** STATIC FUNCTIONS ***/

static void *zmalloc(const size_t sz)
{
	return calloc(1, sz);
}

static size_t next_size_index(const size_t size_index)
{
	if (size_index == COUNT_OF(hash_sizes)) return (size_index);

	return (size_index + 1);
}

static size_t previous_size_index(const size_t size_index)
{
	if (size_index == 0) return (size_index);

	return (size_index - 1);
}

static void *zcalloc(const size_t num, const size_t size)
{
	void *ptr = calloc(num, size);
	if (!ptr) exit(EXIT_FAILURE);
	return (ptr);
}

static ztable_t *zcreate_hash_table_with_size(const size_t size_index)
{
	ztable_t *hash_table;

	hash_table = zmalloc(sizeof(ztable_t));

	hash_table->size_index = size_index;
	hash_table->entry_count = 0;
	hash_table->entries = zcalloc(hash_sizes[size_index], sizeof(void *));
	return (hash_table);
}

/**
 * @author Sebastian Mountaniol (7/28/22)
 * @brief Generate integer key (hash) from the string 
 * @param const char* key_str   String
 * @param const size_t key_str_len   String length
 * @return uint64_t Calculated hash (key value)
 * @details 
 */

static uint64_t zhash_key_int64_from_key_str(const char *key_str, const size_t key_str_len)
{
	uint64_t key_int64 = 0;
	if (NULL == key_str || 0 == key_str_len) {
		DE("Wrong arguments: key_str = %p, key_str_len = %zu\n", key_str, key_str_len);
		abort();
	}
	MurmurHash3_x86_128_to_64(key_str, key_str_len, ZHASH_MURMUR_SEED, &key_int64);
	return key_int64;
}

/**
 * @author Sebastian Mountaniol (7/31/22)
 * @brief Generate hash, means: index in zhash->entries array
 * @param const ztable_t* hash_table Pointer to hash table
 *  			struct
 * @param const uint64_t key_int64 Integer key
 * @return size_t Hash, the array index in zhash->entries
 * @details 
 */
static size_t zgenerate_hash_by_key_int64(const ztable_t *hash_table, const uint64_t key_int64)
{
	const size_t size = hash_sizes[hash_table->size_index];
	return (key_int64 % size);
}

/**
 * @author Sebastian Mountaniol (7/29/22)
 * @brief Rebuild the hash to the new size
 * @param ztable_t* hash_table Hash table 
 * @param const size_t size_index New size
 * @details 
 */
static void zhash_rehash(ztable_t *hash_table, const size_t size_index)
{
	size_t   hash;
	size_t   size;
	size_t   ii;
	zentry_t **entries;

	if (size_index == hash_table->size_index) return;

	size = hash_sizes[hash_table->size_index];
	entries = hash_table->entries;

	hash_table->size_index = size_index;
	hash_table->entries = zcalloc(hash_sizes[size_index], sizeof(void *));

	for (ii = 0; ii < size; ii++) {
		zentry_t *entry;

		entry = entries[ii];
		while (entry) {
			zentry_t *next_entry;
			hash = zgenerate_hash_by_key_int64(hash_table, entry->Key.key_int64);
			next_entry = entry->next;
			entry->next = hash_table->entries[hash];
			hash_table->entries[hash] = entry;

			entry = next_entry;
		}
	}
	zfree(entries);
}

/**
 * @author Sebastian Mountaniol (7/29/22)
 * @brief Fill zentry structure 
 * @param zentry_t* entry      	Structure to fill
 * @param uint64_t key_int64    Integer key, must be
 *  				calculated before this call
 * @param void* val        		Value to keep in the entry
 * @param const size_t val_size Size (in bytes) of the value to
 *  			keep
 * @param char* key_str    		String key; can by NULL
 * @param const size_t key_str_len	Size (length) of the string
 *  			key. No includes \0 terminator, i.e. equal to
 *  			strlen(key_str)
 * @details 
 */
static void zentry_t_fill(zentry_t *entry, uint64_t key_int64,
						  void *val,
						  const size_t val_size,
						  char *key_str,
						  const size_t key_str_len)
{
	entry->Key.key_int64 = key_int64;
	entry->Key.key_str = key_str;
	entry->Key.key_str_len = key_str_len;
	entry->Val.val = val;
	entry->Val.val_size = val_size;
	entry->next = NULL;
}

static zentry_t *zentry_t_alloc(uint64_t key_int64, void *val, size_t val_size, char *key_str, size_t key_str_len)
{
	zentry_t *entry = zmalloc(sizeof(zentry_t));
	zentry_t_fill(entry, key_int64, val, val_size, key_str, key_str_len);
	return (entry);
}

static void zentry_t_release(zentry_t *entry, const bool recursive, const int force_values_clean)
{
	if (recursive && entry->next) zentry_t_release(entry->next, recursive, force_values_clean);

	if (NULL != entry->Key.key_str) {
		DDD("Going to release entry: %p\n", entry);
		DDD("Going to release entry->Key.key_str: %p\n", entry->Key.key_str);
		zfree(entry->Key.key_str);
		entry->Key.key_str = NULL;

		/* If asked to clean the values kept in the ztable: */
		if (force_values_clean && NULL != entry->Val.val) {
			zfree(entry->Val.val);
		}
	}

	zfree(entry);
}

/*** END OF STATIC FUNCTIONS ***/

ztable_t *zhash_table_allocate(void)
{
	return (zcreate_hash_table_with_size(0));
}

void zhash_table_release(ztable_t *hash_table, const int force_values_clean)
{
	size_t size;
	size_t ii;

	size = hash_sizes[hash_table->size_index];

	for (ii = 0; ii < size; ii++) {
		zentry_t *entry;

		if ((entry = hash_table->entries[ii])) {
			DDD("Going to release entry: %p\n", entry);
			zentry_t_release(entry, true, force_values_clean);
		}
	}

	zfree(hash_table->entries);
	zfree(hash_table);
}

/* Internal generic insert. Except all values for an entry. ALways insert by ineger key */
static int zhash_insert(ztable_t *hash_table,
						uint64_t key_int64,
						char *key_str,
						const size_t key_str_len,
						void *val,
						const size_t val_size)
{
	size_t       size;
	zentry_t     *entry;
	const size_t hash   = zgenerate_hash_by_key_int64(hash_table, key_int64);
	entry = hash_table->entries[hash];

	while (entry) {
		/* If existing such an entry, replace its content */
		if (key_int64 == entry->Key.key_int64) {
			DD("Replacing an item: old key %s / %lX, new %s / %lX\n", entry->Key.key_str, entry->Key.key_int64, key_str, key_int64);
			zentry_t_fill(entry, key_int64, val, val_size, key_str, key_str_len);
			return 1;
		}
		entry = entry->next;
	}

	entry = zentry_t_alloc(key_int64, val, val_size, key_str, key_str_len);

	entry->next = hash_table->entries[hash];
	hash_table->entries[hash] = entry;
	hash_table->entry_count++;

	size = hash_sizes[hash_table->size_index];

	if (hash_table->entry_count > size / 2) {
		zhash_rehash(hash_table, next_size_index(hash_table->size_index));
	}
	return 0;
}

int zhash_insert_by_int(ztable_t *hash_table, uint64_t int_key, void *val, size_t val_size)
{
	return zhash_insert(hash_table, int_key, NULL, 0, val, val_size);
}

int zhash_insert_by_str(ztable_t *hash_table,
						char *key_str,
						const size_t key_str_len,
						void *val,
						const size_t val_size)
{
	char     *key_str_copy;
	uint64_t key_int       = zhash_key_int64_from_key_str(key_str, key_str_len);
	DDD("Calculated key_int: %X\n", key_int);
	key_str_copy = strndup(key_str, key_str_len);
	if (NULL == key_str_copy) {
		DE("Could not duplicate string key");
		abort();
	}
	return zhash_insert(hash_table, key_int, key_str_copy, key_str_len, val, val_size);
}

void *zhash_find_by_int(ztable_t *hash_table, uint64_t key_int64)
{
	zentry_t     *entry;
	const size_t hash   = zgenerate_hash_by_key_int64(hash_table, key_int64);
	entry = hash_table->entries[hash];

	while (entry && (key_int64 != entry->Key.key_int64)) entry = entry->next;

	return (entry ? entry->Val.val : NULL);
}

/* TODO: Convert string to int and search by  int */
void *zhash_find_by_str(ztable_t *hash_table, char *key_str, const size_t key_str_len)
{
	uint64_t key_int64 = zhash_key_int64_from_key_str(key_str, key_str_len);
	DDD("Calculated key_int: %X\n", key_int64);
	return zhash_find_by_int(hash_table, key_int64);

}

void *zhash_extract_by_int(ztable_t *hash_table, const uint64_t key_int64)
{
	size_t       size;
	zentry_t     *entry;
	void         *val;
	const size_t hash   = zgenerate_hash_by_key_int64(hash_table, key_int64);
	entry = hash_table->entries[hash];

	if (entry && key_int64 == entry->Key.key_int64) {
		hash_table->entries[hash] = entry->next;
	} else {
		while (entry) {
			if (entry->next && (key_int64 == entry->next->Key.key_int64)) {
				zentry_t *deleted_entry;

				deleted_entry = entry->next;
				entry->next = entry->next->next;
				entry = deleted_entry;
				break;
			}
			entry = entry->next;
		}
	}

	if (!entry) return (NULL);

	val = entry->Val.val;
	zentry_t_release(entry, false, 0);
	hash_table->entry_count--;

	size = hash_sizes[hash_table->size_index];

	if (hash_table->entry_count < size / 8) {
		zhash_rehash(hash_table, previous_size_index(hash_table->size_index));
	}

	return (val);
}

void *zhash_extract_by_str(ztable_t *hash_table, const char *key_str, const size_t key_str_len)
{
	const uint64_t key_int64 = zhash_key_int64_from_key_str(key_str, key_str_len);
	return zhash_extract_by_int(hash_table, key_int64);
}

bool zhash_exists_by_int(const ztable_t *hash_table, const uint64_t key_int64)
{
	zentry_t     *entry;
	const size_t hash   = zgenerate_hash_by_key_int64(hash_table, key_int64);
	entry = hash_table->entries[hash];

	while (entry && key_int64 != entry->Key.key_int64) entry = entry->next;
	if (entry) {
		return true;
	}

	return false;
}

bool zhash_exists_by_str(ztable_t *hash_table, const char *key_str, size_t key_str_len)
{
	const uint64_t key_int64 = zhash_key_int64_from_key_str(key_str, strnlen(key_str, key_str_len));
	return zhash_exists_by_int(hash_table, key_int64);
}

/*** Iterate all items in hash ***/

zentry_t *zhash_list(const ztable_t *hash_table, size_t *index, const zentry_t *entry)
{
	size_t   size;
	TESTP(hash_table, NULL);
	TESTP(index, NULL);

	/* Get numeric size of the hash table */
	size = hash_sizes[hash_table->size_index];

	/* If there is a next member in this hash table cell, just return */
	if (NULL != entry && entry->next) {
		return entry->next;
	}

	/* No more entried in the linked list, advance index */
	(*index)++;

	/* Test all entries, until a filled index found in the array */
	while (*index < size) {
		/* Is there an entry? Return it */
		if (hash_table->entries[*index]) {
			return hash_table->entries[*index];
		}

		/* No entry here; advance the index */
		(*index)++;
	}
	return NULL;
}

/*** ADDITION: ZHASH TO BUF / BUF TO ZHASH ***/

/**
 * @author Sebastian Mountaniol (7/28/22)
 * @brief This function calculated the size of the buffer (in
 *  	  bytes) enough to contain a flat zhash dump buffer
 * @param ztable_t* hash_table
 * @return size_t Size of needded buffer, in bytes
 * @details 
 */
static size_t zhash_to_buf_allocation_size(const ztable_t *hash_table)
{
	/* We need one header for the whole buffer */
	size_t size  = sizeof(zhash_header_t);
	size_t index;

	/* Per entry we need entry header */

	/* Now run on all entries and count data size */
	for (index = 0; index < size; index++) {
		zentry_t *entry = hash_table->entries[index];
		/* Is there an entry? Return it */
		while (entry) {
			size += sizeof(zhash_entry_t);
			size += entry->Key.key_str_len;
			size += entry->Val.val_size;
		}
	}
	return size;
}

void *zhash_to_buf(const ztable_t *hash_table, size_t *size)
{
	size_t         index;
	size_t         offset   = 0;
	zhash_header_t *zheader;
	zhash_entry_t  *zentry;

	char           *buf;
	*size = zhash_to_buf_allocation_size(hash_table);

	buf = malloc(*size);
	zheader = (zhash_header_t *)buf;
	zheader->entry_count = hash_table->entry_count;
	zheader->checksum = ZHASH_WATERMARK;
	zheader->checksum = 0;

	offset += sizeof(zhash_header_t);

	/* Now run on all entries and count data size */
	for (index = 0; index < *size; index++) {
		zentry_t *entry = hash_table->entries[index];
		/* Is there an entry? Return it */
		while (entry) {
			/* Advance the pointer */
			zentry = (zhash_entry_t *)(buf + offset);
			zentry->watemark = ZENTRY_WATERMARK;
			zentry->checksum = 0;
			zentry->key_len = entry->Key.key_str_len;
			zentry->key_int = entry->Key.key_int64;
			zentry->val_len = entry->Val.val_size;

			/* Now, dump the string key (if any) and val (if any) */
			offset += sizeof(zhash_entry_t);
			if (entry->Key.key_str) {
				memcpy(buf + offset, entry->Key.key_str, entry->Key.key_str_len);
				offset += entry->Key.key_str_len;
			}

			if (entry->Val.val) {
				memcpy(buf + offset, entry->Val.val, entry->Val.val_size);
				offset += entry->Val.val_size;
			}
		}
	}
	return buf;
}

ztable_t *zhash_from_buf(const char *buf, const size_t size)
{
	size_t         index;
	size_t         offset = 0;
	zhash_header_t *zhead;
	zhash_entry_t  *zent;

	zhead = (zhash_header_t *)buf;
	if (ZHASH_WATERMARK != zhead->watemark) {
		DE("Bad watermark in zhash_header_t: expected %X but it is %X\n", ZHASH_WATERMARK, zhead->watemark);
		abort();
	}

	/* From the header we know the count of entries in the zhash table */
	ztable_t *zt = zcreate_hash_table_with_size(zhead->entry_count);
	TESTP(zt, NULL);

	offset += sizeof(zhash_header_t);

	for (index = 0; index < zhead->entry_count; index++) {
		zent = (zhash_entry_t  *)(buf + offset);
		if (ZENTRY_WATERMARK != zent->watemark) {
			DE("Bad watermark in zhash_entry_t: expected %X but it is %X\n", ZENTRY_WATERMARK, zent->watemark);
		}

		/* Create the zhash entry with the given paramenters */

	}

	return NULL;
}

