#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mbox.h"
#include "buf_t.h"
#include "debug.h"

static basket_t *basket_t_alloc(void)
{
	basket_t *basket = malloc(sizeof(basket_t));
	if (NULL == basket) {
		DE("Can't allocate\n");
		return (NULL);
	}

	memset(basket, 0, sizeof(basket_t));
	return (basket);
}

static void basket_t_release(basket_t *basket)
{
	if (NULL == basket) {
		DE("Arg is NULL\n");
		return;
	}

	/* Secure way: clear memory before release it */
	memset(basket, 0, sizeof(basket_t));
	free(basket);
}

/* Allocate new Basket object */
basket_t *basket_new(const uint32_t number_of_boxes)
{
	uint32_t i;
	basket_t *basket = basket_t_alloc();
	TESTP(basket, NULL);

	if (0 == number_of_boxes) {
		return basket;
	}

	basket->bufs = malloc(sizeof(buf_t *) * number_of_boxes);
	if (NULL == basket->bufs) {
		DE("Can not allocate pointers array for %u boxes\n", number_of_boxes);
		basket_t_release(basket);
		return NULL;
	}

	for (i = 0; i < number_of_boxes; i++) {
		basket->bufs[i] = buf_new(0);
		if (NULL == basket->bufs[i]) {
			DE("Can not allocate boxe number %u of %u\n", i, number_of_boxes);
			basket_t_release(basket);
			return NULL;
		}
	}
	basket->bufs_num = number_of_boxes;
	return basket;
}

size_t basket_size(const basket_t *basket)
{
	int    i    = 0;
	size_t size = sizeof(basket_t);

	for (i = 0; i < basket->bufs_num; i++) {
		/* Add size of the pointer */
		size += sizeof(buf_t *);
		if (NULL != basket->bufs[i]) {

			/* Add size of buf_t struct */
			size += sizeof(buf_t);

			/* Add size of the buffer in the buf_t struct */
			size += basket->bufs[i]->room;
		}
	}
	return size;
}

/* Clean all boxes */
ret_t basket_clean(basket_t *basket)
{
	TESTP(basket, -1);
	if (basket->bufs && (basket->bufs_num > 0)) {
		uint32_t i;
		DDD("Cleaning basket->bufs_num boxes\n");
		for (i = 0; i < basket->bufs_num; i++) {
			buf_free(basket->bufs[i]);
		}
		free(basket->bufs);
	} else {
		DDD("Not cleaning boxes\n");
	}

	basket->bufs_num = 0;
	return 0;
}

/* Free Mbox */
ret_t basket_release(basket_t *basket)
{
	TESTP(basket, -1);
	if (0 != basket_clean(basket)) {
		return -1;
	}

	basket_t_release(basket);
	return 0;
}

/* Add more boxes */
ret_t box_allocate(basket_t *basket, const size_t num)
{
	size_t i;
	TESTP(basket, -1);
	size_t new_size = basket->bufs_num + num;
	new_size *= sizeof(buf_t *);

	buf_t **new_bufs = realloc(basket->bufs, new_size);

	/* Can not reallocate */
	if (NULL == new_bufs) {
		DE("Realloc failed\n");
		return (-ENOMEM);
	}

	/* Case 2: realloc succeeded, new memory returned */
	/* No need to free the old memory - done by realloc */
	if (basket->bufs != new_bufs) {
		free(basket->bufs);
		basket->bufs = new_bufs;
	}

	/* Now allocate the boxes */
	for (i = basket->bufs_num; i < basket->bufs_num + num; i++) {
		basket->bufs[i] = buf_new(0);
		if (NULL == basket->bufs[i]) {
			DE("Could not add new buf_t at index %lu\n", i);
		}
	}

	/* Increase count of the boxes in the Mbox */
	basket->bufs_num += num;

	/* Case 3: Reallocated, and the same pointer, no need to do anything */
	return 0;
}

ssize_t basket_boxes_count(const basket_t *basket)
{
	return basket->bufs_num;
}

ret_t box_insert_after(basket_t *basket, const uint32_t after, const uint32_t num)
{
	ret_t    rc;
	size_t   i;
	size_t   how_many_to_move;
	uint32_t move_start;
	uint32_t move_end;

	buf_t    *move_start_p;
	buf_t    *move_end_p;

	/* 1. Reallocate the number of boxes, add one more */


	how_many_to_move = basket->bufs_num - (after + 1) * sizeof(buf_t *);
	move_start = after + 1;
	move_end = after + 1 + num;

	rc = box_allocate(basket, num);
	if (0 != rc) {
		return -1;
	}
	/* 2. Shift boxes after the 'after' */

	//move_start_p = basket->bufs + (sizeof(buf_t *) * move_start);
	move_start_p = basket->bufs[move_start];
	//move_end_p = basket->bufs + (sizeof(buf_t *) * move_end);
	move_end_p = basket->bufs[move_end];

	memmove(move_start_p, move_end_p, how_many_to_move);

	/* 3. Now add a new box */
	for (i = 0; i < num; i++) {
		basket->bufs[after + i] = buf_new(0);
		if (NULL == basket->bufs[after + i]) {
			return -1;
		}
	}

	return 0;
}

ret_t box_swap(basket_t *basket, const size_t first, const size_t second)
{
	buf_t *tmp;
	TESTP(basket, -1);
	if (first > (size_t)basket->bufs_num || second > (size_t)basket->bufs_num) {
		return -1;
	}

	tmp = basket->bufs[first];
	basket->bufs[first] = basket->bufs[second];
	basket->bufs[second] = tmp;
	return 0;
}

ret_t box_remove(basket_t *basket, const size_t num)
{
	TESTP(basket, -1);
	if (num > (size_t)basket->bufs_num) {
		return -1;
	}

	if (NULL == basket->bufs[num]) {
		return -1;
	}

	return buf_clean_and_reset(basket->bufs[num]);
}

ret_t box_merge(basket_t *basket, const size_t src, const size_t dst)
{
	ret_t rc;
	TESTP(basket, -1);
	if (src > (size_t)basket->bufs_num || dst > (size_t)basket->bufs_num) {
		return -1;
	}

	rc = buf_add(basket->bufs[dst], basket->bufs[src]->data, basket->bufs[src]->used);
	if (rc) {
		return rc;
	}

	rc = buf_clean_and_reset(basket->bufs[src]);
	if (rc) {
		return rc;
	}

	return 0;
}

ret_t box_bisect(__attribute__((unused)) basket_t *basket,
				 __attribute__((unused))  const  size_t box_num,
				 __attribute__((unused)) const  size_t from_offset)
{
	return -1;
	/* TODO */
}

ret_t basket_collapse_in_place(__attribute__((unused)) basket_t *basket)
{
	return -1;
	/* TODO */
}

/* This function creates a flat buffer ready for sending */
void *basket_to_buf(const basket_t *basket, size_t *size)
{
	int                  i;
	char                 *buf;
	size_t               buf_size          = 0;
	size_t               buf_offset        = 0;

	basket_send_header_t basket_buf_header;
	box_dump_t           box_dump_header;

	TESTP(basket, NULL);
	TESTP(size, NULL);

	/* Let's calculate required buffer size */

	/* The box dump: this is a heade of the buffer */
	buf_size = sizeof(box_dump_t);

	/* We need basket_send_header_t struct per box */
	buf_size += sizeof(basket_send_header_t) * basket->bufs_num;

	/* We need to dump the content of every box */
	for (i = 0; i < basket->bufs_num; i++) {
		buf_size += basket->bufs[i]->used;
	}

	/* Alocate the buffer */
	buf = malloc(buf_size);
	TESTP(buf, NULL);

	memset(buf, 0, buf_size);

	/* Let's start fill the buffer */

	/* First, fill the header */
	basket_buf_header.total_len = buf_size;
	basket_buf_header.boxes_num = basket->bufs_num;

	/* Watermark: a predefined pattern. */
	basket_buf_header.watermark = WATERMARK_BASKET;

	/* TODO */
	basket_buf_header.basket_checksum = 0;

	memcpy(buf, &basket_buf_header, sizeof(basket_send_header_t));
	buf_offset += sizeof(basket_send_header_t);

	/* Now we run on array of boxes, and add one by one to the buffer */

	for (i = 0; i < basket->bufs_num; i++) {

		/* Fill the header, for every box we create and fill the header */
		box_dump_header.box_size = basket->bufs[i]->used;
		box_dump_header.watermark = WATERMARK_BOX;

		/* TODO */
		box_dump_header.box_checksum = 0;
		memcpy(buf + buf_offset, &box_dump_header, sizeof(box_dump_t));
		buf_offset += sizeof(box_dump_t);

		/* If this box is empty, we don't add any data and skip */
		if (basket->bufs[i]->used == 0) continue;

		memcpy(buf + buf_offset, basket->bufs[i]->data, basket->bufs[i]->used);
		buf_offset += basket->bufs[i]->used;
	}

	DDD("Returning buffer, size: %zu, counted offset is: %zu\n", buf_size, buf_offset);
	*size = buf_size;
	return buf;
	/* TODO */
}

basket_t *basket_from_buf(void *buf, const size_t size)
{
	uint32_t             i;
	uint32_t             buf_offset         = 0;
	basket_t             *basket;
	basket_send_header_t *basket_buf_header;
	box_dump_t           *box_dump_header;

	TESTP(buf, NULL);
	if (size < sizeof(basket_send_header_t)) {
		DE("Wrong size: less than size of structure basket_send_header_t\n");
		return NULL;
	}

	basket_buf_header = (basket_send_header_t *)buf;
	if (WATERMARK_BASKET != basket_buf_header->watermark) {
		DE("Wrong buffer: wrong watermark. Expected %X but it is %X\n", WATERMARK_BASKET, basket_buf_header->watermark);
		return NULL;
	}

	basket = basket_new(basket_buf_header->boxes_num);
	TESTP(basket, NULL);

	buf_offset = sizeof(basket_send_header_t);

	for (i = 0; i < basket_buf_header->boxes_num; i++) {
		box_dump_header = buf + buf_offset;

		if (WATERMARK_BOX != box_dump_header->watermark) {
			DE("Wrong box: wrong watermark. Expected %X but it is %X\n", WATERMARK_BOX, box_dump_header->watermark);
			basket_release(basket);
			return NULL;
		}

		buf_offset += sizeof(box_dump_t);

		if (0 == box_dump_header->box_size) {
			continue;
		}

		box_add_to_tail(basket, i, ((char *) buf + buf_offset), box_dump_header->box_size);
		buf_offset += box_dump_header->box_size;
	}

	return basket;
}

/* This function creates a flat buffer ready for sending */
buf_t *basket_to_buf_t(__attribute__((unused)) basket_t *basket)
{
	//basket_send_header_t *basket_buf_header;
	//box_dump_t           *box_dump_header;

	return NULL;
	/* TODO */
}

/* This function restires Mbox object with boxes from the flat buffer */
basket_t *basket_from_buf_t(__attribute__((unused)) const buf_t *buf)
{
	return NULL;
	/* TODO */
}

/* Create a new basket and add data */
ssize_t box_new_from_data(basket_t *basket, const void *buffer, const size_t buffer_size)
{
	TESTP(basket, -1);
	TESTP(buffer, -1);

	/* Add a new box */
	if (0 != box_allocate(basket, 1)) {
		DE("Can not add another box into Mbox\n");
		return -1;
	}

	DDD("Now number of boxes: %lu\n", basket->bufs_num);

	if (0 != box_add_to_tail(basket, basket->bufs_num - 1, buffer, buffer_size)) {
		DE("Could not add data into a new box %lu\n", basket->bufs_num - 1);
	}

	/* Return the number of the new box */
	return (basket->bufs_num - 1);
}

/* Return 1 if the box valid, 0 otherwise, -1 on error;
   It does not tests that the box created, only that the box index is in range */
static int basket_if_box_index_valid(basket_t *basket, buf_s64_t box_num)
{
	TESTP(basket, -1);
	if (0 == basket->bufs_num) {
		return 0;
	}

	if (box_num > (basket->bufs_num - 1)) {
		return 0;
	}

	return 1;
}

/* Tests that there the index of the box valid AND is a box by the given index. */
static int basket_if_box_valid(basket_t *basket, buf_s64_t box_num)
{
	if (1 != basket_if_box_index_valid(basket, box_num)) {
		return 0;
	}

	if (NULL == basket->bufs[box_num]) {
		return 0;
	}

	return 1;
}

/* Add data to a basket */
ret_t box_add_to_tail(basket_t *basket, const buf_s64_t box_num, const void *buffer, const size_t buffer_size)
{
	TESTP(basket, -1);
	TESTP(buffer, -1);

	if (1 != basket_if_box_valid(basket, box_num)) {
		DE("Box index is out of range\n");
		return -1;
	}

	return buf_add(basket->bufs[box_num], buffer, buffer_size);
}

ret_t box_replace_data(basket_t *basket, const size_t box_num, const void *buffer, const size_t buffer_size)
{
	TESTP(basket, -EINVAL);
	TESTP(basket->bufs, -EINVAL);
	TESTP(buffer, -EINVAL);
	if (buffer_size < 1) {
		DE("The new buffer size must be > 0\n");
		return -EINVAL;
	}

	return buf_replace(basket->bufs[box_num], buffer, buffer_size);
}

void *box_data_ptr(const basket_t *basket, const size_t box_num)
{
	TESTP(basket, NULL);
	TESTP(basket->bufs, NULL);

	if ((buf_s64_t)box_num > basket->bufs_num) {
		DE("Asked box is out of range\n");
		return NULL;
	}

	if (NULL == basket->bufs[box_num]) {
		DE("There is no buffer by asked index: %zu\n", box_num);
		return NULL;
	}

	return basket->bufs[box_num]->data;
}

ssize_t box_data_size(const basket_t *basket, const size_t box_num)
{
	TESTP(basket, -1);
	TESTP(basket->bufs, -1);

	if ((buf_s64_t)box_num > basket->bufs_num) {
		DE("Asked box is out of range\n");
		return -1;
	}

	if (NULL == basket->bufs[box_num]) {
		DE("There is no buffer by asked index: %zu\n", box_num);
		return -1;
	}

	return basket->bufs[box_num]->used;
}

ret_t box_data_free(basket_t *basket, const size_t box_num)
{
	TESTP(basket, -1);
	TESTP(basket->bufs, -1);

	return buf_clean_and_reset(basket->bufs[box_num]);
}

void *box_steal_data(basket_t *basket, const size_t box_num)
{
	return buf_data_steal(basket->bufs[box_num]);
}


