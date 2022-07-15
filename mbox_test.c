#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mbox.h"
#include "buf_t.h"
#include "debug.h"

/* Allocate a mbox asked number of boxes */
static mbox_t *mbox_new_several_test(uint32_t num)
{
	uint32_t i;
	mbox_t   *mb = mbox_new(num);

	/* Check that mbox is allocated */
	if (NULL == mb) {
		DE("Failed allocate mbox with %u boxes, got NULL pointer\n", num);
		abort();
	}

	/* Check that asked number of boxes are allocated */
	if (num != mb->bufs_num) {
		DE("Allocated mbox, but wrong num of boxes: asked %u, allocated %lu\n", num, mb->bufs_num);
		abort();
	}

	/* Check that there are real boxes */
	for (i = 0; i < num; i++) {
		if (NULL == mb->bufs[i]) {
			DE("Box %u of %u is NULL pointer\n", i, num);
			abort();
		}

		/* Tests that the box is properly allocated: it is buf_t, test that all fields are 0 */
		if (mb->bufs[i]->used != 0 || mb->bufs[i]->room != 0 || mb->bufs[i]->data != NULL) {
			DE("Box %u is not properly inited, all must bu 0/NULL: mb->bufs[i]->used = %ld, mb->bufs[i]->room = %ld, mb->bufs[i]->data = %p\n",
			   i, mb->bufs[i]->used, mb->bufs[i]->room, mb->bufs[i]->data);
		}
	}

	return mb;
}

/* Test mbox allocation */
static int mbox_new_test(void)
{
	mbox_t   *mb             = NULL;
	uint32_t boxes[]         = {0, 1, 10, 100, 1024};
	uint32_t number_of_tests = sizeof(boxes) / sizeof(uint32_t);
	uint32_t i;

	for (i = 0; i < number_of_tests; i++) {
		/* 1. Allocate an empty mbox */
		mb = mbox_new_several_test(boxes[i]);
		DD("Success: allocated mbox with %u boxes\n", boxes[i]);
		mbox_free(mb);
	}
	return 0;
}

char *lorem_ipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
/* Test the mbox_box_new() function */
static int mbox_box_new_test(void)
{
	mbox_t *mb = NULL;
	size_t buf_size_max = strlen(lorem_ipsum);
	//size_t buf_size;
	size_t internal_buf_size;
	ssize_t buf_num;
	char *internal_data;
	mb = mbox_new(0);
	if (NULL == mb) {
		DE("Can not create a new mbox\n");
		abort();
	}

	buf_num = mbox_box_new(mb, lorem_ipsum, buf_size_max);
	if (buf_num < 0) {
		DE("Error on adding a buffer into Mbox\n");
		abort();
	}

	internal_data = mbox_box_ptr(mb, buf_num);
	if (NULL == internal_data) {
		DE("Can not get pointer to internal buffer for box %ld\n", buf_num);
	}

	internal_buf_size = mbox_box_size(mb, buf_num);
	if (internal_buf_size != buf_size_max) {
		DE("Wrong data size: lorem ipsum is %ld, data in box is %ld\n", buf_size_max, internal_buf_size);
		abort();
	}

	if (0 != memcmp(lorem_ipsum, internal_data, buf_size_max)) {
		DE("Data not match\n");
		abort();
	}

	mbox_free(mb);
	return 0;
}


int main(void)
{
	mbox_new_test();
	mbox_box_new_test();
}

