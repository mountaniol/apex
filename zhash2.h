#ifndef ZHASH_H
#define ZHASH_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* hash table
 * keys are strings or integers
 * values are void *pointers */

#define COUNT_OF(arr) (sizeof(arr) / sizeof(*arr))
#define zfree free

/* Used for Murmur calculation */
#define ZHASH_SEED (17)

typedef struct {
	char *key_str; /**< String key representation */
	uint32_t key_str_len; /**< Length of the key string */
	uint32_t key_int; /**< The integer calculated from the key string */
} zkey_t;

typedef struct {
	void *val; /**< value */
	uint32_t val_size; /**< Size of the buffer pointer by val */
} val_t;

/* struct representing an entry in the hash table */
typedef struct ZHashEntry{
	zkey_t Key;
	val_t Val;
	struct ZHashEntry *next; /* The next entry */
} zentry_t;

/* struct representing the hash table
  size_index is an index into the hash_sizes array in hash.c */
typedef struct {
	uint32_t size_index;
	uint32_t entry_count;
	zentry_t **entries;
} ztable_t; 


/* The structures and macros below we need to create a flat dump of the zhash*/
#define ZHASH_WATERMARK (0xFAFA7777)
#define ZENTRY_WATERMARK (0x898AE990)

typedef struct {
	uint32_t watemark;
	uint32_t checksum;
	// uint32_t size_index;
	uint32_t entry_count;
} __attribute__((packed)) zhash_header_t;

typedef struct {
	uint32_t watemark; /**< Predefined value, for validation */
	uint32_t checksum; /**< Checksum of this entry, not implemented yet*/
	uint32_t key_int; /**< Key: integer */
	uint32_t key_len; /**< Length of the 'char *key', not includes terminating \0 */
	uint32_t val_len; /**< Length of the entry */
} __attribute__((packed)) zhash_entry_t;

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func ztable_t* zcreate_hash_table(void)
 * @brief Create new empty hash table
 * @param void
 * @return ztable_t* Pointer to new hash table on success, NULL on error
 * @details
 */
ztable_t *zcreate_hash_table(void);

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func void zfree_hash_table(ztable_t *hash_table)
 * @brief Release hash table.
 * @param ztable_t * hash_table Hash table to free
 * @details If the hash table is not empty, all entries will be released as well.
 *          The data kept in the entries not released
 */
void zfree_hash_table(ztable_t *hash_table);

/* hash operations */

/* Set of function wehere the key is a string */

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func void zhash_insert_by_str(ztable_t *hash_table, char *key, void *val)
 * @brief Insert new entry into the hash table. The key is string.
 * @param ztable_t * hash_table A hash table to insert the new entry into
 * @param const char * key The key to used for insert / search
 * @param const size_t key_len - Length of the key
 * @param const void * val Pointer to data
 * @param const size_t val_len - Length of the buffer pointed by 'val'
 * @details
 */
void zhash_insert_by_str(ztable_t *hash_table,
						 char *key_str,
						 const size_t key_str_len,
						 void *val,
						 const size_t val_size);
/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func void* zhash_find_by_str(ztable_t *hash_table, char *key)
 * @brief Find an entry in hash table
 * @param ztable_t * hash_table The hash table to search
 * @param char * key A null terminated string to use as key for searching
 * @return void* Pointer to data kept in the hash table, NULL if not found
 * @details The found entry will be not released
 */
void *zhash_find_by_str(ztable_t *hash_table, char *key);

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func void* zhash_extract_by_str(ztable_t *hash_table, char *key)
 * @brief Find and extract data from the hash table using string as the search key
 * @param ztable_t * hash_table The hash table to search in
 * @param char * key A null terminated string to use for search
 * @return void* Data kept in hash table, NULL if not found
 * @details This function removes the found entry from the hash and returns data to caller
 */
void *zhash_extract_by_str(ztable_t *hash_table, const char *key);

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func bool zhash_exists_by_str(ztable_t *hash_table, char *key)
 * @brief Check if an entry for the given key exists in the hash table
 * @param ztable_t * hash_table The hash table to test
 * @param char * key A null terninated string to search
 * @return bool True if a record for the given key presens, false if doesnt
 * @details
 */
bool zhash_exists_by_str(ztable_t *hash_table, const char *key);

/* Set of function where the key is an integer */

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func void zhash_insert_by_str(ztable_t *hash_table, char *key, void *val)
 * @brief Insert new entry into the hash table. The key is integer.
 * @param ztable_t * hash_table A hash table to insert the new entry into
 * @param u_int32_t key The key to use for insert / search
 * @param void * val Pointer to data
 * @details
 */
void zhash_insert_by_int(ztable_t *hash_table, u_int32_t int_key, void *val, size_t val_size);

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func void *zhash_find_by_int(ztable_t *hash_table, u_int32_t key)
 * @brief Find an entry in hash table
 * @param ztable_t * hash_table The hash table to search
 * @param u_int32_t key An integer value to use as key for searching
 * @return void* A pointer to data kept in the hash table, NULL if not found
 * @details The found entry will be deleted from the table
 */
void *zhash_find_by_int(ztable_t *hash_table, u_int32_t key);

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func void *zhash_extract_by_int(ztable_t *hash_table, u_int32_t key)
 * @brief Find and extract data from the hash table using an integer as the search key
 * @param ztable_t * hash_table The hash table to search in
 * @param u_int32_t key An integer value to use for search
 * @return void* Data kept in hash table, NULL if not found
 * @details This function removes the found entry from the hash and returns data to caller
 */
void *zhash_extract_by_int(ztable_t *hash_table, const u_int32_t key);

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func bool zhash_exists_by_int(ztable_t *hash_table, u_int32_t key)
 * @brief Check if an entry for the given key exists in the hash table
 * @param ztable_t * hash_table The hash table to test
 * @param u_int32_t key An integer value to search
 * @return bool True if the a record for the given key presens, false if doesnt
 * @details
 */
bool zhash_exists_by_int(const ztable_t *hash_table, const u_int32_t key);

/* hash entry creation and destruction */

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func zentry_t* zentry_t_alloc_str(char *key, void *val)
 * @brief Allocate new entry for the hash table
 * @param char * key Null terminated string to use as the key
 * @param void * val Pointer to a date to keep in the hash table
 * @return zentry_t* Pointer to a new zenty_t structure on success, NULL on error
 * @details
 */
zentry_t *zentry_t_alloc_str(const char *key, const size_t key_len, void *val, const size_t val_size);
/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func zentry_t *zentry_t_alloc_int(u_int32_t key, void *val)
 * @brief Allocate new entry for the hash table
 * @param char * key An integer to use as the key
 * @param void * val Pointer to a date to keep in the hash table
 * @return zentry_t* Pointer to a new zenty_t structure on success, NULL on error
 * @details
 */
zentry_t *zentry_t_alloc_int(u_int32_t key, void *val);

/**
 * @author Sebastian Mountaniol (23/08/2020)
 * @func void zentry_t_release(zentry_t *entry, bool recursive)
 * @brief Release zentry
 * @param zentry_t * entry The entry to release
 * @param bool recursive If this == true, and the entry holds several nodes (i.e. linked list) release all of them
 * @details
 */
void zentry_t_release(zentry_t *entry, const bool recursive);

/**
 * @author Sebastian Mountaniol (11/3/21)
 * zentry_t* zhash_list(ztable_t *hash_table, size_t *index, zentry_t *entry)
 * 
 * @brief Iterate over all entries in zhash
 * @param hash_table - The hash table to iterate
 * @param index - Pointer to index, must be inited by caller as
 *  						0
 * @param entry - The pointer of the previously returned
 *  								 entry, must be passed every time
 * 
 * @return zentry_t* - A pointer to an entry, or NULL when
 *  			 no more entries
 * @details Be careful! This function return zentry_t! You
 *  				should use entry->val to get the value saved in hash
 */
zentry_t *zhash_list(const ztable_t *hash_table, size_t *index, const zentry_t *entry);

/**
 * @author Sebastian Mountaniol (7/27/22)
 * @brief Create a flat buffer from the zhash table. You can
 *  	  restore zhash from this memory buffer with function
 *  	  ::zhash_from_buf()
 * @param ztable_t* hash_table Hash to dump into memory buffer
 * @param size_t* size  The size of resulting buffer will be
 *  			returned in this variable
 * @return void* Poiter to the new buffer. In case of error a
 *  	   NULL returned.
 * @details The original zhash not affected by this operation,
 *  		you can use it or release it, by your choice.
 */
extern void *zhash_to_buf(const ztable_t *hash_table, size_t *size);

/**
 * @author Sebastian Mountaniol (7/27/22)
 * @brief Create zhash table from the flat memory buffer. The
 *  	  flat memory buffer must be a result of
 *  	  ::zhash_to_buf() function
 * @param void* buf   Flat memory buffer containing a dump of
 *  		  zhash table
 * @param size_t size  Size of the flat memory buffer
 * @return ztable_t* zhash object, restored from the buffer.
 *  	   NULL on an error.
 * @details 
 */
extern ztable_t *zhash_from_buf(const char *buf, const size_t size);
#endif

