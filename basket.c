#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "basket.h"
#include "buf_t.h"
#include "debug.h"
#include "tests.h"

static void basket_free_mem(void *mem, const char *who, const int line)
{
	DD("FREE / %s +%d / %p\n", who, line, mem);
	TESTP_ABORT(mem);
	free(mem);
}

basket_t *basket_new(void)
{
	basket_t *basket = malloc(sizeof(basket_t));
	if (NULL == basket) {
		DE("Can't allocate\n");
		return (NULL);
	}

	memset(basket, 0, sizeof(basket_t));
	return (basket);
}

ret_t basket_release(basket_t *basket)
{
	TESTP_ABORT(basket);

	/* TODO: Deallocate boxes */
	if (basket->boxes) {
		buf_u32_t i;

		for (i = 0; i < basket->boxes_used; i++) {
			if (buf_free(basket->boxes[i]) < 0) {
				DE("Could not release box[%u]\n", i);
				ABORT_OR_RETURN(-1);
			}
		}

		basket_free_mem(basket->boxes, __func__, __LINE__);
		DD("Freed basket->bufs\n");
	}

	/* Secure way: clear memory before release it */
	basket_free_mem(basket, __func__, __LINE__);
	return 0;
}

/*** Getters / Setters */

/* Internal function: get pointer of a box inside of the basket */
buf_t *basket_get_box(const basket_t *basket, buf_u32_t box_index)
{
	TESTP_ABORT(basket);
	TESTP_ABORT(basket->boxes);
	if (box_index > basket->boxes_used) {
		DE("Asked to get a box out of range\n");
		ABORT_OR_RETURN(NULL);
	}

	// DDD("Returning pointer to box %u (%p)\n", box_index, (bufs + box_index));
	return (basket->boxes[box_index]);
}

static buf_t *basket_get_last_box(const basket_t *basket)
{
	TESTP_ABORT(basket);
	TESTP_ABORT(basket->boxes);
	if (0 == basket->boxes_used) {
		return NULL;
	}

	DDD("The boxes count is %u, last buf index is %u, num of pointers is %u, returning ptr %p\n",
		basket->boxes_used, basket->boxes_used - 1, basket->boxes_allocated, /*bufs[number - 1]*/ basket->boxes[basket->boxes_used - 1]);
	return basket->boxes[basket->boxes_used - 1];
}

static buf_s64_t basket_get_last_box_index(const basket_t *basket)
{
	TESTP_ABORT(basket);
	return basket->boxes_used - 1;
}

size_t basket_size(const basket_t *basket)
{
	buf_u32_t i    = 0;
	size_t    size;

	TESTP_ABORT(basket);

	size = sizeof(basket_t);

	for (i = 0; i < basket->boxes_used; i++) {
		buf_t  *box = basket_get_box(basket, i);

		size += sizeof(buf_t);
		/* It can be NULL; normal */
		if (NULL == box) {
			continue;
		}

		/* Add size of the pointer */
		size += buf_room_take(box);
	}
	return size;
}

/* Clean all boxes */
ret_t basket_clean(basket_t *basket)
{
	uint32_t  i;
	TESTP_ABORT(basket);

	/* Validity test 1 */
	if (NULL == basket->boxes && basket->boxes_used > 0) {
		DE("Error: if number of boxes > 0 (%d), the boxes array must not be NULL pointer, but it is\n", basket->boxes_used);
		abort();
	}

	/* Validity test 2 */
	if (NULL != basket->boxes && basket->boxes_used == 0) {
		DE("Error: if number of boxes == 0, the boxes array must not be NULL pointer, but it is not : %p\n", basket->boxes);
		abort();
	}

	if (NULL == basket->boxes) {
		DDD("Not cleaning boxes because they are not exist: bufs ptr : %p, num of boxes: %u\n", basket->boxes, basket->boxes_used);
		return 0;
	}

	DDD("Cleaning basket->bufs_num (%u) boxes (%p)\n", basket->boxes_used, basket->boxes);
	for (i = 0; i < basket->boxes_used; i++) {

		/* It can be NULL; It is normal */
		if (basket->boxes[i]) {
			continue;
		}

		if (OK != buf_free(basket->boxes[i])) {
			DE("Could not free box\n");
			ABORT_OR_RETURN(-1);
		}
	}

	DDD("Free() basket->bufs_num boxes (%p)\n", basket->boxes);
	if (NULL != basket->boxes) {
		free(basket->boxes);
		basket->boxes = NULL;
	}

	basket->boxes_used = 0;
	basket_free_mem(basket, __func__, __LINE__);
	return 0;
}

static ret_t basket_grow_box_pointers(basket_t *basket)
{
	void *tmp;

	TESTP_ABORT(basket);

	DD("Going to call reallocarray(bufs = %p, basket->bufs_allocated + BASKET_BUFS_GROW_RATE = %u, sizeof(void *) = %zu)\n",
	   basket->boxes, basket->boxes_allocated + BASKET_BUFS_GROW_RATE, sizeof(void *));

	tmp = reallocarray(basket->boxes, basket->boxes_allocated + BASKET_BUFS_GROW_RATE, sizeof(void *));
	if (NULL == tmp) {
		DE("Allocation failed\n");
		ABORT_OR_RETURN(-1);
	}

	/* If we need to allocate more pointers in basket->bufs array,
	 * we allocate more than on.
	 * To be exact, we allocate BASKET_BUFS_GROW_RATE number of pointers.
	   This way we decrease delays when user adds new boxes */
	basket->boxes_allocated += BASKET_BUFS_GROW_RATE;

	if (tmp != basket->boxes) {
		/*
		 * TODO: Free old buffer?
		 * When it uncommented, we have a crash caused double free here
		 */
		// free(basket->bufs);
		basket->boxes = tmp;
	}
	return 0;
}

/* Add a new box */
ret_t box_add_new(basket_t *basket)
{
	TESTP_ABORT(basket);

	/* If no buffers, we should allocate - call 'grow' func */
	if (NULL == basket->boxes) {
		basket_grow_box_pointers(basket);
	}

	/* Here it MUST be not NULL */
	if (NULL == basket->boxes) {
		ABORT_OR_RETURN(-1);
	}

	if (basket->boxes_used == basket->boxes_allocated) {
		if (OK != basket_grow_box_pointers(basket)) {
			DE("Could not grow basket->bufs\n");
			ABORT_OR_RETURN(-1);
		}
	}

	/* The 'buf_new()' allocates a new box and cleans it, no need to clean */
	basket->boxes[basket->boxes_used] = buf_new(0);
	basket->boxes_used++;
	DDD("Allocated a new box, set at index %u\n", basket->boxes_used - 1);
	buf_dump(basket->boxes[basket->boxes_used - 1], "box_add_new(): added a new box, must be all 0/NULL");
	return 0;
}

/* TODO: Not tested yet */
ret_t box_insert_after(basket_t *basket, const uint32_t after_index)
{
	size_t    how_many_bytes_to_move;
	buf_u32_t move_start_offset_bytes;

	void      *move_start_p           = NULL;
	void      *move_end_p             = NULL;
	TESTP_ABORT(basket);
	TESTP_ABORT(basket->boxes);

	/* Test that we have enough allocated slots in basket->bufs to move all by 1 position */
	if ((basket->boxes_used + 1) > basket->boxes_allocated) {
		if (0 != basket_grow_box_pointers(basket)) {
			ABORT_OR_RETURN(-1);
		}
	}

	/* Move memory */
	how_many_bytes_to_move = (basket->boxes_used - after_index) * sizeof(void *);
	move_start_offset_bytes = (after_index + 1) * sizeof(void *);

	/* We start here ... */
	move_start_p = basket->boxes + move_start_offset_bytes;
	/*... and move to free one void * pointer, where we want to insert a new box */
	move_end_p = basket->boxes + move_start_offset_bytes + sizeof(void *);

	memmove(/* Dst */move_end_p, /* Src */ move_start_p, /* Size */ how_many_bytes_to_move);

	/* Insert a new box*/
	basket->boxes[after_index + 1] = buf_new(0);

	/* Increase boxes count */
	basket->boxes_used++;

	/* We are done */
	return 0;
}

ret_t box_swap(basket_t *basket, const buf_u32_t first_index, const buf_u32_t second_index)
{
	buf_t     *tmp;
	TESTP_ABORT(basket);

	if (first_index > basket->boxes_used || second_index > basket->boxes_used) {
		DE("One of asked boxes is out of range, first = %u, second = %u, number of boxes is %u\n",
		   first_index, second_index, basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	tmp = basket->boxes[first_index];
	basket->boxes[first_index] = basket->boxes[second_index];
	basket->boxes[second_index] = tmp;
	return 0;
}

ret_t box_remove(basket_t *basket, const buf_u32_t num)
{
	buf_t *box;
	TESTP_ABORT(basket);
	if (num > basket->boxes_used) {
		DE("Asked to remove a box (%u) out of range (%u)\n",
		   num, basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	box = basket_get_box(basket, num);

	TESTP(box, -1);

	if (OK != buf_clean_and_reset(box)) {
		DE("An error on box removal (buf_free failed)\n");
		ABORT_OR_RETURN(-1);
	}

	/* TOOD: If this box is the last box, run realloc and remove unused memory */
	return 0;
}

ret_t box_merge(basket_t *basket, const buf_u32_t src, const buf_u32_t dst)
{
	buf_t *box_src;
	buf_t *box_dst;
	ret_t rc;
	TESTP_ABORT(basket);

	if (src > basket->boxes_used || dst > basket->boxes_used) {
		DE("Src (%u) or dst (%u) box is out of range (%u)\n", src, dst, basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	box_src = basket_get_box(basket, src);

	if (NULL == box_src) {
		DD("Source box is NULL, no action needed\n");
		return 0;
	}

	if (0 == buf_used_take(box_src)) {
		DD("Source box is there, but it is empty, no action needded\n");
		return 0;
	}

	box_dst = basket_get_box(basket, dst);

	/*
	 * If we are here, it means:
	 * - There are bos 'src' and 'dst' boxes
	 * - There is some data in 'src' box
	 */
	rc = buf_add(box_dst, buf_data_take(box_src), buf_used_take(box_src));
	if (rc) {
		DE("Could not add data from box src (%u) to dst (%u)\n", src, dst);
		ABORT_OR_RETURN(rc);
	}

	return box_remove(basket, src);
}

ret_t box_bisect(__attribute__((unused)) basket_t *basket,
				 __attribute__((unused)) const buf_u32_t box_num,
				 __attribute__((unused)) const size_t from_offset)
{
	return -1;
	/* TODO */
}

ret_t basket_collapse_in_place(basket_t *basket)
{
	buf_u32_t i;
	TESTP_ABORT(basket);
	TESTP_ABORT(basket->boxes);

	// basket_dump(basket, "Going to collapse in place");

	/* We need two or more boxes to run this function */
	if (basket->boxes_used < 2) {
		return 0;
	}

	for (i = 1; i < basket->boxes_used; i++) {
		if (NULL == basket->boxes[i]) {
			DD("Do not merge box[%u] of %u, because it is NULL pointer\n", i, basket->boxes_used);
			continue;
		}

		DD("BOX[%u]\n", i);
		//buf_dump(basket->boxes[i], "Merging buf[X] into buf[0]");
		if (OK != buf_merge(basket->boxes[0], basket->boxes[i])) {
			ABORT_OR_RETURN(-1);
		}
	}

	return 0;
}

/* This function creates a flat buffer ready for sending/saving */
void *basket_to_buf(const basket_t *basket, size_t *size)
{
	buf_u32_t            i;
	char                 *buf;
	size_t               buf_size          = 0;
	size_t               buf_offset        = 0;

	basket_send_header_t basket_buf_header;
	box_dump_t           box_dump_header;

	TESTP_ABORT(basket);
	TESTP_ABORT(size);

	/* Let's calculate required buffer size */

	/* The box dump: this is a header of the buffer */
	buf_size = sizeof(basket_send_header_t);

	/* And we need the struct box_dump_t per box */
	buf_size += sizeof(box_dump_t) * basket->boxes_used;

	/* We need to dump the content of every box, so count common of buffer in all boxes */
	for (i = 0; i < basket->boxes_used; i++) {
		buf_t *box = basket_get_box(basket, i);
		buf_size += buf_used_take(box);
	}

	/* Alocate the memory buffer */
	buf = malloc(buf_size);
	TESTP(buf, NULL);

	memset(buf, 0, buf_size);

	/* Let's start fill the buffer */

	/* First, fill the header */
	basket_buf_header.total_len = buf_size;
	basket_buf_header.boxes_num = basket->boxes_used;

	/* Watermark: a predefined pattern. */
	basket_buf_header.watermark = WATERMARK_BASKET;

	/* TODO: The checksum not implemented yet */
	basket_buf_header.basket_checksum = 0;

	memcpy(buf, &basket_buf_header, sizeof(basket_send_header_t));
	buf_offset += sizeof(basket_send_header_t);

	/* Now we run on array of boxes, and add one by one to the buffer */

	for (i = 0; i < basket->boxes_used; i++) {
		buf_t  *box     = basket_get_box(basket, i);

		/* Fill the header, for every box we create and fill the header */
		box_dump_header.box_size = buf_used_take(box);
		box_dump_header.watermark = WATERMARK_BOX;

		/* TODO */
		box_dump_header.box_checksum = 0;

		memcpy(buf + buf_offset, &box_dump_header, sizeof(box_dump_t));
		buf_offset += sizeof(box_dump_t);

		/* If this box is NULL, or is empty, we don't add any data and skip */
		if (0 == box_dump_header.box_size) continue;

		memcpy(buf + buf_offset, buf_data_take(box), box_dump_header.box_size);
		buf_offset += box_dump_header.box_size;
	}

	DDD("Returning buffer, size: %zu, counted offset is: %zu\n", buf_size, buf_offset);
	*size = buf_size;
	return buf;
}

basket_t *basket_from_buf(void *buf, const size_t size)
{
	uint32_t             i;
	uint32_t             buf_offset         = 0;
	basket_t             *basket;
	basket_send_header_t *basket_buf_header;
	box_dump_t           *box_dump_header;
	char                 *buf_char          = buf;

	TESTP_ABORT(buf);
	if (size < sizeof(basket_send_header_t)) {
		DE("Wrong size: less than size of structure basket_send_header_t\n");
		TRY_ABORT();
		return NULL;
	}

	/* This must be the header of the basket flattern buffer */
	basket_buf_header = (basket_send_header_t *)buf;
	/* Let's check that there is a predefined 'watermask'; if not, it is not a basked */
	if (WATERMARK_BASKET != basket_buf_header->watermark) {
		DE("Wrong buffer: wrong watermark. Expected %X but it is %X\n", WATERMARK_BASKET, basket_buf_header->watermark);
		ABORT_OR_RETURN(NULL);
	}

	basket = basket_new();
	TESTP(basket, NULL);

	buf_offset = sizeof(basket_send_header_t);

	for (i = 0; i < basket_buf_header->boxes_num; i++) {
		box_dump_header = (box_dump_t *)buf_char + buf_offset;

		if (WATERMARK_BOX != box_dump_header->watermark) {
			DE("Wrong box: wrong watermark. Expected %X but it is %X\n", WATERMARK_BOX, box_dump_header->watermark);
			if (0 != basket_release(basket)) {
				DE("Could not release basket\n");
			}
			ABORT_OR_RETURN(NULL);
		}

		buf_offset += sizeof(box_dump_t);

		/* If there us no data, we do not even add a box */
		if (0 == box_dump_header->box_size) {
			continue;
		}

		if (0 != box_add_to_tail(basket, i, (buf_char + buf_offset), box_dump_header->box_size)) {
			DE("Adding a buffer size (%u) to tail of box (%u) failed\n", box_dump_header->box_size, i);
			if (basket_release(basket)) {
				DE("Could not release basket\n");
			}
			ABORT_OR_RETURN(NULL);
		}
		buf_offset += box_dump_header->box_size;
	}

	return basket;
}

/* This function creates a flat buffer ready for sending */
buf_t *basket_to_buf_t(basket_t *basket)
{
	TESTP_ABORT(basket);
	return NULL;
	/* TODO */
}

/* This function restires Mbox object with boxes from the flat buffer */
basket_t *basket_from_buf_t(const buf_t *buf)
{
	TESTP_ABORT(buf);
	return NULL;
	/* TODO */
}

/* Create a new basket and add data */
/* Testing function: basket_test.c::box_new_from_data_test() */
ssize_t box_new_from_data(basket_t *basket, const void *buffer, const buf_u32_t buffer_size)
{
	buf_t     *box;
	TESTP_ABORT(basket);
	TESTP_ABORT(buffer);

	/* Add a new box */
	if (0 != box_add_new(basket)) {
		DE("Can not add another box into Basket\n");
		ABORT_OR_RETURN(-1);
	}

	/* We allocated an additional box. Thus, number of boxes MUST be > 0 */
	if (0 == basket->boxes_used) {
		DE("The number of boxes must not be 0 here, but it is\n");
		ABORT_OR_RETURN(-1);
	}

	box = basket_get_last_box(basket);

	buf_dump(box, "box_new_from_data(): got the last box from basket");

	if (NULL == box) {
		DE("Could not get pointer to the last box, number of boxes (%u) - stop\n",
		   basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	DD("Going to add data in the tail of a new buf, new data size is %u\n", buffer_size);
	if (OK != buf_add(box, buffer, buffer_size)) {
		DE("Could not add data into the last (new) box: new data size %u\n", buffer_size);
		ABORT_OR_RETURN(-1);
	}

	buf_dump(box, "box_new_from_data(): Added new data");

	/* Return the number of the new box */
	return basket_get_last_box_index(basket);
}

/* Add data to a basket */
ret_t box_add_to_tail(basket_t *basket, const buf_u32_t box_num, const void *buffer, const size_t buffer_size)
{
	buf_t     *box;
	TESTP_ABORT(basket);
	TESTP_ABORT(buffer);

	if (box_num > basket->boxes_used) {
		DE("Asked box is out of range: asked box %u, number of boxes is %u\n", box_num, basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	box = basket_get_box(basket, box_num);
	if (NULL == box) {
		DE("Could not get box pointer - stopping\n");
		ABORT_OR_RETURN(-1);
	}

	return buf_add(box, buffer, buffer_size);
}

ret_t box_replace_data(basket_t *basket, const buf_u32_t box_num, const void *buffer, const size_t buffer_size)
{
	buf_t *box;
	TESTP_ABORT(basket);
	TESTP_ABORT(buffer);
	if (buffer_size < 1) {
		DE("The new buffer size must be > 0\n");
		return -EINVAL;
	}

	box = basket_get_box(basket, box_num);
	if (NULL == box) {
		DE("Could not take pointer for the box (%u) - stopping\n", box_num);
		ABORT_OR_RETURN(-1);
	}

	return buf_replace(box, buffer, buffer_size);
}

void *box_data_ptr(const basket_t *basket, const buf_u32_t box_num)
{
	buf_t     *box;
	TESTP_ABORT(basket);
	TESTP_ABORT(basket->boxes);

	if (box_num > basket->boxes_used) {
		DE("Asked box (%u) is out of range (%d)\n", box_num, basket->boxes_used);
		ABORT_OR_RETURN(NULL);
	}

	box = basket_get_box(basket, box_num);

	/* This should not happen never */
	if (NULL == box) {
		DE("There is no buffer by asked index: %u\n", box_num);
		abort();
	}

	return buf_data_take(box);
}

ssize_t box_data_size(const basket_t *basket, const buf_u32_t box_num)
{
	buf_t     *box;
	TESTP_ABORT(basket);

	if (box_num > basket->boxes_used) {
		DE("Asked box (%u) is out of range (%d)\n", box_num, basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}
	box = basket_get_box(basket, box_num);

	/* Never should happen */
	if (NULL == box) {
		DE("There is no buffer by asked index: %u\n", box_num);
		ABORT_OR_RETURN(-1);
	}

	return buf_used_take(box);
}

ret_t box_data_free(basket_t *basket, const buf_u32_t box_num)
{
	buf_t *box;
	TESTP_ABORT(basket);

	box = basket_get_box(basket, box_num);

	/* The box can be NULL, it is a valid situation */
	if (NULL == box) {
		return 0;
	}

	return buf_clean_and_reset(box);
}

void *box_steal_data(basket_t *basket, const buf_u32_t box_num)
{
	TESTP_ABORT(basket);

	buf_t  *box = basket_get_box(basket, box_num);

	/* The box can be NULL, it is a valid situation */
	if (NULL == box) {
		return NULL;
	}

	/* The box can be empty, in this case no buffer is in buf_t */
	if (0 == buf_used_take(box)) {
		return NULL;
	}

	return buf_data_steal(box);
}

void basket_dump(basket_t *basket, const char *msg)
{
	TESTP_ABORT(basket);
	TESTP_ABORT(msg);

	DD("~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	DD("%s\n", msg);
	DD("Basket ptr:            %p\n", basket);
	DD("Basket number of bufs: %u\n", basket->boxes_used);
	DD("Basket bufs[] ptr    : %p\n", basket->boxes);

	if (basket->boxes_used > 0) {
		buf_u32_t i;

		for (i = 0; i < basket->boxes_used; i++) {
			buf_t *box = basket_get_box(basket, i);

			DD("`````````````````````````\n");

			DD(">>> Box[%u] ptr:        %p\n", i, box);
			if (NULL == box) continue;

			DD(">>> Box[%u] used:       %ld\n", i, buf_used_take(box));
			DD(">>> Box[%u] room:       %ld\n", i, buf_room_take(box));
			DD(">>> Box[%u] data ptr:   %p\n", i, buf_data_take(box));

		}
	}
	DD("^^^^^^^^^^^^^^^^^^^^^^^^^\n");
}

