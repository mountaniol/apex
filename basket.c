#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "basket.h"
#include "box_t.h"
#include "debug.h"
#include "tests.h"
#include "checksum.h"
#include "optimization.h"

static ret_t box_new_from_data_by_index(void *basket, box_u32_t box_index, const void *buffer, const box_u32_t buffer_size);

/**
 * @author Sebastian Mountaniol (7/27/22)
 * @brief 'free()' wrapper
 * @param void* mem   Memory to free
 * @param const char* who   The nama od caller
 * @param const int line  The string of caller
 * @details Use: basket_free_mem(mem, __func__, __LINE__);
 */
#
#ifdef DEBUG3
static void basket_free_mem(void *mem, const char *who, const int line)
#else
static void basket_free_mem(void *mem,
							__attribute__((unused))const char *who,
							__attribute__((unused))const int line)
#endif
{
	DDD("FREE / %s +%d / %p\n", who, line, mem);
	TESTP_ABORT(mem);
	free(mem);
}

FATTR_COLD FATTR_UNUSED static void basket_dump(void *basket, const char *msg)
{
	const basket_t *_basket = basket;
	TESTP_ABORT(_basket);
	TESTP_ABORT(msg);

	DD("~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	DD("%s\n", msg);
	DD("Basket ptr:            %p\n", _basket);
	DD("Basket number of bufs: %u\n", _basket->boxes_used);
	DD("Basket bufs[] ptr    : %p\n", _basket->boxes);

	if (_basket->boxes_used > 0) {
		box_u32_t i;

		for (i = 0; i < _basket->boxes_used; i++) {
			box_t *box = basket_get_box(_basket, i);

			DD("`````````````````````````\n");

			DD(">>> Box[%u] ptr:        %p\n", i, box);
			if (NULL == box) continue;

			DD(">>> Box[%u] used:       %ld\n", i, bx_used_take(box));
			DD(">>> Box[%u] room:       %ld\n", i, bx_room_take(box));
			DD(">>> Box[%u] data ptr:   %p\n", i, bx_data_take(box));

		}
	}
	DD("^^^^^^^^^^^^^^^^^^^^^^^^^\n");
}

FATTR_WARN_UNUSED_RET void *basket_new(void)
{
	/* basket: pointer to the allocated memory */
	basket_t *basket = malloc(sizeof(basket_t));
	if (NULL == basket) {
		DE("Can't allocate\n");
		return (NULL);
	}

	memset(basket, 0, sizeof(basket_t));
	return (basket);
}

FATTR_WARN_UNUSED_RET ret_t basket_release(void *basket)
{
	basket_t *_basket = basket;
	TESTP_ABORT(_basket);

	/* TODO: Deallocate boxes */
	if (_basket->boxes) {
		/* Index variable to iterate boxes */
		box_u32_t box_index;

		for (box_index = 0; box_index < _basket->boxes_used; box_index++) {
			if (bx_free(_basket->boxes[box_index]) < 0) {
				DE("Could not release box[%u]\n", box_index);
				ABORT_OR_RETURN(-1);
			}
		}

		basket_free_mem(_basket->boxes, __func__, __LINE__);
		DDD("Freed basket->boxes\n");
	}

	if (_basket->zhash) {
		zhash_release(_basket->zhash, 1);
	}

	/* Secure way: clear memory before release it */
	basket_free_mem(_basket, __func__, __LINE__);
	return 0;
}

/*** Getters / Setters */

/* Internal function: get pointer of a box inside of the basket */
FATTR_HOT void *basket_get_box(const void *basket, const box_u32_t box_index)
{
	const basket_t *_basket = basket;
	TESTP_ABORT(_basket);
	TESTP_ABORT(_basket->boxes);
	if (box_index > _basket->boxes_used) {
		DE("Asked to get a box out of range\n");
		ABORT_OR_RETURN(NULL);
	}

	DDD("Returning pointer to box %u (%p)\n", box_index, _basket->boxes[box_index]);
	return (_basket->boxes[box_index]);
}

/**
 * @author Sebastian Mountaniol (7/27/22)
 * @brief Get pointer to the last box. Can not be NULL.
 * @param const basket_t* basket Basket to get poiter to the
 *  			last box
 * @return box_t* Pointer to the last box. NULL if no box
 *  	   allocated.
 * @details NULL pointer is not an error. This function will
 *  		crash on error. Error conditions: 
 */
FATTR_WARN_UNUSED_RET static void *basket_get_last_box(const void *basket)
{
	const basket_t *_basket = basket;;
	TESTP_ABORT(_basket);
	if (0 == _basket->boxes_used) {
		return NULL;
	}

	DDD("The boxes count is %u, last box index is %u, num of pointers is %u, returning ptr %p\n",
		_basket->boxes_used, _basket->boxes_used - 1, _basket->boxes_allocated, /*boxes[number - 1]*/ _basket->boxes[_basket->boxes_used - 1]);
	return _basket->boxes[_basket->boxes_used - 1];
}

FATTR_WARN_UNUSED_RET static box_s64_t basket_get_last_box_index(const void *basket)
{
	const basket_t *_basket = basket;
	TESTP_ABORT(_basket);
	return _basket->boxes_used - 1;
}

FATTR_WARN_UNUSED_RET size_t basket_memory_size(const void *basket)
{
	const basket_t *_basket  = basket;
	/* Index variable to iterate boxes */
	box_u32_t      box_index = 0;
	/* Accumulator variable to count size */
	size_t         size      = 0;

	TESTP_ABORT(_basket);

	/* Size of the basket_t structure */
	size = sizeof(basket_t);

	/* Number of pointers in ->boxes array */
	size += _basket->boxes_allocated * sizeof(void *);

	for (box_index = 0; box_index < _basket->boxes_used; box_index++) {
		const box_t  *box = basket_get_box(_basket, box_index);

		/* Size of box_t structure */
		size += sizeof(box_t);
		/* It can be NULL; normal */
		if (NULL == box) {
			continue;
		}

		/* Add size of the data buffer */
		size += bx_room_take(box);
	}
	return size;
}

/* Clean all boxes */
FATTR_WARN_UNUSED_RET ret_t basket_clean(void *basket)
{
	basket_t *_basket  = basket;
	uint32_t box_index;
	TESTP_ABORT(_basket);

	/* Validity test 1 */
	if (NULL == _basket->boxes && _basket->boxes_used > 0) {
		DE("Error: if number of boxes > 0 (%d), the boxes array must not be NULL pointer, but it is\n", _basket->boxes_used);
		abort();
	}

	/* Validity test 2 */
	if (NULL != _basket->boxes && _basket->boxes_used == 0) {
		DE("Error: if number of boxes == 0, the boxes array must not be NULL pointer, but it is not : %p\n", _basket->boxes);
		abort();
	}

	if (NULL == _basket->boxes) {
		DDD("Not cleaning boxes because they are not exist: ->boxes ptr : %p, num of boxes: %u\n", _basket->boxes, _basket->boxes_used);
		return 0;
	}

	DDD("Cleaning basket->boxes_used (%u) boxes (%p)\n", _basket->boxes_used, _basket->boxes);
	for (box_index = 0; box_index < _basket->boxes_used; box_index++) {

		/* It can be NULL; It is normal */
		if (_basket->boxes[box_index]) {
			continue;
		}

		if (OK != bx_free(_basket->boxes[box_index])) {
			DE("Could not free box\n");
			ABORT_OR_RETURN(-1);
		}
	}

	DDD("Free() basket->boxes_used boxes (%p)\n", _basket->boxes);
	if (NULL != _basket->boxes) {
		free(_basket->boxes);
		_basket->boxes = NULL;
	}

	_basket->boxes_used = 0;
	basket_free_mem(_basket, __func__, __LINE__);
	return 0;
}

FATTR_WARN_UNUSED_RET static ret_t basket_grow_box_pointers(void *basket)
{
	basket_t *_basket             = basket;
	void     *reallocated_mem     = NULL;

	char     *clean_start_bytes_p = NULL;
	size_t   clean_szie_bytes;

	TESTP_ABORT(_basket);

	DDD("Going to call reallocarray(boxes = %p, basket->boxes_allocated + BASKET_BUFS_GROW_RATE = %u, sizeof(void *) = %zu)\n",
		_basket->boxes, _basket->boxes_allocated + BASKET_BUFS_GROW_RATE, sizeof(void *));

	reallocated_mem = reallocarray(_basket->boxes, _basket->boxes_allocated + BASKET_BUFS_GROW_RATE, sizeof(void *));
	if (NULL == reallocated_mem) {
		DE("Allocation failed\n");
		ABORT_OR_RETURN(-1);
	}

	/* Clean the freshly allocated memory */
	clean_start_bytes_p = (char *)reallocated_mem + (_basket->boxes_allocated * sizeof(void *));
	clean_szie_bytes = BASKET_BUFS_GROW_RATE * sizeof(void *);
	//DD("Going to clean: tmp (%p) + basket->boxes_allocated (%u) == %p, 0, BASKET_BUFS_GROW_RATE (%d) * sizeof(void *) %(zu)\n",
	//   (char *) tmp, basket->boxes_allocated, (tmp + basket->boxes_allocated), BASKET_BUFS_GROW_RATE, sizeof(void *) );
	memset(clean_start_bytes_p, 0, clean_szie_bytes);


	/* If we need to allocate more pointers in basket->boxes array,
	 * we allocate more than on.
	 * To be exact, we allocate BASKET_BUFS_GROW_RATE number of pointers.
	   This way we decrease delays when user adds new boxes */
	_basket->boxes_allocated += BASKET_BUFS_GROW_RATE;

	if (reallocated_mem != _basket->boxes) {
		/*
		 * TODO: Free old box?
		 * When it uncommented, we have a crash caused double free here
		 */
		// free(basket->boxes);
		_basket->boxes = reallocated_mem;
	}

	return 0;
}

/* TODO: Not tested yet */
FATTR_WARN_UNUSED_RET ret_t box_insert_after(void *basket, const uint32_t after_index, const void *buffer, const box_u32_t buffer_size)
{
	basket_t  *_basket                = basket;
	box_t     *box;
	size_t    how_many_bytes_to_move;
	box_u32_t move_start_offset_bytes;

	void      *move_start_p           = NULL;
	void      *move_end_p             = NULL;
	TESTP_ABORT(_basket);
	TESTP_ABORT(_basket->boxes);

	/* Test that we have enough allocated slots in basket->boxes to move all by 1 position */
	if ((_basket->boxes_used + 1) > _basket->boxes_allocated) {
		if (0 != basket_grow_box_pointers(_basket)) {
			ABORT_OR_RETURN(-1);
		}
	}

	/* Move memory */
	how_many_bytes_to_move = (_basket->boxes_used - after_index) * sizeof(void *);
	move_start_offset_bytes = (after_index + 1) * sizeof(void *);

	/* We start here ... */
	move_start_p = _basket->boxes + move_start_offset_bytes;
	/*... and move to free one void * pointer, where we want to insert a new box */
	move_end_p = _basket->boxes + move_start_offset_bytes + sizeof(void *);

	memmove(/* Dst */move_end_p, /* Src */ move_start_p, /* Size */ how_many_bytes_to_move);

	/* Insert a new box*/
	box = bx_new(0);
	if (NULL == box) {
		ABORT_OR_RETURN(-1);
	}

	if (OK != bx_add(box, buffer, buffer_size)) {
		bx_free(box);
		ABORT_OR_RETURN(-1);
	}

	_basket->boxes[after_index + 1] = box;


	/* Increase boxes count */
	_basket->boxes_used++;

	/* We are done */
	return 0;
}

/* TODO: Not tested */
FATTR_WARN_UNUSED_RET ret_t box_swap(void *basket, const box_u32_t first_index, const box_u32_t second_index)
{
	basket_t *_basket = basket;
	box_t    *tmp;
	TESTP_ABORT(_basket);

	if (first_index > _basket->boxes_used || second_index > _basket->boxes_used) {
		DE("One of asked boxes is out of range, first = %u, second = %u, number of boxes is %u\n",
		   first_index, second_index, _basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	tmp = _basket->boxes[first_index];
	_basket->boxes[first_index] = _basket->boxes[second_index];
	_basket->boxes[second_index] = tmp;
	return 0;
}

FATTR_WARN_UNUSED_RET ret_t box_clean(void *basket, const box_u32_t box_index)
{
	basket_t *_basket = basket;
	box_t    *box;
	TESTP_ABORT(_basket);
	if (box_index > _basket->boxes_used) {
		DE("Asked to remove a box (%u) out of range (%u)\n",
		   box_index, _basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	box = basket_get_box(_basket, box_index);

	TESTP(box, -1);

	if (OK != bx_clean_and_reset(box)) {
		DE("An error on box removal (buf_clean_and_reset failed)\n");
		ABORT_OR_RETURN(-1);
	}

	/* TOOD: If this box is the last box, run realloc and remove unused memory */
	return 0;
}

FATTR_WARN_UNUSED_RET ret_t box_merge_box(void *basket, const box_u32_t src, const box_u32_t dst)
{
	basket_t *_basket = basket;
	box_t    *box_src;
	box_t    *box_dst;
	ret_t    rc;
	TESTP_ABORT(_basket);

	if (src > _basket->boxes_used || dst > _basket->boxes_used) {
		DE("Src (%u) or dst (%u) box is out of range (%u)\n", src, dst, _basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	box_src = basket_get_box(_basket, src);

	if (NULL == box_src) {
		DD("Source box is NULL, no action needed\n");
		return 0;
	}

	if (0 == bx_used_take(box_src)) {
		DD("Source box is there, but it is empty, no action needded\n");
		return 0;
	}

	box_dst = basket_get_box(_basket, dst);

	/*
	 * If we are here, it means:
	 * - There are bos 'src' and 'dst' boxes
	 * - There is some data in 'src' box
	 */
	rc = bx_add(box_dst, bx_data_take(box_src), bx_used_take(box_src));
	if (rc) {
		DE("Could not add data from box src (%u) to dst (%u)\n", src, dst);
		ABORT_OR_RETURN(rc);
	}

	return box_clean(_basket, src);
}

FATTR_WARN_UNUSED_RET ret_t box_bisect(__attribute__((unused))void *basket,
									   __attribute__((unused)) const box_u32_t box_num,
									   __attribute__((unused)) const size_t from_offset)
{
	return -1;
	/* TODO */
}

FATTR_WARN_UNUSED_RET ret_t basket_collapse(void *basket)
{
	basket_t  *_basket  = basket;
	box_u32_t box_index;
	TESTP_ABORT(_basket);
	TESTP_ABORT(_basket->boxes);

	// basket_dump(basket, "Going to collapse in place");

	/* We need two or more boxes to run this function */
	if (_basket->boxes_used < 2) {
		return 0;
	}

	for (box_index = 1; box_index < _basket->boxes_used; box_index++) {
		if (NULL == _basket->boxes[box_index]) {
			DD("Do not merge box[%u] of %u, because it is NULL pointer\n", box_index, _basket->boxes_used);
			continue;
		}

		if (OK != bx_merge(_basket->boxes[0], _basket->boxes[box_index])) {
			ABORT_OR_RETURN(-1);
		}
	}

	return 0;
}

size_t basket_flat_buf_size(const basket_t *basket)
{
	box_u32_t box_index;
	size_t    buf_size  = sizeof(basket_send_header_t);

	/* And we need the struct box_dump_t per box */
	buf_size += sizeof(box_dump_t) * basket->boxes_used;

	/* We need to dump the content of every box, so count total size of buffers in all boxes */
	for (box_index = 0; box_index < basket->boxes_used; box_index++) {
		const box_t *box = basket_get_box(basket, box_index);
		buf_size += bx_used_take(box);
	}

	/* If there key/value hash, add the size of needded to dump it into calculation */
	if (basket->zhash) {
		buf_size += zhash_to_buf_allocation_size(basket->zhash);
	}
	return buf_size;
}

/**
 * @author Sebastian Mountaniol (7/31/22)
 * @brief Fill the basket_send_header_t from basket_t structure 
 * @param basket_send_header_t* basket_buf_header_p
 * @param const basket_t* basket             
 * @param const size_t buf_size           
 * @details 
 */
static void basket_fill_send_header_from_basket_t(basket_send_header_t *basket_buf_header_p,
												  const basket_t *basket)
{
	box_u32_t box_index;
	size_t    buf_size  = sizeof(basket_send_header_t);

	/* And we need the struct box_dump_t per box */
	buf_size += sizeof(box_dump_t) * basket->boxes_used;

	/* We need to dump the content of every box, so count total size of buffers in all boxes */
	for (box_index = 0; box_index < basket->boxes_used; box_index++) {
		const box_t *box = basket_get_box(basket, box_index);
		buf_size += bx_used_take(box);
	}

	basket_buf_header_p->ticket = basket->ticket;
	basket_buf_header_p->total_len = buf_size;
	basket_buf_header_p->boxes_num = basket->boxes_used;

	basket_buf_header_p->total_len = buf_size;
	basket_buf_header_p->boxes_num = basket->boxes_used;

	/* Watermark: a predefined pattern. */
	basket_buf_header_p->watermark = WATERMARK_BASKET;

	/* If there is key/value, hget the size */
	if (basket->zhash) {
		basket_buf_header_p->ztable_buf_size = (uint32_t)zhash_to_buf_allocation_size(basket->zhash);
	}

	/* TODO: The checksum not implemented yet */
	basket_buf_header_p->checksum = 0;
}

void basket_checksum_set(basket_send_header_t *basket_buf_header_p)
{
	/* We start the checksum from 'ticket',
	   we do not consider watermark and the checksum fields */
	size_t start_offset      = offsetof(basket_send_header_t, ticket);
	/* Size to count - the size of this buffer less two filelds, 'watermark' and 'checksum' */
	size_t size_to_count     = basket_buf_header_p->total_len - start_offset;
	/* Start from 'ticket' field */
	char   *address_to_start = (char *)basket_buf_header_p + start_offset;

	if (0 != checksum_buf_to_32_bit(address_to_start, size_to_count, &basket_buf_header_p->checksum)) {
		DE("Failed to calculate the final buffer checksum");
		abort();
	}

	DDD("Checksum set: %X\n", basket_buf_header_p->checksum);
}

int basket_checksum_test(const basket_send_header_t *basket_buf_header_p)
{
	int      rc                = -1;
	uint32_t calculated_sum    = 0XDEADBEEF;

	/* We start the checksum from 'ticket',
		we do not consider watermark and the checksum fields */
	size_t   start_offset      = 0;
	/* Size to count - the size of this buffer less two filelds, 'watermark' and 'checksum' */
	size_t   size_to_count     = 0;
	/* Start from 'ticket' field */
	char     *address_to_start = NULL;

	TESTP(basket_buf_header_p, -1);

	start_offset      = offsetof(basket_send_header_t, ticket);
	size_to_count     = basket_buf_header_p->total_len - start_offset;
	address_to_start = (char *)basket_buf_header_p + start_offset;

	rc = checksum_buf_to_32_bit(address_to_start, size_to_count, &calculated_sum);

	if (0 != rc) {
		DE("Failed to calculate the final buffer checksum");
		abort();
	}

	if (basket_buf_header_p->checksum != calculated_sum) {
		DE("Wrong checksum: expected %X but it is %X\n", basket_buf_header_p->checksum, calculated_sum);
		return 1;
	}
	return 0;
}

/**
 * @author Sebastian Mountaniol (7/31/22)
 * @brief Fill the box_dump_t from the box_t structure
 * @param box_dump_t* box_dump_header_p The pointer to
 *  				box_dump_t structure. WARNING: THis pointer
 *  				CAN be NULL, it is a legal situation!
 * @param const box_t* box The pointer to box_t structure
 * @return size_t The size (in bytes) of the resulting dump
 * @details This function assuption that the box_dump_header_p
 *  		points to a buffer big enoght for the box_dump_t
 *  		structure and all dumped data from the box. 
 */
FATTR_WARN_UNUSED_RET static size_t basket_fill_send_box_from_box_t(box_dump_t *box_dump_header_p, const box_t *box)
{
	char      *buf       = (char *)box_dump_header_p;
	char      *box_data;
	box_s64_t box_used   = 0;
	size_t    buf_offset = 0;

	/* Fill the header, for every box we create and fill the header */
	if (box) {
		box_used = bx_used_take(box);
	}

	box_dump_header_p->box_size = box_used;
	box_dump_header_p->watermark = WATERMARK_BOX;

	// memcpy(buf + buf_offset, &box_dump_header, sizeof(box_dump_t));
	buf_offset += sizeof(box_dump_t);

	/* If this box is NULL, or is empty, we don't add any data and skip */
	if (0 == box_used) return buf_offset;

	box_data = bx_data_take(box);

	if (NULL == box_data) {
		DE("Critical error: box data == NULL but box->used > 0 (%d)\n", box_dump_header_p->box_size);
		abort();
	}

	memcpy((buf + buf_offset), box_data, box_used);
	buf_offset += box_used;

	return buf_offset;
}

/* This function creates a flat buffer ready for sending/saving */
FATTR_WARN_UNUSED_RET void *basket_to_buf(const void *basket, size_t *size)
{
	const basket_t       *_basket             = basket;
	box_u32_t            box_index;
	char                 *buf;
	size_t               buf_size             = 0;
	size_t               buf_offset           = 0;

	basket_send_header_t *basket_buf_header_p;
	box_dump_t           *box_dump_header_p;

	TESTP_ABORT(_basket);
	TESTP_ABORT(size);

	/* Let's calculate required buffer size */

	/*** 1. Fill the header of this buffer ***/

	buf_size             = basket_flat_buf_size(_basket);
	/* Alocate the memory buffer */
	buf = malloc(buf_size);
	TESTP(buf, NULL);

	memset(buf, 0, buf_size);

	/* Fill the header */
	basket_buf_header_p = (basket_send_header_t *)buf;
	basket_fill_send_header_from_basket_t(basket_buf_header_p, basket);

	/* Advance memory byffer poiter */
	buf_offset += sizeof(basket_send_header_t);


	/*** 2. Dump all boxes ***/

	/* Now we run on array of boxes, and add one by one to the buffer */

	for (box_index = 0; box_index < _basket->boxes_used; box_index++) {
		const box_t *box      = basket_get_box(_basket, box_index);
		box_dump_header_p = (box_dump_t *)(buf + buf_offset);

		/* Advance memory byffer pointer bu size of the returned offset */
		buf_offset += basket_fill_send_box_from_box_t(box_dump_header_p, box);
	}

	/*** 3. Dump key/value hash ***/
	/* TODO: Pass the buffer to fill to zhash from outside */
	if (_basket->zhash) {
		size_t zsize;
		char   *zhash_dump = zhash_to_buf(_basket->zhash, &zsize);
		TESTP_ABORT(zhash_dump);
		memcpy((buf + buf_offset), zhash_dump, zsize);
		free(zhash_dump);
		buf_offset += zsize;
	}

	/* Calculate and set the buffer checksum */
	/* This call can not fail; on failure the execution will be terminated */
	basket_checksum_set(basket_buf_header_p);

	DDD("Returning buffer, size: %zu, counted offset is: %zu\n", buf_size, buf_offset);
	*size = buf_size;
	return buf;
}

FATTR_WARN_UNUSED_RET void *basket_from_buf(void *buf, const size_t size)
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

	/* Test the buffer checksum */
	if (0 != basket_checksum_test(basket_buf_header)) {
		DE("A wrong buffer, checksum not match\n");
		return NULL;
	}


	basket = basket_new();
	TESTP(basket, NULL);

	/* Now, we need to pre-allocate ->box pointers; grow it until we have enough */
	while (basket_buf_header->boxes_num > basket->boxes_allocated) {
		if (OK != basket_grow_box_pointers(basket)) {
			DE("Could not grow ->bufs pointers\n");
			ABORT_OR_RETURN(NULL);
		}
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

	/*** Restore zhash, is presents ***/
	if (basket_buf_header->ztable_buf_size > 0) {
		basket->zhash = zhash_from_buf(buf_char + buf_offset, basket_buf_header->ztable_buf_size);
		buf_offset += basket_buf_header->ztable_buf_size;
	}

	return basket;
}

FATTR_WARN_UNUSED_RET int box_compare_box(const void *box_left, const void *box_right)
{
	const box_t *_box_left  = box_left;
	const box_t *_box_right = box_right;
	int         memcmp_rc;
	/* Both boxes must be either NULL or not NULL */
	if (_box_right != NULL && _box_left == NULL) {
		DDD("box_right != NULL && box_left == NULL\n");
		return 1;
	}

	/* Both boxes must be either NULL or not NULL */
	if (_box_right == NULL && _box_left != NULL) {
		DDD("box_right == NULL && box_left != NULL\n");
		return 1;
	}

	if (_box_right == NULL && _box_left == NULL) {
		return 0;
	}

	if (_box_right->used != _box_left->used) {
		DDD("box_right->used (%ld) != box_left->used (%ld)\n", _box_right->used, _box_left->used);
		return 1;
	}

	memcmp_rc = memcmp(_box_right->data, _box_left->data, _box_right->used);
	if (0 != memcmp_rc) {
		DDD("box_right->data != box_left->data at %d : data size is %lu\n", memcmp_rc, _box_right->used);
		return 1;
	}

	return 0;
}

/* Compare two backets, return 0 if they are equal, 1 if not, < 0 on an error */
FATTR_WARN_UNUSED_RET int basket_compare_basket(const void *basket_right, const void *basket_left)
{
	const basket_t *_basket_right = basket_right;
	const basket_t *_basket_left  = basket_left;
	box_u32_t      box_index;
	TESTP(_basket_left, -1);
	TESTP(_basket_right, -1);

	if (_basket_left->boxes_used != _basket_right->boxes_used) {
		DDD("basket_left->boxes_used (%u) != basket_right->boxes_used (%u)\n",
			_basket_left->boxes_used, _basket_right->boxes_used);
		return 1;
	}

	for (box_index = 0; box_index < _basket_right->boxes_used; box_index++) {
		box_t *box_right = _basket_right->boxes[box_index];
		box_t *box_left  = _basket_left->boxes[box_index];

		if (0 != box_compare_box(box_left, box_right)) {
			DDD("Box[%u] in left and right basket is not the same\n", box_index);
			return 1;
		}
	}

	return 0;
}

/* Add a new box */
FATTR_WARN_UNUSED_RET static ret_t box_add_new(void *basket)
{
	basket_t *_basket = basket;
	TESTP_ABORT(_basket);

	/* If no boxes, we should allocate - call 'grow' func */
	if (NULL == _basket->boxes) {
		if (OK != basket_grow_box_pointers(_basket)) {
			DE("Can not grow ->boxes pointers\n");
			ABORT_OR_RETURN(-1);
		}
	}

	/* Here it MUST be not NULL */
	if (NULL == _basket->boxes) {
		ABORT_OR_RETURN(-1);
	}

	if (_basket->boxes_used == _basket->boxes_allocated) {
		if (OK != basket_grow_box_pointers(_basket)) {
			DE("Could not grow basket->boxes\n");
			ABORT_OR_RETURN(-1);
		}
	}

	/* The 'buf_new()' allocates a new box and cleans it, no need to clean */
	_basket->boxes[_basket->boxes_used] = bx_new(0);
	_basket->boxes_used++;
	DDD("Allocated a new box, set at index %u\n", _basket->boxes_used - 1);
	bx_dump(_basket->boxes[_basket->boxes_used - 1], "box_add_new(): added a new box, must be all 0/NULL");
	return 0;
}

/* Create a new box in the basket and add data */
/* Testing function: basket_test.c::box_new_from_data_test() */
FATTR_WARN_UNUSED_RET ssize_t box_new(void *basket, const void *buffer, const box_u32_t buffer_size)
{
	basket_t *_basket = basket;
	box_t    *box;
	TESTP_ABORT(_basket);
	TESTP_ABORT(buffer);

	/* Add a new box */
	if (0 != box_add_new(_basket)) {
		DE("Can not add another box into Basket\n");
		ABORT_OR_RETURN(-1);
	}

	/* We allocated an additional box. Thus, number of boxes MUST be > 0 */
	if (0 == _basket->boxes_used) {
		DE("The number of boxes must not be 0 here, but it is\n");
		ABORT_OR_RETURN(-1);
	}

	box = basket_get_last_box(_basket);

	bx_dump(box, "box_new_from_data(): got the last box from basket");

	if (NULL == box) {
		DE("Could not get pointer to the last box, number of boxes (%u) - stop\n",
		   _basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	DDD("Going to add data in the tail of a new buf, new data size is %u\n", buffer_size);
	if (OK != bx_add(box, buffer, buffer_size)) {
		DE("Could not add data into the last (new) box: new data size %u\n", buffer_size);
		ABORT_OR_RETURN(-1);
	}

	bx_dump(box, "box_new_from_data(): Added new data");

	/* Return the number of the new box */
	return basket_get_last_box_index(_basket);
}

/* Set a box in certain position; we need this function when restore basket from a flat buffer */
FATTR_WARN_UNUSED_RET static ret_t box_new_from_data_by_index(void *basket, box_u32_t box_index, const void *buffer, const box_u32_t buffer_size)
{
	basket_t *_basket = basket;
	box_t    *box;
	TESTP_ABORT(_basket);
	TESTP_ABORT(buffer);

	/* Add a new box */
	while (_basket->boxes_allocated < box_index) {
		if (0 != basket_grow_box_pointers(_basket)) {
			ABORT_OR_RETURN(-1);
		}
	}

	if (NULL == _basket->boxes[box_index]) {
		DDD("Allocating new box\n");
		box = bx_new(0);
	} else {
		DDD("There is a box, use it\n");
		box = _basket->boxes[box_index];
	}

	if (NULL == box) {
		ABORT_OR_RETURN(-1);
	}

	DDD("Going to add data in the tail of a the buf[%u], new data size is %u\n", box_index, buffer_size);
	if (OK != bx_add(box, buffer, buffer_size)) {
		DE("Could not add data into the new box[%u]: new data size %u\n", box_index, buffer_size);
		ABORT_OR_RETURN(-1);
	}

	bx_dump(box, "box_new_from_data(): Added new data");

	/* Now set the box at given index */
	_basket->boxes[box_index] = box;

	/* Return the number of the new box */
	return OK;
}

/* Add data to a basket */
FATTR_WARN_UNUSED_RET ret_t box_add(void *basket, const box_u32_t box_num, const void *buffer, const size_t buffer_size)
{
	basket_t *_basket = basket;
	box_t    *box;
	TESTP_ABORT(_basket);
	TESTP_ABORT(buffer);

	if (box_num > _basket->boxes_used) {
		DE("Asked box is out of range: asked box %u, number of boxes is %u\n", box_num, _basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}

	box = basket_get_box(_basket, box_num);
	if (NULL == box) {
		DE("Could not get box pointer - stopping\n");
		ABORT_OR_RETURN(-1);
	}

	return bx_add(box, buffer, buffer_size);
}

FATTR_WARN_UNUSED_RET ret_t box_data_replace(void *basket, const box_u32_t box_num, const void *buffer, const size_t buffer_size)
{
	basket_t *_basket = basket;
	box_t    *box;
	TESTP_ABORT(_basket);
	TESTP_ABORT(buffer);
	if (buffer_size < 1) {
		DE("The new buffer size must be > 0\n");
		return -EINVAL;
	}

	box = basket_get_box(_basket, box_num);
	if (NULL == box) {
		DE("Could not take pointer for the box (%u) - stopping\n", box_num);
		ABORT_OR_RETURN(-1);
	}

	return bx_replace_data(box, buffer, buffer_size);
}

FATTR_WARN_UNUSED_RET void *box_data_ptr(const void *basket, const box_u32_t box_num)
{
	const basket_t *_basket = basket;
	box_t          *box;
	TESTP_ABORT(_basket);
	TESTP_ABORT(_basket->boxes);

	if (box_num > _basket->boxes_used) {
		DE("Asked box (%u) is out of range (%d)\n", box_num, _basket->boxes_used);
		ABORT_OR_RETURN(NULL);
	}

	box = basket_get_box(_basket, box_num);

	/* This should not happen never */
	if (NULL == box) {
		DE("There is no buffer by asked index: %u\n", box_num);
		abort();
	}

	return bx_data_take(box);
}

FATTR_WARN_UNUSED_RET FATTR_CONST ssize_t box_data_size(const void *basket, const box_u32_t box_num)
{
	const basket_t *_basket = basket;
	box_t          *box;
	TESTP_ABORT(_basket);

	if (box_num > _basket->boxes_used) {
		DE("Asked box (%u) is out of range (%d)\n", box_num, _basket->boxes_used);
		ABORT_OR_RETURN(-1);
	}
	box = basket_get_box(_basket, box_num);

	/* Never should happen */
	if (NULL == box) {
		DE("There is no buffer by asked index: %u\n", box_num);
		ABORT_OR_RETURN(-1);
	}

	return bx_used_take(box);
}

FATTR_WARN_UNUSED_RET ret_t box_data_free(void *basket, const box_u32_t box_num)
{
	const basket_t *_basket = basket;
	box_t          *box;
	TESTP_ABORT(_basket);

	box = basket_get_box(_basket, box_num);

	/* The box can be NULL, it is a valid situation */
	if (NULL == box) {
		return 0;
	}

	return bx_clean_and_reset(box);
}

FATTR_WARN_UNUSED_RET FATTR_CONST void *box_steal_data(void *basket, const box_u32_t box_num)
{
	const basket_t *_basket = basket;
	TESTP_ABORT(_basket);

	box_t  *box = basket_get_box(_basket, box_num);

	/* The box can be NULL, it is a valid situation */
	if (NULL == box) {
		return NULL;
	}

	/* The box can be empty, in this case no buffer is in buf_t */
	if (0 == bx_used_take(box)) {
		return NULL;
	}

	return bx_data_steal(box);
}


/*** KEY/VALUE ***/

/* Allocate zhash, if not allocated yet */
static void basket_validate_zhash(void *basket)
{
	basket_t *_basket = basket;
	if (NULL == _basket->zhash) {
		_basket->zhash = zhash_allocate();
	}

	TESTP_ABORT(_basket->zhash);
}

ret_t basket_keyval_add_by_int64(void *basket, uint64_t key_int64, void *val, size_t val_size)
{
	TESTP(basket, -1);
	TESTP(val, -1);

	basket_t *_basket = basket;
	basket_validate_zhash(basket);
	return zhash_insert_by_int(_basket->zhash, key_int64, val, val_size);
}

ret_t basket_keyval_add_by_str(void *basket, char *key_str, size_t key_str_len, void *val, size_t val_size)
{
	TESTP(basket, -1);
	TESTP(val, -1);

	basket_t *_basket = basket;
	basket_validate_zhash(basket);
	return zhash_insert_by_str(_basket->zhash, key_str, key_str_len, val, val_size);
}

uint64_t basket_keyval_str_to_int64(char *key_str, size_t key_str_len)
{
	TESTP(key_str, 0);
	return zhash_key_int64_from_key_str(key_str, key_str_len);
}

void *basket_keyval_find_by_int64(void *basket, uint64_t key_int64, ssize_t *val_size)
{
	TESTP(basket, NULL);
	TESTP(val_size, NULL);

	basket_t *_basket = basket;
	if (NULL == _basket->zhash) {
		*val_size = -1;
		return NULL;
	}
	basket_validate_zhash(basket);
	return zhash_find_by_int(_basket->zhash, key_int64, val_size);
}

void *basket_keyval_find_by_str(void *basket, char *key_str, size_t key_str_len, ssize_t *val_size)
{
	TESTP(basket, NULL);
	TESTP(key_str, NULL);
	TESTP(val_size, NULL);

	basket_t *_basket = basket;
	if (NULL == _basket->zhash) {
		*val_size = -1;
		return NULL;
	}
	basket_validate_zhash(basket);
	return zhash_find_by_str(_basket->zhash, key_str, key_str_len, val_size);
}

void *basket_keyval_extract_by_in64(void *basket, uint64_t key_int64, ssize_t *size)
{
	TESTP(basket, NULL);
	TESTP(size, NULL);

	basket_t *_basket = basket;
	if (NULL == _basket->zhash) {
		*size = -1;
		return NULL;
	}
	basket_validate_zhash(basket);
	return zhash_extract_by_int(_basket->zhash, key_int64, size);
}

void *basket_keyval_extract_by_str(void *basket, char *key_str, size_t key_str_len, ssize_t *size)
{
	TESTP(basket, NULL);
	TESTP(key_str, NULL);
	TESTP(size, NULL);

	basket_t *_basket = basket;
	if (NULL == _basket->zhash) {
		*size = -1;
		return NULL;
	}
	basket_validate_zhash(basket);
	return zhash_extract_by_str(_basket->zhash, key_str, key_str_len, size);
}

