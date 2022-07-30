#include <string.h>
#include <stdlib.h>
#include "zhash3.h"
#include "debug.h"
#include "tests.h"

static ztable_t *zllocate_empty_zhash(void)
{
	ztable_t *zt = zcreate_hash_table();
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
	zfree_hash_table(zt, 1);
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
	/* Allocate an empty zhash table */
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
	zhash_insert_by_str(zt, ONE_ITEM_SIZE_KEY_STR, strlen(ONE_ITEM_SIZE_KEY_STR), item, ONE_ITEM_SIZE);
	DDD("Inserted item: %p\n", item);

	/* Try to extract the item by the key */
	item_ret = zhash_find_by_str(zt, ONE_ITEM_SIZE_KEY_STR);

	/* The poiter must be the same */
	if (item_ret != item) {
		DE("[TEST] Pointer of returned item (%p) does not match th item (%p)\n", item_ret, item);
		abort();
	}

	DDD("Returned pointer to item: %p\n", item_ret);

	//free(item_ret);

	/* Free the table */
	DDD("Going to release the hass table. Also release the values. \n");
	zfree_hash_table(zt, 1);
	DD("[TEST] Successfully finished basic test\n");
}

int main(void)
{
	basic_test();
	add_one_item_test();
	return 0;
}
