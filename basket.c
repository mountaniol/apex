#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "basket.h"
#include "box_t.h"
#include "debug.h"
#include "tests.h"

static ret_t box_new_from_data_by_index(basket_t *basket, box_u32_t box_index, const void *buffer, const box_u32_t buffer_size);

static void basket_free_mem(void *mem, const char *who, const int line)
{
	DDD("FREE / %s +%d / %p\n", who, line, mem);
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
		box_u32_t box_index;

		for (box_index = 0; box_index < basket->boxes_used; box_index++) {
			if (box_free(basket->boxes[box_index]) < 0) {
				DE("Could not release box[%u]\n", box_index);
				ABORT_OR_RETURN(-1);
			}
		}

		basket_free_mem(basket->boxes, __func__, __LINE__);
		DDD("Freed basket->boxes\n");
	}

	/* Secure way: clear memory before release it */
	basket_free_mem(basket, __func__, __LINE__);
	return 0;
}

/*** Getters / Setters */

/* Internal function: get pointer of a box inside of the basket */
box_t *basket_get_box(const basket_t *basket, box_u32_t box_index)
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

static box_t *basket_get_last_box(const basket_t *basket)
{
	TESTP_ABORT(basket);
	TESTP_ABORT(basket->boxes);
	if (0 == basket->boxes_used) {
		return NULL;
	}

	DDD("The boxes count is %u, last box index is %u, num of pointers is %u, returning ptr %p\n",
		basket->boxes_used, basket->boxes_used - 1, basket->boxes_allocated, /*boxes[number - 1]*/ basket->boxes[basket->boxes_used - 1]);
	return basket->boxes[basket->boxes_used - 1];
}

static box_s64_t basket_get_last_box_index(const basket_t *basket)
{
	TESTP_ABORT(basket);
	return basket->boxes_used - 1;
}

size_t basket_size(const basket_t *basket)
{
	box_u32_t box_index    = 0;
	size_t    size = 0;

	TESTP_ABORT(basket);

	size = sizeof(basket_t);
	size += basket->boxes_allocated * sizeof(void *);

	for (box_index = 0; box_index < basket->boxes_used; box_index++) {
		box_t  *box = basket_get_box(basket, box_index);

		size += sizeof(box_t);
		/* It can be NULL; normal */
		if (NULL == box) {
			continue;
		}

		/* Add size of the pointer */
		size += box_room_take(box);
	}
	return size;
}

/* Clean all boxes */
ret_t basket_clean(basket_t *basket)
{
	uint32_t  box_index;
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
		DDD("Not cleaning boxes because they are not exist: ->boxes ptr : %p, num of boxes: %u\n", basket->boxes, basket->boxes_used);
		return 0;
	}

	DDD("Cleaning basket->boxes_used (%u) boxes (%p)\n", basket->boxes_used, basket->boxes);
	for (box_index = 0; box_index < basket->boxes_used; box_index++) {

		/* It can be NULL; It is normal */
		if (basket->boxes[box_index]) {
			continue;
		}

		if (OK != box_free(basket->boxes[box_index])) {
			DE("Could not free box\n");
			ABORT_OR_RETURN(-1);
		}
	}

	DDD("Free() basket->boxes_used boxes (%p)\n", basket->boxes);
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
	void   *reallocated_mem;

	char   *clean_start_bytes_p;
	size_t clean_szie_bytes;

	TESTP_ABORT(basket);

	DDD("Going to call reallocarray(boxes = %p, basket->boxes_allocated + BASKET_BUFS_GROW_RATE = %u, sizeof(void *) = %zu)\n",
	   basket->boxes, basket->boxes_allocated + BASKET_BUFS_GROW_RATE, sizeof(void *));

	reallocated_mem = reallocarray(basket->boxes, basket->boxes_allocated + BASKET_BUFS_GROW_RATE, sizeof(void *));
	if (NULL == reallocated_mem) {
		DE("Allocation failed\n");
		ABORT_OR_RETURN(-1);
	}

	/* Clean the freshly allocated memory */
	clean_start_bytes_p = (char *)reallocated_mem + (basket->boxes_allocated * sizeof(void *));
	clean_szie_bytes = BASKET_BUFS_GROW_RATE * sizeof(void *);
	//DD("Going to clean: tmp (%p) + basket->boxes_allocated (%u) == %p, 0, BASKET_BUFS_GROW_RATE (%d) * sizeof(void *) %(zu)\n",
	//   (char *) tmp, basket->boxes_allocated, (tmp + basket->boxes_allocated), BASKET_BUFS_GROW_RATE, sizeof(void *) );
	memset(clean_start_bytes_p, 0, clean_szie_bytes);


	/* If we need to allocate more pointers in basket->boxes array,
	 * we allocate more than on.
	 * To be exact, we allocate BASKET_BUFS_GROW_RATE number of pointers.
	   This way we decrease delays when user adds new boxes */
	basket->boxes_allocated += BASKET_BUFS_GROW_RATE;

	if (reallocated_mem != basket->boxes) {
		/*
		 * TODO: Free old box?
		 * When it uncommented, we have a crash caused double free here
		 */
		// free(basket->boxes);
		basket->boxes = reallocated_mem;
	}

	return 0;
}

/* Add a new box */
ret_t box_add_new(basket_t *basket)
{
	TESTP_ABORT(basket);

	/* If no boxes, we should allocate - call 'grow' func */
	if (NULL == basket->boxes) {
		basket_grow_box_pointers(basket);
	}

	/* Here it MUST be not NULL */
	if (NULL == basket->boxes) {
		ABORT_OR_RETURN(-1);
	}

	if (basket->boxes_used == basket->boxes_allocated) {
		if (OK != basket_grow_box_pointers(basket)) {
			DE("Could not grow basket->boxes\n");
			ABORT_OR_RETURN(-1);
		}
	}

	/* The 'buf_new()' allocates a new box and cleans it, no need to clean */
	basket->boxes[basket->boxes_used] = box_new(0);
	basket->boxes_used++;
	DDD("Allocated a new box, set at index %u\n", basket->boxes_used - 1);
	box_dump(basket->boxes[basket->boxes_used - 1], "box_add_new(): added a new box, must be all 0/NULL");
	return 0;
}

/* TODO: Not tested yet */
ret_t box_insert_after(basket_t *basket, const uint32_t after_index)
{
	size_t    how_many_bytes_to_move;
	box_u32_t move_start_offset_bytes;

	void      *move_start_p           = NULL;
	void      *move_end_p             = NULL;
	TESTP_ABORT(basket);
	TESTP_ABORT(basket->boxes);

	/* Test that we have enough allocated slots in basket->boxes to move all by 1 position */
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
	basket->boxes[after_index + 1] = box_new(0);

	/* Increase boxes count */
	basket->boxes_used++;

	/* We are done */
	return 0;
}

ret_t box_swap(basket_t *basket, const box_u32_t first_index, const box_u32_t second_index)
{
	box_t     *tmp;
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

ret_t box_remove(basket_t *basket, const box_u32_t box_index)
{
	box_t *box;
	TESTP_ABORT(basket);
	if (box_index > basket->boxes_used) {
		DE("Asked to remove a box (%u) out of range (%u)\n",
		   box_index, basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	box = basket_get_box(basket, box_index);

	TESTP(box, -1);

	if (OK != box_clean_and_reset(box)) {
		DE("An error on box removal (buf_clean_and_reset failed)\n");
		ABORT_OR_RETURN(-1);
	}

	/* TOOD: If this box is the last box, run realloc and remove unused memory */
	return 0;
}

ret_t box_merge_box(basket_t *basket, const box_u32_t src, const box_u32_t dst)
{
	box_t *box_src;
	box_t *box_dst;
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

	if (0 == box_used_take(box_src)) {
		DD("Source box is there, but it is empty, no action needded\n");
		return 0;
	}

	box_dst = basket_get_box(basket, dst);

	/*
	 * If we are here, it means:
	 * - There are bos 'src' and 'dst' boxes
	 * - There is some data in 'src' box
	 */
	rc = box_add(box_dst, box_data_take(box_src), box_used_take(box_src));
	if (rc) {
		DE("Could not add data from box src (%u) to dst (%u)\n", src, dst);
		ABORT_OR_RETURN(rc);
	}

	return box_remove(basket, src);
}

ret_t box_bisect(__attribute__((unused)) basket_t *basket,
				 __attribute__((unused)) const box_u32_t box_num,
				 __attribute__((unused)) const size_t from_offset)
{
	return -1;
	/* TODO */
}

ret_t basket_collapse_in_place(basket_t *basket)
{
	box_u32_t box_index;
	TESTP_ABORT(basket);
	TESTP_ABORT(basket->boxes);

	// basket_dump(basket, "Going to collapse in place");

	/* We need two or more boxes to run this function */
	if (basket->boxes_used < 2) {
		return 0;
	}

	for (box_index = 1; box_index < basket->boxes_used; box_index++) {
		if (NULL == basket->boxes[box_index]) {
			DD("Do not merge box[%u] of %u, because it is NULL pointer\n", box_index, basket->boxes_used);
			continue;
		}

		if (OK != box_merge(basket->boxes[0], basket->boxes[box_index])) {
			ABORT_OR_RETURN(-1);
		}
	}

	return 0;
}

/* This function creates a flat buffer ready for sending/saving */
void *basket_to_buf(const basket_t *basket, size_t *size)
{
	box_u32_t            box_index;
	char                 *buf;
	size_t               buf_size             = 0;
	size_t               buf_offset           = 0;

	basket_send_header_t *basket_buf_header_p;
	box_dump_t           *box_dump_header_p;

	TESTP_ABORT(basket);
	TESTP_ABORT(size);

	/* Let's calculate required buffer size */

	/* The box dump: this is a header of the buffer */
	buf_size = sizeof(basket_send_header_t);

	/* And we need the struct box_dump_t per box */
	buf_size += sizeof(box_dump_t) * basket->boxes_used;

	/* We need to dump the content of every box, so count total size of buffers in all boxes */
	for (box_index = 0; box_index < basket->boxes_used; box_index++) {
		box_t *box = basket_get_box(basket, box_index);
		buf_size += box_used_take(box);
	}

	/* Alocate the memory buffer */
	buf = malloc(buf_size);
	TESTP(buf, NULL);

	memset(buf, 0, buf_size);

	basket_buf_header_p = (basket_send_header_t *)buf;

	/* Let's start filling the buffer */

	/* First, fill the header */
	basket_buf_header_p->total_len = buf_size;
	basket_buf_header_p->boxes_num = basket->boxes_used;

	basket_buf_header_p->total_len = buf_size;
	basket_buf_header_p->boxes_num = basket->boxes_used;

	/* Watermark: a predefined pattern. */
	basket_buf_header_p->watermark = WATERMARK_BASKET;

	/* TODO: The checksum not implemented yet */
	basket_buf_header_p->basket_checksum = 0;

	//memcpy(buf, &basket_buf_header, sizeof(basket_send_header_t));
	buf_offset += sizeof(basket_send_header_t);

	/* Now we run on array of boxes, and add one by one to the buffer */

	for (box_index = 0; box_index < basket->boxes_used; box_index++) {
		box_t     *box      = basket_get_box(basket, box_index);
		char      *box_data;
		box_s64_t box_used  = 0;

		box_dump_header_p = (box_dump_t *)(buf + buf_offset);

		/* Fill the header, for every box we create and fill the header */
		if (box) {
			box_used = box_used_take(box);
		}

		box_dump_header_p->box_size = box_used;
		box_dump_header_p->watermark = WATERMARK_BOX;

		/* TODO */
		box_dump_header_p->box_checksum = 0;

		// memcpy(buf + buf_offset, &box_dump_header, sizeof(box_dump_t));
		buf_offset += sizeof(box_dump_t);

		/* If this box is NULL, or is empty, we don't add any data and skip */
		if (0 == box_used) continue;

		box_data = box_data_take(box);

		if (NULL == box_data) {
			DE("Critical error: box data == NULL but box->used > 0 (%d)\n", box_dump_header_p->box_size);
		}

		memcpy((buf + buf_offset), box_data, box_used);
		buf_offset += box_used;
	}

	DDD("Returning buffer, size: %zu, counted offset is: %zu\n", buf_size, buf_offset);
	*size = buf_size;
	return buf;
}

basket_t *basket_from_buf(void *buf, const size_t size)
{
	uint32_t             box_index;
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

	/* Now, we need to pre-allocate ->box pointers; grow it until we have enough */
	while (basket_buf_header->boxes_num > basket->boxes_allocated) {
		basket_grow_box_pointers(basket);
	}

	buf_offset = sizeof(basket_send_header_t);

	for (box_index = 0; box_index < basket_buf_header->boxes_num; box_index++) {

		/* Advance the pointer to the next box header */
		box_dump_header = (box_dump_t *)(buf_char + buf_offset);

		/* Test watermark */
		if (WATERMARK_BOX != box_dump_header->watermark) {
			DE("Wrong box[%u]: wrong watermark. Expected %X but it is %X\n", box_index, WATERMARK_BOX, box_dump_header->watermark);
			if (0 != basket_release(basket)) {
				DE("Could not release basket\n");
			}
			ABORT_OR_RETURN(NULL);
		}

		/* Advance pointer: right after the box header is the box data */
		buf_offset += sizeof(box_dump_t);

		/* If there no data, we do not even call the box creation, and the box pointer stays NULL */
		if (0 == box_dump_header->box_size) {
			continue;
		}

		/* Test that the memory area we pass to box_new_from_data_by_index() is in boundaries of the buf */
		if (buf_offset + box_dump_header->box_size > size) {
			DE("Wrong: The size of box[%u] overhead size of the whole buffer: %d = buf_offset (%d) + box_dump_header->box_size (%d) > size (%zu)\n",
			   box_index, buf_offset + box_dump_header->box_size, buf_offset, box_dump_header->box_size, size);
			abort();
		}

		/* Create a new box */
		if (OK != box_new_from_data_by_index(basket, box_index, buf_char + buf_offset, box_dump_header->box_size)) {
			DE("Adding a buffer size (%u) to tail of box (%u) failed\n", box_dump_header->box_size, box_index);


			if (0 != basket_release(basket)) {
				DE("Could not release basket\n");
			}
			ABORT_OR_RETURN(NULL);
		}

		/* Advance the offset */
		buf_offset += box_dump_header->box_size;

		/* Increase number of used boxes */
		basket->boxes_used++;
	}

	return basket;
}

/* This function creates a flat buffer ready for sending */
box_t *basket_to_buf_t(basket_t *basket)
{
	TESTP_ABORT(basket);
	return NULL;
	/* TODO */
}

/* This function restires Mbox object with boxes from the flat buffer */
basket_t *basket_from_buf_t(const box_t *buf)
{
	TESTP_ABORT(buf);
	return NULL;
	/* TODO */
}

/* Compare two backets, return 0 if they are equal, 1 if not, < 0 on an error */
int basket_compare_basket(basket_t *basket_left, basket_t *basket_right)
{
	box_u32_t box_index;
	int       memcmp_rc;
	TESTP(basket_left, -1);
	TESTP(basket_right, -1);

	if (basket_left->boxes_used != basket_right->boxes_used) {
		DDD("basket_left->boxes_used (%u) != basket_right->boxes_used (%u)\n",
			basket_left->boxes_used, basket_right->boxes_used);
		return 1;
	}

	for (box_index = 0; box_index < basket_right->boxes_used; box_index++) {
		box_t *box_right = basket_right->boxes[box_index];
		box_t *box_left  = basket_left->boxes[box_index];

		if (box_right != NULL && box_left == NULL) {
			DDD("box_right[%u] != NULL && box_left[%u] == NULL\n", box_index, box_index);
			return 1;
		}

		if (box_right == NULL && box_left != NULL) {
			DDD("box_right[%u] == NULL && box_left[%u] != NULL\n", box_index, box_index);
			return 1;
		}

		if (box_right == NULL && box_left == NULL) {
			continue;
		}

		if (box_right->used != box_left->used) {
			DDD("box_right[%u]->used (%ld) != box_left[%u]->used (%ld)\n",
				box_index, box_right->used, box_index, box_left->used);
			return 1;
		}

		memcmp_rc = memcmp(box_right->data, box_left->data, box_right->used);
		if (0 != memcmp_rc) {
			DDD("box_right[%u]->data != box_left[%u]->data at %d : data size is %lu\n",
				box_index, box_index, memcmp_rc, box_right->used);
			return 1;
		}
	}

	return 0;
}

/* Create a new basket and add data */
/* Testing function: basket_test.c::box_new_from_data_test() */
ssize_t box_new_from_data(basket_t *basket, const void *buffer, const box_u32_t buffer_size)
{
	box_t     *box;
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

	box_dump(box, "box_new_from_data(): got the last box from basket");

	if (NULL == box) {
		DE("Could not get pointer to the last box, number of boxes (%u) - stop\n",
		   basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	DDD("Going to add data in the tail of a new buf, new data size is %u\n", buffer_size);
	if (OK != box_add(box, buffer, buffer_size)) {
		DE("Could not add data into the last (new) box: new data size %u\n", buffer_size);
		ABORT_OR_RETURN(-1);
	}

	box_dump(box, "box_new_from_data(): Added new data");

	/* Return the number of the new box */
	return basket_get_last_box_index(basket);
}

/* Set a box in certain position; we need this function when restore basket from a flat buffer */
static ret_t box_new_from_data_by_index(basket_t *basket, box_u32_t box_index, const void *buffer, const box_u32_t buffer_size)
{
	box_t     *box;
	TESTP_ABORT(basket);
	TESTP_ABORT(buffer);

	/* Add a new box */
	while (basket->boxes_allocated < box_index) {
		if (0 != basket_grow_box_pointers(basket)) {
			ABORT_OR_RETURN(-1);
		}
	}

	if (NULL == basket->boxes[box_index]) {
		DDD("Allocating new box\n");
		box = box_new(0);
	} else {
		DDD("There is a box, use it\n");
		box = basket->boxes[box_index];
	}

	if (NULL == box) {
		ABORT_OR_RETURN(-1);
	}

	DDD("Going to add data in the tail of a the buf[%u], new data size is %u\n", box_index, buffer_size);
	if (OK != box_add(box, buffer, buffer_size)) {
		DE("Could not add data into the new box[%u]: new data size %u\n", box_index, buffer_size);
		ABORT_OR_RETURN(-1);
	}

	box_dump(box, "box_new_from_data(): Added new data");

	/* Now set the box at given index */
	basket->boxes[box_index] = box;

	/* Return the number of the new box */
	return OK;
}

/* Add data to a basket */
ret_t box_add_to_tail(basket_t *basket, const box_u32_t box_num, const void *buffer, const size_t buffer_size)
{
	box_t     *box;
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

	return box_add(box, buffer, buffer_size);
}

ret_t box_replace_data(basket_t *basket, const box_u32_t box_num, const void *buffer, const size_t buffer_size)
{
	box_t *box;
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

	return box_replace(box, buffer, buffer_size);
}

void *box_data_ptr(const basket_t *basket, const box_u32_t box_num)
{
	box_t     *box;
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

	return box_data_take(box);
}

ssize_t box_data_size(const basket_t *basket, const box_u32_t box_num)
{
	box_t     *box;
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

	return box_used_take(box);
}

ret_t box_data_free(basket_t *basket, const box_u32_t box_num)
{
	box_t *box;
	TESTP_ABORT(basket);

	box = basket_get_box(basket, box_num);

	/* The box can be NULL, it is a valid situation */
	if (NULL == box) {
		return 0;
	}

	return box_clean_and_reset(box);
}

void *box_steal_data(basket_t *basket, const box_u32_t box_num)
{
	TESTP_ABORT(basket);

	box_t  *box = basket_get_box(basket, box_num);

	/* The box can be NULL, it is a valid situation */
	if (NULL == box) {
		return NULL;
	}

	/* The box can be empty, in this case no buffer is in buf_t */
	if (0 == box_used_take(box)) {
		return NULL;
	}

	return box_data_steal(box);
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
		box_u32_t i;

		for (i = 0; i < basket->boxes_used; i++) {
			box_t *box = basket_get_box(basket, i);

			DD("`````````````````````````\n");

			DD(">>> Box[%u] ptr:        %p\n", i, box);
			if (NULL == box) continue;

			DD(">>> Box[%u] used:       %ld\n", i, box_used_take(box));
			DD(">>> Box[%u] room:       %ld\n", i, box_room_take(box));
			DD(">>> Box[%u] data ptr:   %p\n", i, box_data_take(box));

		}
	}
	DD("^^^^^^^^^^^^^^^^^^^^^^^^^\n");
}

