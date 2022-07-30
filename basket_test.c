#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "basket.h"
#include "box_t.h"
#include "debug.h"

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

/* Allocate a basket with asked number of boxes */
static basket_t *basket_new_test(void)
{
	basket_t *basket = basket_new();

	/* Check that mbox is allocated */
	if (NULL == basket) {
		DE("Failed allocate basket\n");
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
	size_t    buf_size_max      = strlen(lorem_ipsum);
	size_t    internal_buf_size;
	ssize_t   buf_num;
	char      *internal_data;
	box_s64_t lorem_ipsum_size  = (box_s64_t)strnlen(lorem_ipsum, 4096);
	#ifdef DEBUG3
	/* We need this variable only if extended debug is ON */
	int       basket_sz;
	#endif /* DEBUG3 */

	/* Iterate array boxes_in_test[], and use each value of this array as a number of boxes in basket */
	for (i = 0; i < number_of_tests; i++) {
		uint32_t number_of_boxes = boxes_in_test[i];
		uint32_t box_iterator;

		DDD("[TEST] Starting test [%d]: %u boxes basket\n", i, number_of_boxes);

		PR3("\n============================== %.4d ========================================\n\n", i);

		/* We create an empty basket, in the next loop we add boxes one by one */
		basket = basket_new();
		if (NULL == basket) {
			DE("[TEST] Can not create a new basket\n");
			abort();
		}

		/* Iterate all boxes in the basket */
		for (box_iterator = 0; box_iterator < number_of_boxes; box_iterator++) {
			DDD("[TEST] Going to add a new data into buf[%u], new data size is %zu\n", box_iterator, lorem_ipsum_size);

			PR3("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %.4d ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n", box_iterator);

			// basket_dump(basket, "box_new_from_data_test: calling box_new_from_data()");
			DDD("[TEST] Going to call box_new_from_data() for box[%u]\n", box_iterator);
			buf_num = box_new(basket, lorem_ipsum, lorem_ipsum_size);
			if (buf_num < 0) {
				DE("[TEST] Error on adding a buffer into box[%u]\n", box_iterator);
				abort();
			}

			DDD("[TEST] Going to get data ptr for box [%d]\n", box_iterator);
			internal_data = box_data_ptr(basket, box_iterator);
			if (NULL == internal_data) {
				DE("[TEST] Can not get pointer to internal buffer for box[%u]\n", box_iterator);
			}

			internal_buf_size = box_data_size(basket, box_iterator);
			if (internal_buf_size != buf_size_max) {
				DE("[TEST] Wrong data size: lorem ipsum is %ld, data in box[%d] is %ld\n",
				   buf_size_max, box_iterator, internal_buf_size);
				abort();
			}

			if (0 != memcmp(lorem_ipsum, internal_data, buf_size_max)) {
				DE("[TEST] Data not match for box [%u]\n", box_iterator);
				abort();
			}
		}


		#ifdef DEBUG3
		basket_sz = basket_size(basket);
		#endif /* DEBUG3 */

		basket_release(basket);
		DDD("[TEST] Success: Created, filled, tested and destroyed basket with %u boxes, size was: %d\n",
			number_of_boxes, basket_sz);

		PR3("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ %.4d ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n", i);
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
	size_t    buf_size_max      = strlen(lorem_ipsum);
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
		DE("[TEST] Can not get pointer to internal buffer for box %u\n", 0);
	}

	internal_buf_size = box_data_size(basket, 0);
	if (internal_buf_size != buf_size_max) {
		DE("[TEST] Wrong data size: lorem ipsum is %ld, data in box %d is %ld\n",
		   buf_size_max, 0, internal_buf_size);
		abort();
	}

	if (0 != memcmp(lorem_ipsum, internal_data, buf_size_max)) {
		DE("[TEST] Data not match\n");
		abort();
	}

	basket_sz = basket_size(basket);
	basket_release(basket);

	PR("[TEST] Success: Created, filled, tested and destroyed basket with %u boxes, size was: %d\n", 1, basket_sz);
	return 0;
}

static int box_data_insert_and_validate(basket_t *basket, const box_u32_t box_num, const char *data, const size_t data_len)
{
	ret_t  rc;
	char   *data_ptr     = NULL;
	size_t data_ptr_size;
	TESTP(basket, -1);
	TESTP(data, -1);

	rc = box_new(basket, data, data_len);
	if (rc < 0) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	data_ptr = box_data_ptr(basket, box_num);
	TESTP(data_ptr, -1);

	data_ptr_size = box_data_size(basket, box_num);
	if (data_ptr_size != data_len) {
		DE("[TEST] The size (%lu) of data in the box[%u] is differ from expected (%lu)\n", data_ptr_size, box_num, data_len);
		return -1;
	}

	if (memcmp(data_ptr, data, data_len)) {
		DE("[TEST] The data in box and testes data are differern\n");
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

	if (0 != box_data_insert_and_validate(basket, 0, string_alice_1, strlen(string_alice_1))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 1, string_alice_2, strlen(string_alice_2))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 2, string_alice_3, strlen(string_alice_3))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 3, string_alice_4, strlen(string_alice_4))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 4, string_alice_5, strlen(string_alice_5))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 5, string_alice_6, strlen(string_alice_6))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 6, string_alice_7, strlen(string_alice_7))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	if (0 != box_data_insert_and_validate(basket, 7, string_alice_8, strlen(string_alice_8))) {
		DE("[TEST] Failed adding a string to a box\n");
		abort();
	}

	return basket;
}

static void basket_collapse_in_place_test(void)
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

	if (0 != strncmp(string_alice_all, box0_data_ptr, strnlen(string_alice_all, 1024))) {
		DE("[TEST] Failed compare string in box[0] with string_all\n");
		abort();
	}

	if (OK != basket_release(basket)) {
		DE("[TEST] Failed basket releasing\n");
		abort();
	}

	PR("[TEST] Success: A complex 'collapse all boxes in place' test\n");
}

void basket_to_buf_test(void)
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

	DDD("[TEST] Created a flat memory buffer from 'Alice' basket, size of basket = %lu, size of buf %zu, size of Alice text is %zu, overhead of the basket = %zu\n",
		basket_size(basket), flat_buf_size, strlen(string_alice_all), flat_buf_size - strlen(string_alice_all));

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

	PR("[TEST] Success: 'Basket to Flat Memory Buffer' and 'Flat Memory Buffer to Basket' test\n");
}

int main(void)
{
	basket_new_test();
	box_new_from_data_simple_test();
	box_new_from_data_test();
	basket_collapse_in_place_test();
	basket_to_buf_test();
}

