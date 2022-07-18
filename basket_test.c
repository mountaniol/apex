#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "basket.h"
#include "buf_t.h"
#include "debug.h"

/* Allocate a mbox asked number of boxes */
static basket_t *basket_new_several_test(uint32_t num)
{
	uint32_t i;
	basket_t   *basket = basket_new(num);

	/* Check that mbox is allocated */
	if (NULL == basket) {
		DE("Failed allocate mbox with %u boxes, got NULL pointer\n", num);
		abort();
	}

	/* Check that asked number of boxes are allocated */
	if (num != basket->bufs_num) {
		DE("Allocated mbox, but wrong num of boxes: asked %u, allocated %lu\n", num, basket->bufs_num);
		abort();
	}

	/* Check that there are real boxes */
	for (i = 0; i < num; i++) {
		if (NULL == basket->bufs[i]) {
			DE("Box %u of %u is a NULL pointer\n", i, num);
			abort();
		}

		/* Tests that the box is properly allocated: it is buf_t, test that all fields are 0 */
		if (basket->bufs[i]->used != 0 ||
			basket->bufs[i]->room != 0 ||
			basket->bufs[i]->data != NULL) {
			DE("Box %u is not properly inited, all must bu 0/NULL: mb->bufs[i]->used = %ld, mb->bufs[i]->room = %ld, mb->bufs[i]->data = %p\n",
			   i, basket->bufs[i]->used, basket->bufs[i]->room, basket->bufs[i]->data);
		}
	}

	return basket;
}

/* Test mbox allocation */
static int basket_new_test(void)
{
	basket_t   *basket             = NULL;
	uint32_t boxes[]         = {0, 1, 10, 100, 1024};
	uint32_t number_of_tests = sizeof(boxes) / sizeof(uint32_t);
	uint32_t i;

	for (i = 0; i < number_of_tests; i++) {
		/* 1. Allocate an empty mbox */
		basket = basket_new_several_test(boxes[i]);
		DD("Success: allocated mbox with %u boxes\n", boxes[i]);
		basket_release(basket);
	}
	return 0;
}

char *lorem_ipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
/* Test the mbox_box_new() function */
static int basket_box_new_test(void)
{
	uint32_t boxes[]         = {0, 1, 10, 100, 1024};
	uint32_t number_of_tests = sizeof(boxes) / sizeof(uint32_t);
	uint32_t i;

	basket_t *mb = NULL;
	size_t buf_size_max = strlen(lorem_ipsum);
	//size_t buf_size;
	size_t internal_buf_size;
	ssize_t buf_num;
	char *internal_data;
	mb = basket_new(0);
	if (NULL == mb) {
		DE("Can not create a new mbox\n");
		abort();
	}

	buf_num = box_new_from_data(mb, lorem_ipsum, buf_size_max);
	if (buf_num < 0) {
		DE("Error on adding a buffer into Mbox\n");
		abort();
	}

	internal_data = box_data_ptr(mb, buf_num);
	if (NULL == internal_data) {
		DE("Can not get pointer to internal buffer for box %ld\n", buf_num);
	}

	internal_buf_size = box_data_size(mb, buf_num);
	if (internal_buf_size != buf_size_max) {
		DE("Wrong data size: lorem ipsum is %ld, data in box is %ld\n", buf_size_max, internal_buf_size);
		abort();
	}

	if (0 != memcmp(lorem_ipsum, internal_data, buf_size_max)) {
		DE("Data not match\n");
		abort();
	}

	basket_release(mb);
	return 0;
}


int main(void)
{
	basket_new_test();
	basket_box_new_test();
}

