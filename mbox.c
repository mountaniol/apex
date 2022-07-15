#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mbox.h"
#include "buf_t.h"
#include "debug.h"

static mbox_t *mbox_t_alloc(void)
{
	mbox_t *mbox = malloc(sizeof(mbox_t));
	if (NULL == mbox) {
		DE("Can't allocate\n");
		return (NULL);
	}

	memset(mbox, 0, sizeof(mbox_t));
	return (mbox);
}

static void mbox_t_release(mbox_t *mbox)
{
	if (NULL == mbox) {
		DE("Arg is NULL\n");
		return;
	}

	/* Secure way: clear memory before release it */
	memset(mbox, 0, sizeof(mbox_t));
	free(mbox);
}

/* Allocate new Mbox object */
mbox_t *mbox_new(uint32_t number_of_boxes)
{
	uint32_t i;
	mbox_t   *mbox = mbox_t_alloc();
	TESTP(mbox, NULL);

	if (0 == number_of_boxes) {
		return mbox;
	}

	mbox->bufs = malloc(sizeof(buf_t *) * number_of_boxes);
	if (NULL == mbox->bufs) {
		DE("Can not allocate pointers array for %u boxes\n", number_of_boxes);
		mbox_t_release(mbox);
		return NULL;
	}

	for (i = 0; i < number_of_boxes; i++) {
		mbox->bufs[i] = buf_new(0);
		if (NULL == mbox->bufs[i]) {
			DE("Can not allocate boxe number %u of %u\n", i, number_of_boxes);
			mbox_t_release(mbox);
			return NULL;
		}
	}
	mbox->bufs_num = number_of_boxes;
	return mbox;
}

/* Clean all boxes */
ret_t mbox_clean(mbox_t *mbox)
{
	TESTP(mbox, -1);
	if (mbox->bufs && (mbox->bufs_num > 0)) {
		uint32_t i;
		DDD("Cleaning mbox->bufs_num boxes\n");
		for (i = 0; i < mbox->bufs_num; i++) {
			buf_free(mbox->bufs[i]);
		}
		free(mbox->bufs);
	} else {
		DDD("Not cleaning boxes\n");
	}

	mbox->bufs_num = 0;
	return 0;
}

/* Free Mbox */
ret_t mbox_free(mbox_t *mbox)
{
	TESTP(mbox, -1);
	if (0 != mbox_clean(mbox)) {
		return -1;
	}

	mbox_t_release(mbox);
	return 0;
}

/* Add more boxes */
ret_t mbox_boxes_add(mbox_t *mbox, size_t num)
{
	size_t i;
	TESTP(mbox, -1);
	size_t new_size = mbox->bufs_num + num;
	new_size *= sizeof(buf_t *);

	buf_t **new_bufs = realloc(mbox->bufs, new_size);

	/* Can not reallocate */
	if (NULL == new_bufs) {
		DE("Realloc failed\n");
		return (-ENOMEM);
	}

	/* Case 2: realloc succeeded, new memory returned */
	/* No need to free the old memory - done by realloc */
	if (mbox->bufs != new_bufs) {
		free(mbox->bufs);
		mbox->bufs = new_bufs;
	}

	/* Now allocate boxes */
	for (i = mbox->bufs_num; i < mbox->bufs_num + num; i++) {
		mbox->bufs[i] = buf_new(0);
		if (NULL == mbox->bufs[i]) {
			DE("Could not add new buf_t at index %lu\n", i);
		}
	}
	mbox->bufs_num += num;

	/* Case 3: Reallocated, and the same pointer, no need to do anything */
	return 0;
}

ssize_t mbox_boxes_count(mbox_t *mbox)
{
	return mbox->bufs_num;
}

ret_t mbox_boxes_insert_after(mbox_t *mbox, uint32_t after, uint32_t num)
{
	ret_t    rc;
	size_t   i;
	size_t   how_many_to_move;
	uint32_t move_start;
	uint32_t move_end;

	buf_t    *move_start_p;
	buf_t    *move_end_p;

	/* 1. Reallocate the number of boxes, add one more */


	how_many_to_move = mbox->bufs_num - (after + 1) * sizeof(buf_t *);
	move_start = after + 1;
	move_end = after + 1 + num;

	rc = mbox_boxes_add(mbox, num);
	if (0 != rc) {
		return -1;
	}
	/* 2. Shift boxes after the 'after' */

	//move_start_p = mbox->bufs + (sizeof(buf_t *) * move_start);
	move_start_p = mbox->bufs[move_start];
	//move_end_p = mbox->bufs + (sizeof(buf_t *) * move_end);
	move_end_p = mbox->bufs[move_end];

	memmove(move_start_p, move_end_p, how_many_to_move);

	/* 3. Now add a new box */
	for (i = 0; i < num; i++) {
		mbox->bufs[after + i] = buf_new(0);
		if (NULL == mbox->bufs[after + i]) {
			return -1;
		}
	}

	return 0;
}

ret_t mbox_boxes_swap(mbox_t *mbox, size_t first, size_t second)
{
	buf_t *tmp;
	TESTP(mbox, -1);
	if (first > (size_t) mbox->bufs_num || second > (size_t) mbox->bufs_num) {
		return -1;
	}

	tmp = mbox->bufs[first];
	mbox->bufs[first] = mbox->bufs[second];
	mbox->bufs[second] = tmp;
	return 0;
}


ret_t mbox_box_remove(mbox_t *mbox, size_t num)
{
	TESTP(mbox, -1);
	if (num > (size_t) mbox->bufs_num) {
		return -1;
	}

	if (NULL == mbox->bufs[num]) {
		return -1;
	}

	return buf_clean_and_reset(mbox->bufs[num]);
}

ret_t mbox_boxes_merge(mbox_t *mbox, size_t src, size_t dst)
{
	ret_t rc;
	TESTP(mbox, -1);
	if (src > (size_t) mbox->bufs_num || dst > (size_t) mbox->bufs_num) {
		return -1;
	}

	rc = buf_add(mbox->bufs[dst], mbox->bufs[src]->data, mbox->bufs[src]->used);
	if (rc) {
		return rc;
	}

	rc = buf_clean_and_reset(mbox->bufs[src]);
	if (rc) {
		return rc;
	}

	return 0;
}

ret_t mbox_box_bisect(__attribute__((unused)) mbox_t *mbox,
					  __attribute__((unused))  size_t box_num,
					  __attribute__((unused)) size_t from_offset)
{
	return -1;
	/* TODO */
}

ret_t mbox_boxes_collapse_in_place(__attribute__((unused)) mbox_t *mbox)
{
	return -1;
	/* TODO */
}

buf_t *mbox_to_buf(__attribute__((unused)) mbox_t *mbox)
{
	return NULL;
	/* TODO */
}

mbox_t *mbox_from_buf(__attribute__((unused)) buf_t *buf)
{
	return NULL;
	/* TODO */
}

/* Create a new mbox and add data */
ssize_t mbox_box_new(mbox_t *mbox, void *buffer, size_t buffer_size)
{
	TESTP(mbox, -1);
	TESTP(buffer, -1);

	/* Add a new box */
	if (0 != mbox_boxes_add(mbox,1)) {
		DE("Can not add another box into Mbox\n");
		return -1;
	}

	DDD("Now number of boxes: %lu\n", mbox->bufs_num);

	if(0 != mbox_box_add(mbox, mbox->bufs_num - 1, buffer, buffer_size)) {
		DE("Could not add data into a new box %lu\n", mbox->bufs_num - 1);
	}

	/* Return the number of the new box */
	return (mbox->bufs_num - 1);
}

/* Return 1 if the box valid, 0 otherwise, -1 on error;
   It does not tests that the box created, only that the box index is in range */
static int mbox_if_box_index_valid(mbox_t *mbox, buf_s64_t box_num)
{
	TESTP(mbox, -1);
	if (0 == mbox->bufs_num) {
		return 0;
	}

	if (box_num > (mbox->bufs_num - 1)) {
		return 0;
	}

	return 1;
}

/* Tests that there the index of the box valid AND is a box by the given index. */
static int mbox_if_box_valid(mbox_t *mbox, buf_s64_t box_num)
{
	if (1 != mbox_if_box_index_valid(mbox,box_num)) {
		return 0;
	}

	if (NULL == mbox->bufs[box_num]) {
		return 0;
	}

	return 1;
}

/* Add data to a mbox */
ret_t mbox_box_add(mbox_t *mbox, buf_s64_t box_num, void *buffer, size_t buffer_size)
{
	TESTP(mbox, -1);
	TESTP(buffer, -1);

	if (1 != mbox_if_box_valid(mbox, box_num)) {
		DE("Box index is out of range\n");
		return -1;
	}

	return buf_add(mbox->bufs[box_num], buffer, buffer_size);
}

ret_t mbox_box_replace(__attribute__((unused)) mbox_t *mbox, 
					   __attribute__((unused)) size_t box_num, 
					   __attribute__((unused)) void *buffer, 
					   __attribute__((unused)) size_t buffer_size)
{
	return -1;
	/* TODO */
}

void *mbox_box_ptr(mbox_t *mbox, size_t box_num)
{
	TESTP(mbox, NULL);
	TESTP(mbox->bufs, NULL);

	if ((buf_s64_t) box_num > mbox->bufs_num) {
		DE("Asked box is out of range\n");
		return NULL;
	}

	if (NULL == mbox->bufs[box_num]) {
		DE("There is no buffer by asked index: %zu\n", box_num);
		return NULL;
	}

	return mbox->bufs[box_num]->data;
}

ssize_t mbox_box_size(mbox_t *mbox, size_t box_num)
{
	TESTP(mbox, -1);
	TESTP(mbox->bufs, -1);

	if ((buf_s64_t) box_num > mbox->bufs_num) {
		DE("Asked box is out of range\n");
		return -1;
	}

	if (NULL == mbox->bufs[box_num]) {
		DE("There is no buffer by asked index: %zu\n", box_num);
		return -1;
	}

	return mbox->bufs[box_num]->used;
}

ret_t mbox_box_free_mem(__attribute__((unused)) mbox_t *mbox, 
						__attribute__((unused)) size_t box_num)
{
	return -1;
	/* TODO */
}

void *mbox_box_steal_mem(__attribute__((unused)) mbox_t *mbox, 
						 __attribute__((unused)) size_t box_num)
{
	return NULL;
	/* TODO */
}


