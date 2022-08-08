#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

#include "zhash3.h"
#include "tests.h"
#include "basket.h"
#include "box_t.h"
#include "debug.h"

#define STRING_ALICE_ALL_LEN (660)
const char *string_alice_all = "It was the White Rabbit, trotting slowly back again, and looking anxiously about as it went, as if it had lost something; and she heard it muttering to itself 'The Duchess! The Duchess! Oh my dear paws! Oh my fur and whiskers! She’ll get me executed, as sure as ferrets are ferrets! Where can I have dropped them, I wonder?' Alice guessed in a moment that it was looking for the fan and the pair of white kid gloves, and she very good-naturedly began hunting about for them, but they were nowhere to be seen—everything seemed to have changed since her swim in the pool, and the great hall, with the glass table and the little door, had vanished completely.";
const char *string_alice_1   = "It was the White Rabbit, trotting slowly back again, ";
const char *string_alice_2   = "and looking anxiously about as it went, as if it had lost something; ";
const char *string_alice_3   = "and she heard it muttering to itself 'The Duchess! The Duchess! Oh my dear paws! Oh my fur and whiskers! ";
const char *string_alice_4   = "She’ll get me executed, as sure as ferrets are ferrets! Where can I have dropped them, I wonder?' ";
const char *string_alice_5   = "Alice guessed in a moment that it was looking for the fan and the pair of white kid gloves, ";
const char *string_alice_6   = "and she very good-naturedly began hunting about for them, but they were nowhere to be seen—everything ";
const char *string_alice_7   = "seemed to have changed since her swim in the pool, and the great hall, ";
const char *string_alice_8   = "with the glass table and the little door, had vanished completely.";

const char *lorem_ipsum      = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\0";
#define LOREM_IPSUM_SIZE (447)
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
	PR("[TEST] Successfully finished ztable create and destroy test\n");
}

/* Size of the item for the 'one item' test */
#define ONE_ITEM_SIZE (16)
/* Fill the item with this pattern */
#define ONE_ITEM_SIZE_PATTERN 0xA7

/* Use this string key to insert the item */
#define ONE_ITEM_SIZE_KEY_STR "The Key"
#define ONE_ITEM_SIZE_KEY_STR_LEN (8)

/* Create and release an empty zhash table */
static void add_one_item_test(void)
{
	int      rc;
	/* Allocate an empty zhash table */
	ssize_t  val_size;
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
	rc = zhash_insert_by_str(zt, ONE_ITEM_SIZE_KEY_STR, strnlen(ONE_ITEM_SIZE_KEY_STR, ONE_ITEM_SIZE_KEY_STR_LEN), item, ONE_ITEM_SIZE);

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
	item_ret = zhash_find_by_str(zt, ONE_ITEM_SIZE_KEY_STR, strnlen(ONE_ITEM_SIZE_KEY_STR, ONE_ITEM_SIZE_KEY_STR_LEN), &val_size);

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
	PR("[TEST] Successfully finished basic ztable + 1 item test\n");
}

/* How many items to add into hash */
#define NUMBER_OF_ITEMS (1024 * 1024 * 10)
#define KEY_FULL_NAME_LEN (32)
static void add_many_items_test(uint32_t number_of_items)
{
	ssize_t    val_size;
	/* Allocate an empty zhash table */
	uint32_t   index;
	const char *key_base                        = "Key";
	char       key_full_name[KEY_FULL_NAME_LEN];

	/* This allocation can not fail; it if fail, the execution aborted */
	ztable_t   *zt                              = zllocate_empty_zhash();

	setlocale(LC_NUMERIC, "");

	for (index = 0; index < number_of_items; index++) {
		void *item     = malloc(ONE_ITEM_SIZE);
		void *item_ret = NULL;

		if (NULL == item) {
			DE("[TEST] Pointer of item is NULL allocation failed\n");
			abort();
		}

		if (0 == (index % (1024 * 100))) {
			PR("\r[TEST] Stress-test: adding %'u / %'u  (%.3f %%) \r", index, number_of_items, (double)index / (number_of_items)*100);
		}

		/* Fill the item with the pattern */
		memset(item, 0, ONE_ITEM_SIZE);

		const size_t key_full_name_size = snprintf(key_full_name, KEY_FULL_NAME_LEN, "%s_%u", key_base, index);
		memcpy(item, key_full_name, key_full_name_size);

		/* Add item into the hash */
		const int rc = zhash_insert_by_str(zt, key_full_name, key_full_name_size, item, ONE_ITEM_SIZE);
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

		/* Free the table */
		DDD("Going to release the hass table. Also release the values. \n");
	}

	zhash_release(zt, 1);
	PR("\n[TEST] Congrats! Successfully finished the zhash stress-test: added %u items, no collisions\n", number_of_items);
}

/* How many items to create before test zhash to buffer and back */
// #define NUMBER_OF_ITEMS_ZHASH_TO_BUF (1024)
#define NUMBER_OF_ITEMS_ZHASH_TO_BUF (1024)
/* Length of key string */

static void zhash_to_buf_and_back(void)
{
	ssize_t    val_size;
	/* Allocate an empty zhash table */
	uint32_t   index;
	const char *key_base                        = "Key";
	char       key_full_name[KEY_FULL_NAME_LEN];

	/* This allocation can not fail; it if fail, the execution aborted */
	ztable_t   *zt                              = zllocate_empty_zhash();

	ztable_t   *zt2                             = NULL;
	char       *buf                             = NULL;
	size_t     buf_size                         = 0;

	for (index = 0; index < NUMBER_OF_ITEMS_ZHASH_TO_BUF; index++) {
		void *item     = malloc(ONE_ITEM_SIZE);
		void *item_ret = NULL;

		if (NULL == item) {
			DE("[TEST] Pointer of item is NULL allocation failed\n");
			abort();
		}

		/* Fill the item with the pattern */
		memset(item, 0, ONE_ITEM_SIZE);
		const size_t key_full_name_size = snprintf(key_full_name, KEY_FULL_NAME_LEN, "%s_%u", key_base, index);
		memcpy(item, key_full_name, key_full_name_size);

		/* Add item into the hash */
		const int rc = zhash_insert_by_str(zt, key_full_name, key_full_name_size, item, ONE_ITEM_SIZE);

		if (rc < 0) {
			DE("Error: returned status < 0, aborting\n");
			abort();
		}

		if (rc > 0) {
			const void *item_ret_on_abort = zhash_find_by_str(zt, key_full_name, key_full_name_size, &val_size);
			DE("Collision: the key %s can not be inserted because of collision with: %s\n", key_full_name, (char *)item_ret_on_abort);
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
	PR("[TEST] Successfully finished zhash-to-buf and buf-to-zhash test\n");
}


/*** BASKET + BOX TESTS */


/* Allocate a basket with asked number of boxes */
static basket_t *basket_new_test(void)
{
	basket_t *basket = basket_new();

	/* Check that mbox is allocated */
	if (NULL == basket) {
		DE("[TEST] Failed allocate basket\n");
		abort();
	}

	/* Check that thre are 0 boxes are allocated */
	if (0 != basket->boxes_used) {
		DE("[TEST] Allocated basket, but wrong num of boxes: must be 0, allocated %u\n", basket->boxes_used);
		abort();
	}

	/* Check that there are real boxes */
	if (0 != basket_release(basket)) {
		DE("[TEST] Can not release basket\n");
	}

	PR("[TEST] Success: A simple 'basket create and destroy' test\n");
	return 0;
}

/*
 * Test the basket_new_from_data() function.
 * Also it tests:
 * basket_new()
 * box_data_size()
 * basket_release()
 */

static int box_new_from_data_test(void)
{
	uint32_t  boxes_in_test[]   = {0, 1, 10, 100, 1024};
	uint32_t  number_of_tests   = sizeof(boxes_in_test) / sizeof(uint32_t);
	uint32_t  i;

	basket_t  *basket           = NULL;
	size_t    buf_size_max      = strnlen(lorem_ipsum, LOREM_IPSUM_SIZE);
	size_t    internal_buf_size;
	ssize_t   buf_num;
	char      *internal_data;
	box_s64_t lorem_ipsum_size  = (box_s64_t)strnlen(lorem_ipsum, 4096);

	/* Iterate array boxes_in_test[], and use each value of this array as a number of boxes in basket */
	for (i = 0; i < number_of_tests; i++) {
		uint32_t number_of_boxes = boxes_in_test[i];
		uint32_t box_iterator;

#ifdef DEBUG3
		/* We need this variable only if extended debug is ON */
		int      basket_sz;
#endif /* DEBUG3 */


		DDD("[TEST] Starting test [%u]: %u boxes basket\n", i, number_of_boxes);

		PR3("\n============================== %.4u ========================================\n\n", i);

		/* We create an empty basket, in the next loop we add boxes one by one */
		basket = basket_new();
		if (NULL == basket) {
			DE("[TEST] Can not create a new basket\n");
			abort();
		}

		/* Iterate all boxes in the basket */
		for (box_iterator = 0; box_iterator < number_of_boxes; box_iterator++) {
			DDD("[TEST] Going to add a new data into buf[%u], new data size is %lld\n", box_iterator, lorem_ipsum_size);

			PR3("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %.4u ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n", box_iterator);

			// basket_dump(basket, "box_new_from_data_test: calling box_new_from_data()");
			DDD("[TEST] Going to call box_new_from_data() for box[%u]\n", box_iterator);
			buf_num = box_new(basket, lorem_ipsum, lorem_ipsum_size);
			if (buf_num < 0) {
				DE("[TEST] Error on adding a buffer into box[%u]\n", box_iterator);
				abort();
			}

			DDD("[TEST] Going to get data ptr for box [%u]\n", box_iterator);
			internal_data = box_data_ptr(basket, box_iterator);
			if (NULL == internal_data) {
				DE("[TEST] Can not get pointer to internal buffer for box[%u]\n", box_iterator);
			}

			internal_buf_size = box_data_size(basket, box_iterator);
			if (internal_buf_size != buf_size_max) {
				DE("[TEST] Wrong data size: lorem ipsum is %zu, data in box[%u] is %zu\n",
				   buf_size_max, box_iterator, internal_buf_size);
				abort();
			}

			if (0 != memcmp(lorem_ipsum, internal_data, buf_size_max)) {
				DE("[TEST] Data not match for box [%u]\n", box_iterator);
				abort();
			}
		}


#ifdef DEBUG3
		basket_sz = basket_memory_size(basket);
#endif /* DEBUG3 */

		basket_release(basket);


#ifdef DEBUG3
		DDD("[TEST] Success: Created, filled, tested and destroyed basket with %u boxes, size was: %d\n",
			number_of_boxes, basket_sz);
#endif /* DEBUG3 */

		PR3("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ %.4u ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n", i);
	}

	PR("[TEST] Success: Intensive 'new box from data()' test\n");
	return 0;
}
/*
 * Test the basket_new_from_data() function.
 * Also it tests:
 * basket_new()
 * box_data_size()
 * basket_release()
 */
static int box_new_from_data_simple_test(void)
{
	basket_t  *basket           = NULL;
	size_t    buf_size_max      = strnlen(lorem_ipsum, LOREM_IPSUM_SIZE);
	size_t    internal_buf_size;
	ssize_t   buf_num;
	char      *internal_data;
	box_s64_t lorem_ipsum_size  = (box_s64_t)strnlen(lorem_ipsum, 4096);
	int       basket_sz;

	/* Create an empty basket and add a new box */
	basket = basket_new();
	if (NULL == basket) {
		DE("[TEST] Can not create a new mbox\n");
		abort();
	}

	PR3("\n============================== 1 ========================================\n\n");

	/* Add 1 box */
	buf_num = box_new(basket, lorem_ipsum, lorem_ipsum_size);
	if (buf_num < 0) {
		DE("[TEST] Error on adding a buffer into Basket\n");
		abort();
	}

	PR3("\n============================== 2 ========================================\n\n");

	/* Add second box */
	buf_num = box_new(basket, lorem_ipsum, lorem_ipsum_size);
	if (buf_num < 0) {
		DE("[TEST] Error on adding a buffer into Basket\n");
		abort();
	}

	PR3("\n============================== 3 ========================================\n\n");

	DDD("[TEST] Going to get data ptr\n");
	internal_data = box_data_ptr(basket, 0);
	if (NULL == internal_data) {
		DE("[TEST] Can not get pointer to internal buffer for box %d\n", 0);
	}

	internal_buf_size = box_data_size(basket, 0);
	if (internal_buf_size != buf_size_max) {
		DE("[TEST] Wrong data size: lorem ipsum is %zu data in box %d is %zu\n",
		   buf_size_max, 0, internal_buf_size);
		abort();
	}

	if (0 != memcmp(lorem_ipsum, internal_data, buf_size_max)) {
		DE("[TEST] Data not match\n");
		abort();
	}

	basket_sz = basket_memory_size(basket);
	basket_release(basket);

	PR("[TEST] Success: Created, filled, tested and destroyed basket with %d boxes, size was: %d\n", 1, basket_sz);
	return 0;
}

/**
 * @author Sebastian Mountaniol (8/3/22)
 * @brief Insert data into a new box in the basket and validate
 *  	  that the operation succidded
 * @param basket_t* basket  Basket to insert the data
 * @param const box_u32_t box_num Expected number of the box
 * @param const char* data    
 * @param const size_t data_len
 * @return int 
 * @details 
 */
static int box_data_insert_and_validate(basket_t *basket, const box_u32_t box_num, const char *data, const size_t data_len)
{
	ret_t  rc;
	char   *data_ptr     = NULL;
	size_t data_ptr_size;
	TESTP(basket, -1);
	
	rc = box_new(basket, data, data_len);
	if (rc < 0) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	data_ptr = box_data_ptr(basket, box_num);
	// TESTP(data_ptr, -1);

	data_ptr_size = box_data_size(basket, box_num);

	if (data_ptr_size != data_len) {
		DE("[TEST] The size (%zu) of data in the box[%u] is differ from expected (%zu)\n", data_ptr_size, box_num, data_len);
		return -1;
	}

	if ((data != NULL) && (memcmp(data_ptr, data, data_len))) {
		DE("[TEST] The data in box and testes data are different\n");
		return -1;
	}

	return 0;
}

/* Create a basket, and create multiple boxes contain Alice text.
   The created basket is validated */
static basket_t *create_alice_basket(void)
{
	basket_t *basket   = NULL;

	basket = basket_new();
	if (NULL == basket) {
		DE("[TEST] Failed a basket creation\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 0, string_alice_1, strnlen(string_alice_1, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 1, string_alice_2, strnlen(string_alice_2, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 2, string_alice_3, strnlen(string_alice_3, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 3, string_alice_4, strnlen(string_alice_4, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 4, string_alice_5, strnlen(string_alice_5, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 5, string_alice_6, strnlen(string_alice_6, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 6, string_alice_7, strnlen(string_alice_7, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 7, string_alice_8, strnlen(string_alice_8, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	return basket;
}

/**
 * @author Sebastian Mountaniol (8/8/22)
 * @brief Create a basket with not sequantional boxes; see
 *  	  details
 * @return basket_t* Created basket
 * @details Not "sequantional boxes" means there are holes
 *  		between boxes. For example, box[0] contains data,
 *  		box[1] is empty (not even initialized), box[2]
 *  		contains data, box [3] exists but empty 
 */
static basket_t *create_irregular_alice_basket(void)
{
	basket_t *basket   = NULL;

	basket = basket_new();
	if (NULL == basket) {
		DE("[TEST] Failed a basket creation\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 0, string_alice_1, strnlen(string_alice_1, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box 0\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 1, NULL, 0)) {
		DE("[TEST] Failed adding a NULL/0 to a box 1\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 2, string_alice_2, strnlen(string_alice_2, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box 2\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 3, NULL, 0)) {
		DE("[TEST] Failed adding a NULL/0 to a box 1\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 4, NULL, 0)) {
		DE("[TEST] Failed adding a NULL/0 to a box 1\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 5, string_alice_3, strnlen(string_alice_3, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 6, string_alice_4, strnlen(string_alice_4, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 7, string_alice_5, strnlen(string_alice_5, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 8, NULL, 0)) {
		DE("[TEST] Failed adding a NULL/0 to a box 1\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 9, string_alice_6, strnlen(string_alice_6, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 10, string_alice_7, strnlen(string_alice_7, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 11, NULL, 0)) {
		DE("[TEST] Failed adding a NULL/0 to a box 1\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 12, string_alice_8, strnlen(string_alice_8, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	return basket;
}

static void basket_regular_collapse_in_place_test(void)
{
	ret_t    rc;
	basket_t *basket        = NULL;
	char     *box0_data_ptr;

	basket = create_alice_basket();
	if (NULL == basket) {
		DE("[TEST] Failed a basket creation\n");
		abort();
	}

	rc = basket_collapse(basket);
	if (OK != rc) {
		DE("[TEST] Failed basket collapsing in place\n");
		abort();
	}

	box0_data_ptr = box_data_ptr(basket, 0);
	if (NULL == box0_data_ptr) {
		DE("[TEST] Failed to get box[0] data ptr\n");
		abort();
	}

	if (0 != strncmp(string_alice_all, box0_data_ptr, strnlen(string_alice_all, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed compare string in box[0] with string_all\n");
		abort();
	}

	if (OK != basket_release(basket)) {
		DE("[TEST] Failed basket releasing\n");
		abort();
	}

	PR("[TEST] Success: A complex 'collapse all boxes in place' test, a regular basket\n");
}

static void basket_irregular_collapse_in_place_test(void)
{
	ret_t    rc;
	basket_t *basket        = NULL;
	char     *box0_data_ptr;

	basket = create_irregular_alice_basket();

	if (NULL == basket) {
		DE("[TEST] Failed a basket creation\n");
		abort();
	}

	rc = basket_collapse(basket);
	if (OK != rc) {
		DE("[TEST] Failed basket collapsing in place\n");
		abort();
	}

	box0_data_ptr = box_data_ptr(basket, 0);
	if (NULL == box0_data_ptr) {
		DE("[TEST] Failed to get box[0] data ptr\n");
		abort();
	}

	if (0 != strncmp(string_alice_all, box0_data_ptr, strnlen(string_alice_all, STRING_ALICE_ALL_LEN))) {
		DE("[TEST] Failed compare string in box[0] with string_all\n");
		abort();
	}

	if (OK != basket_release(basket)) {
		DE("[TEST] Failed basket releasing\n");
		abort();
	}

	PR("[TEST] Success: A complex 'collapse all boxes in place' test, an irregular basket\n");
}

/**
 * @author Sebastian Mountaniol (8/3/22)
 * @brief This function: <br>
 * 1. Creates a basket, add strings into boxes <br>
 * 2. Creates a flat memory buffer from the basket <br>
 * 3. Restore a basket from the flat memory buffer <br>
 * 4. Compare the original and restored baskets <br>
 * If these two buffers are equal, the test considered as
 * auccessful
 * @details 
 */
void basket_regular_to_buf_test(void)
{
	ret_t    rc;
	basket_t *basket       = NULL;
	basket_t *basket_2     = NULL;
	char     *flat_buf;
	size_t   flat_buf_size;

	basket = create_alice_basket();
	if (NULL == basket) {
		DE("[TEST] Failed a basket creation\n");
		abort();
	}

	flat_buf = basket_to_buf(basket, &flat_buf_size);
	if (NULL == flat_buf) {
		DE("[TEST] Can not create a flat memory buffer from basket\n");
		abort();
	}

	DDD("[TEST] Created a flat memory buffer from 'Alice' basket, size of basket = %zu, size of buf %zu, size of Alice text is %zu, overhead of the basket = %zu\n",
		basket_memory_size(basket), flat_buf_size, strnlen(string_alice_all, STRING_ALICE_ALL_LEN), flat_buf_size - strnlen(string_alice_all, STRING_ALICE_ALL_LEN));

	basket_2 = basket_from_buf(flat_buf, flat_buf_size);
	if (NULL == basket_2) {
		DE("[TEST] Can not create a basket from flat memory buffer\n");
		abort();
	}

	free(flat_buf);

	if (basket_compare_basket(basket, basket_2)) {
		DE("[TEST] Original basket and the restored basket are not the same\n");
		abort();
	}

	rc = basket_release(basket);
	if (0 != rc) {
		DE("[TEST] Can not release the original basket\n");
		abort();
	}

	rc = basket_release(basket_2);
	if (0 != rc) {
		DE("[TEST] Can not release the restored basket\n");
		abort();
	}

	PR("[TEST] Success: 'Basket to Flat Memory Buffer' and 'Flat Memory Buffer to Basket' test, a regular basket\n");
}

void basket_irregular_to_buf_test(void)
{
	ret_t    rc;
	basket_t *basket       = NULL;
	basket_t *basket_2     = NULL;
	char     *flat_buf;
	size_t   flat_buf_size;

	basket = create_irregular_alice_basket();

	if (NULL == basket) {
		DE("[TEST] Failed a basket creation\n");
		abort();
	}

	flat_buf = basket_to_buf(basket, &flat_buf_size);
	if (NULL == flat_buf) {
		DE("[TEST] Can not create a flat memory buffer from basket\n");
		abort();
	}

	DDD("[TEST] Created a flat memory buffer from 'Alice' basket, size of basket = %zu, size of buf %zu, size of Alice text is %zu, overhead of the basket = %zu\n",
		basket_memory_size(basket), flat_buf_size, strnlen(string_alice_all, STRING_ALICE_ALL_LEN), flat_buf_size - strnlen(string_alice_all, STRING_ALICE_ALL_LEN));

	basket_2 = basket_from_buf(flat_buf, flat_buf_size);
	if (NULL == basket_2) {
		DE("[TEST] Can not create a basket from flat memory buffer\n");
		abort();
	}

	free(flat_buf);

	if (basket_compare_basket(basket, basket_2)) {
		DE("[TEST] Original basket and the restored basket are not the same\n");
		abort();
	}

	rc = basket_release(basket);
	if (0 != rc) {
		DE("[TEST] Can not release the original basket\n");
		abort();
	}

	rc = basket_release(basket_2);
	if (0 != rc) {
		DE("[TEST] Can not release the restored basket\n");
		abort();
	}

	PR("[TEST] Success: 'Basket to Flat Memory Buffer' and 'Flat Memory Buffer to Basket' test, an irregular basket\n");
}

#define STR_KEY_LEN (32)
#define HOW_MANY_KEYVALUE_ENTRIES (1024)
void basket_test_keyval(void)
{
	char       key_str[STR_KEY_LEN];
	size_t     key_str_len;
	char       *val;
	ssize_t    val_size;
	const char *key_str_base        = "Key";

	int        rc;
	uint32_t   index;
	basket_t   *basket              = NULL;
	basket_t   *basket_2            = NULL;
	char       *flat_buf;
	size_t     flat_buf_size;

	/*** Create a basket ***/
	basket = basket_new();
	if (NULL == basket) {
		DE("[TEST] Failed a basket creation\n");
		abort();
	}

	/*** Add key/values ***/

	/* Insert HOW_MANY_KEYVALUE_ENTRIES into key/val repository of the basket */
	for (index = 0; index < HOW_MANY_KEYVALUE_ENTRIES; index++) {
		/* Clean string */
		memset(key_str, 0, STR_KEY_LEN);
		key_str_len = snprintf(key_str, STR_KEY_LEN, "%s_%u", key_str_base, index);

		val = strndup(key_str, key_str_len);
		val_size = strnlen(val, STR_KEY_LEN) + 1;
		rc = basket_keyval_add_by_str(basket, key_str, key_str_len, val, val_size);
		if (rc != 0) {
			DE("[TEST] Can not add key/val: %s/%p, size %zd, ret value: %d\n", key_str, val, val_size, rc);
		}

		DDD("Added: key(%s)/size(%zu) val(%s)/size(%zd)\n", key_str, key_str_len, val, val_size);
	}

	/*** Validate key/values ***/

	for (index = 0; index < HOW_MANY_KEYVALUE_ENTRIES; index++) {
		char *found;
		/* Clean string */
		memset(key_str, 0, STR_KEY_LEN);
		key_str_len = snprintf(key_str, STR_KEY_LEN, "%s_%u", key_str_base, index);
		found = basket_keyval_find_by_str(basket, key_str, key_str_len, &val_size);
		if (NULL == found) {
			DE("[TEST] Can not find value by key: %s\n", key_str);
			abort();
		}

		DDD("Found: key(%s)/size(%zu) val(%s)/size(%zd)\n", key_str, key_str_len, found, val_size);

		if (val_size != ((ssize_t)strnlen(found, STR_KEY_LEN) + 1)) {
			DE("[TEST] Size not match for key/value: %s/%s, expected %zu but it is %zd\n",
			   key_str, val, strnlen(found, STR_KEY_LEN), val_size);
			abort();
		}
	}

	/*** Create a flat buffer from the basket ***/
	flat_buf = basket_to_buf(basket, &flat_buf_size);

	if (NULL == flat_buf) {
		DE("[TEST] Can not create a flat memory buffer from basket\n");
		abort();
	}

	DDD("[TEST] Created a flat memory buffer from 'Alice' basket, size of basket = %zu, size of buf %zu, size of Alice text is %zu, overhead of the basket = %zu\n",
		basket_memory_size(basket), flat_buf_size, strnlen(string_alice_all, STRING_ALICE_ALL_LEN), flat_buf_size - strnlen(string_alice_all, STRING_ALICE_ALL_LEN));

	basket_2 = basket_from_buf(flat_buf, flat_buf_size);
	if (NULL == basket_2) {
		DE("[TEST] Can not create a basket from flat memory buffer\n");
		abort();
	}

	free(flat_buf);

	if (basket_compare_basket(basket, basket_2)) {
		DE("[TEST] Original basket and the restored basket are not the same\n");
		abort();
	}

	/*** Validate key/values for the new basket ***/

	for (index = 0; index < HOW_MANY_KEYVALUE_ENTRIES; index++) {
		char *found;
		/* Clean string */
		memset(key_str, 0, STR_KEY_LEN);
		key_str_len = snprintf(key_str, STR_KEY_LEN, "%s_%u", key_str_base, index);
		found = basket_keyval_find_by_str(basket_2, key_str, key_str_len, &val_size);
		if (NULL == found) {
			DE("[TEST] Can not find value by key: %s\n", key_str);
			abort();
		}

		DDD("Found: key(%s)/size(%zu) val(%s)/size(%zd)\n", key_str, key_str_len, found, val_size);

		if (val_size != ((ssize_t)strnlen(found, STR_KEY_LEN) + 1)) {
			DE("[TEST] Size not match for key/value: %s/%s, expected %zu but it is %zd\n",
			   key_str, val, strnlen(found, STR_KEY_LEN), val_size);
			zhash_dump(basket->zhash, "ORIGINAL ZHASH");
			abort();
		}
	}

	rc = basket_release(basket);
	if (0 != rc) {
		DE("[TEST] Can not release the original basket\n");
		abort();
	}

	rc = basket_release(basket_2);
	if (0 != rc) {
		DE("[TEST] Can not release the restored basket\n");
		abort();
	}

	PR("[TEST] Success: Key/Value test\n");
}

int main(void)
{
	PR("\nSECTION 1: ZHASH\n");
	/* zhash tests */
	basic_test();
	add_one_item_test();
	zhash_to_buf_and_back();
	add_many_items_test(1000);
	add_many_items_test(1024 * 1024 * 10);
	//add_many_items_test();

	/* Basket and Box tests */
	PR("\nSECTION 2: BOX\n");
	box_new_from_data_simple_test();
	box_new_from_data_test();

	PR("\nSECTION 3: BASKET, REGULAR\n");
	basket_new_test();
	basket_regular_collapse_in_place_test();
	basket_regular_to_buf_test();

	PR("\nSECTION 4: BASKET, IRREGULAR\n");
	basket_irregular_collapse_in_place_test();
	basket_irregular_to_buf_test();

	PR("\nSECTION 5: BASKET, KEY/VALUE\n");
	basket_test_keyval();

	return 0;
}
