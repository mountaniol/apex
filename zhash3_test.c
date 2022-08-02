#include <string.h>
#include <stdlib.h>
#include "zhash3.h"
#include "debug.h"
#include "tests.h"

static ztable_t *zllocate_empty_zhash(void)
{
	ztable_t *zt = zhash_allocate();
	if (NULL == zt) {
		DE("[TEST] Failed to allocate zhash table\n");
		abort();
	}

	/* The table must be empty - 0 number of entries */
	if (zt->entry_count > 0) {
		DE("[TEST] Number of entries must bw 0, but it is %d\n", zt->entry_count);
		abort();
	}

	/* The entries must not be NULL pointer of entries */
	if (NULL == zt->entries) {
		DE("[TEST] Pointer of entries is NULL, but it is should not be\n");
		abort();
	}

	return zt;
}

/* Create and release an empty zhash table */
static void basic_test(void)
{
	/* Allocate an empty zhash table */
	ztable_t *zt = zllocate_empty_zhash();

	/* Free the table, force cleaning values. And let's see what happens */
	zhash_release(zt, 1);
	DD("[TEST] Successfully finished basic test\n");
}

/* Size of the item for the 'one item' test */
#define ONE_ITEM_SIZE (16)
/* Fill the item with this pattern */
#define ONE_ITEM_SIZE_PATTERN 0xA7

/* Use this string key to insert the item */
#define ONE_ITEM_SIZE_KEY_STR "The Key"

/* Create and release an empty zhash table */
static void add_one_item_test(void)
{
	int rc;
	/* Allocate an empty zhash table */
	ssize_t val_size;
	ztable_t *zt       = zllocate_empty_zhash();

	void     *item     = malloc(ONE_ITEM_SIZE);
	void     *item_ret = NULL;
	if (NULL == item) {
		DE("[TEST] Pointer of item is NULL allocation failed\n");
		abort();
	}

	/* FIll the item with the pattern */
	memset(item, ONE_ITEM_SIZE_PATTERN, ONE_ITEM_SIZE);

	/* Add item into the hash */
	rc = zhash_insert_by_str(zt, ONE_ITEM_SIZE_KEY_STR, strlen(ONE_ITEM_SIZE_KEY_STR), item, ONE_ITEM_SIZE);

	if (rc < 0) {
		DE("Error: returned status < 0, aborting\n");
		abort();
	}

	if (rc > 0) {
		DE("Collision: the key %s can not be inserted because of collision\n", ONE_ITEM_SIZE_KEY_STR);
		abort();
	}

	DDD("Inserted item: %p\n", item);

	/* Try to extract the item by the key */
	item_ret = zhash_find_by_str(zt, ONE_ITEM_SIZE_KEY_STR, strlen(ONE_ITEM_SIZE_KEY_STR), &val_size);

	/* The poiter must be the same */
	if (item_ret != item) {
		DE("[TEST] Pointer of returned item (%p) does not match th item (%p)\n", item_ret, item);
		abort();
	}
	if (val_size != ONE_ITEM_SIZE) {
		DE("[TEST] Wrong size returned: expected %d but it is %zd\n", ONE_ITEM_SIZE, val_size);
		abort();
	}

	DDD("Returned pointer to item: %p\n", item_ret);

	//free(item_ret);

	/* Free the table */
	DDD("Going to release the hass table. Also release the values. \n");
	zhash_release(zt, 1);
	DD("[TEST] Successfully finished basic test\n");
}

/* How many items to add into hash */
#define NUMBER_OF_ITEMS (1024 * 1024 * 10)
static void add_many_items_test(void)
{
	int rc;
	ssize_t val_size;
	/* Allocate an empty zhash table */
	uint32_t   index;
	const char *key_base          = "Key";
	char       key_full_name[32];
	char       key_full_name_size;

	/* This allocation can not fail; it if fail, the execution aborted */
	ztable_t   *zt                = zllocate_empty_zhash();

	DD("[TEST] Strting a long and intensive test of adding %d items into zhash\n", NUMBER_OF_ITEMS);

	for (index = 0; index < NUMBER_OF_ITEMS; index++) {
		void *item     = malloc(ONE_ITEM_SIZE);
		void *item_ret = NULL;

		if (NULL == item) {
			DE("[TEST] Pointer of item is NULL allocation failed\n");
			abort();
		}

		if (0 == (index % (1024 * 100))) {
				PR("\rAdding %d / %d  (%.3f %%) \r", index, NUMBER_OF_ITEMS, (double)index/(NUMBER_OF_ITEMS) *100 );
		}

		/* Fill the item with the pattern */
		memset(item, 0, ONE_ITEM_SIZE);

		key_full_name_size = sprintf(key_full_name, "%s_%u", key_base, index);
		memcpy(item, key_full_name, key_full_name_size);

		/* Add item into the hash */
		rc = zhash_insert_by_str(zt, key_full_name, key_full_name_size, item, ONE_ITEM_SIZE);
		if (rc < 0) {
			DE("Error: returned status < 0, aborting\n");
			abort();
		}

		if (rc > 0) {
			item_ret = zhash_find_by_str(zt, key_full_name, key_full_name_size, &val_size);
			DE("Collision: the key %s can not be inserted because of collision with: %s\n", key_full_name, (char *)item_ret);
			abort();
		}

		DDD("Inserted item: %p\n", item);

		/* Try to extract the item by the key */
		item_ret = zhash_find_by_str(zt, key_full_name, key_full_name_size, &val_size);

		/* The poiter must be the same */
		if (item_ret != item) {
			DE("[TEST] Pointer of returned item (%p) does not match the item (%p)\n", item_ret, item);
			abort();
		}

		if (val_size != ONE_ITEM_SIZE) {
			DE("[TEST] Wrong size returned: expected %d but it is %zd\n", ONE_ITEM_SIZE, val_size);
			abort();
		}

		DDD("Returned pointer to item: %p\n", item_ret);

		//free(item_ret);

		/* Free the table */
		DDD("Going to release the hass table. Also release the values. \n");
	}

	zhash_release(zt, 1);
	DD("[TEST] Congrats! Successfully finished the stressed zhash\n");
}


/* How many items to create before test zhash to buffer and back */
// #define NUMBER_OF_ITEMS_ZHASH_TO_BUF (1024)
#define NUMBER_OF_ITEMS_ZHASH_TO_BUF (1024)
static void zhash_to_buf_and_back(void)
{
	int rc;
	ssize_t val_size;
	/* Allocate an empty zhash table */
	uint32_t   index;
	const char *key_base          = "Key";
	char       key_full_name[32];
	char       key_full_name_size;

	/* This allocation can not fail; it if fail, the execution aborted */
	ztable_t   *zt                = zllocate_empty_zhash();

	ztable_t   *zt2               = NULL;
	char       *buf               = NULL;
	size_t     buf_size           = 0;

	for (index = 0; index < NUMBER_OF_ITEMS_ZHASH_TO_BUF; index++) {
		void *item     = malloc(ONE_ITEM_SIZE);
		void *item_ret = NULL;

		if (NULL == item) {
			DE("[TEST] Pointer of item is NULL allocation failed\n");
			abort();
		}

		/* Fill the item with the pattern */
		memset(item, 0, ONE_ITEM_SIZE);
		key_full_name_size = sprintf(key_full_name, "%s_%u", key_base, index);
		memcpy(item, key_full_name, key_full_name_size);

		/* Add item into the hash */
		rc = zhash_insert_by_str(zt, key_full_name, key_full_name_size, item, ONE_ITEM_SIZE);

		if (rc < 0) {
			DE("Error: returned status < 0, aborting\n");
			abort();
		}

		if (rc > 0) {
			item_ret = zhash_find_by_str(zt, key_full_name, key_full_name_size, &val_size);
			DE("Collision: the key %s can not be inserted because of collision with: %s\n", key_full_name, (char *)item);
			abort();
		}

		DDD("Inserted item: %p\n", item);

		/* Try to extract the item by the key */
		item_ret = zhash_find_by_str(zt, key_full_name, key_full_name_size, &val_size);

		/* The poiter must be the same */
		if (item_ret != item) {
			DE("[TEST] Pointer of returned item (%p) does not match th item (%p)\n", item_ret, item);
			abort();
		}

		if (val_size != ONE_ITEM_SIZE) {
			DE("[TEST] Wrong size returned: expected %d but it is %zd\n", ONE_ITEM_SIZE, val_size);
			abort();
		}

		DDD("Returned pointer to item: %p\n", item_ret);

		/* Free the table */
	}

	buf = zhash_to_buf(zt, &buf_size);
	if (NULL == buf) {
		DE("[TEST] zhash_to_buf returned NULL\n");
		abort();
	}

	if (0 == buf_size) {
		DE("[TEST] buf_size returned 0\n");
		abort();
	}

	zt2 = zhash_from_buf(buf, buf_size);
	free(buf);

	if (NULL == zt2) {
		DE("[TEST] zhash_from_buf returned NULL\n");
		abort();
	}

	if (0 != zhash_cmp_zhash(zt, zt2)) {
		DE("[TEST] zhash_from_buf and original zhash are not match\n");
		abort();
	}

	DDD("Going to release the zhash tables. Also release the values. \n");
	zhash_release(zt, 1);
	zhash_release(zt2, 1);
	DD("[TEST] Successfully finished zhash-to-buf and buf-to-zhash test\n");
}


int main(void)
{
	basic_test();
	add_one_item_test();
	add_many_items_test();
	zhash_to_buf_and_back();
	return 0;
}
