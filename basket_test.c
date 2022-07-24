#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "basket.h"
#include "buf_t.h"
#include "debug.h"

const char *lorem_ipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\0";

/* Allocate a basket with asked number of boxes */
static basket_t *basket_new_and_several_boxes(uint32_t num)
{
	uint32_t i;
	basket_t *basket = basket_new();

	/* Check that mbox is allocated */
	if (NULL == basket) {
		DE("Failed allocate mbox with %u boxes, got NULL pointer\n", num);
		abort();
	}

	/* Check that asked number of boxes are allocated */
	if (num != basket_get_boxes_count(basket)) {
		DE("[TEST] Allocated mbox, but wrong num of boxes: asked %u, allocated %ld\n",
		   num, basket_get_boxes_count(basket));
		abort();
	}

	/* Check that there are real boxes */
	for (i = 0; i < num; i++) {
		buf_t  *box = basket_get_box(basket, i);
		if (NULL == box) {
			DD("[TEST] Box %u of %u is a NULL pointer\n", i, num);
			continue;
		}

		/* Tests that the box is properly allocated: it is buf_t, test that all fields are 0 */
		if (buf_used_take(box) || buf_room_take(box) || buf_data_take(box)) {
			DE("[TEST] Box %u is not properly inited, all must be 0/NULL: mb->bufs[i]->used = %ld, mb->bufs[i]->room = %ld, mb->bufs[i]->data = %p\n",
			   i, buf_used_take(box), buf_room_take(box), buf_data_take(box));
		}
	}

	return basket;
}

/* Test basket_new() function */
static int basket_new_test(void)
{
	basket_t *basket         = NULL;
	uint32_t boxes_in_test[] = {0, 1, 10, 100, 1024};
	uint32_t number_of_tests = sizeof(boxes_in_test) / sizeof(uint32_t);
	uint32_t i;

	for (i = 0; i < number_of_tests; i++) {
		/* 1. Allocate an empty mbox */
		basket = basket_new_and_several_boxes(boxes_in_test[i]);
		DD("[TEST] Success: allocated mbox with %u boxes\n", boxes_in_test[i]);
		basket_release(basket);
	}
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
	buf_s64_t lorem_ipsum_size  = (buf_s64_t)strnlen(lorem_ipsum, 4096);
	int       basket_sz;

	/* Iterate array boxes_in_test[], and use each value of this array as a number of boxes in basket */
	for (i = 0; i < number_of_tests; i++) {
		uint32_t number_of_boxes = boxes_in_test[i];
		uint32_t box_iterator;

		DDD("[TEST] Starting test [%d]: %u boxes basket\n", i, number_of_boxes);

		PR("\n============================== %.4d ========================================\n\n", i);

		/* We create an empty basket, in the next loop we add boxes one by one */
		basket = basket_new();
		if (NULL == basket) {
			DE("[TEST] Can not create a new basket\n");
			abort();
		}

		/* Iterate all boxes in the basket */
		for (box_iterator = 0; box_iterator < number_of_boxes; box_iterator++) {
			DD("[TEST] Going to add a new data into buf[%u], new data size is %zu\n", box_iterator, lorem_ipsum_size);

			PR("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %.4d ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n", box_iterator);

			// basket_dump(basket, "box_new_from_data_test: calling box_new_from_data()");
			DD("[TEST] Going to call box_new_from_data() for box[%u]\n", box_iterator);
			buf_num = box_new_from_data(basket, lorem_ipsum, lorem_ipsum_size);
			if (buf_num < 0) {
				DE("[TEST] Error on adding a buffer into box[%u]\n", box_iterator);
				abort();
			}

			DD("[TEST] Going to get data ptr for box [%d]\n", box_iterator);
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

		basket_sz = basket_size(basket);
		basket_release(basket);
		DD("[TEST] Success: Created, filled, tested and destroyed basket with %u boxes, size was: %d\n",
		   number_of_boxes, basket_sz);

		PR("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ %.4d ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n", i);
	}
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
	buf_s64_t lorem_ipsum_size  = (buf_s64_t)strnlen(lorem_ipsum, 4096);
	int       basket_sz;

	/* Create an empty basket and add a new box */
	basket = basket_new();
	if (NULL == basket) {
		DE("[TEST] Can not create a new mbox\n");
		abort();
	}

	PR("\n============================== 1 ========================================\n\n");

	/* Add 1 box */
	buf_num = box_new_from_data(basket, lorem_ipsum, lorem_ipsum_size);
	if (buf_num < 0) {
		DE("[TEST] Error on adding a buffer into Basket\n");
		abort();
	}

	PR("\n============================== 2 ========================================\n\n");

	/* Add second box */
	buf_num = box_new_from_data(basket, lorem_ipsum, lorem_ipsum_size);
	if (buf_num < 0) {
		DE("[TEST] Error on adding a buffer into Basket\n");
		abort();
	}

	PR("\n============================== 3 ========================================\n\n");

	DD("[TEST] Going to get data ptr\n");
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
	DD("[TEST] Success: Created, filled, tested and destroyed basket with %u boxes, size was: %d\n",
	   1, basket_sz);
	return 0;
}

int main(void)
{
	// basket_new_test();
	//box_new_from_data_simple_test();
	box_new_from_data_test();
}

