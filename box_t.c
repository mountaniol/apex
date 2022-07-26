#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <netdb.h>
#include <errno.h>
#include <stddef.h>
#include <linux/errno.h>

#include "box_t.h"

#include "box_t_debug.h"
#include "box_t_memory.h"
#include "tests.h"

/* Syntax Note:
 *
 * *** General ***
 *
 * A function name composed of:
 * 1. Module name: "box", "basket" or something else.
 * 2. Field name (if related to a specific field): " "_used", "_room"
 * 3. Action name (if applicable): "_set", "_take" etc.
 *
 * *** Sufixes ***
 *
 * Four sufixes are used in the code:
 * "_take" - read value; used instead "_get" to avoid confusion: the "_get" often means a lock operation.
 * "_set" - set a new value, ignoring the old value.
 * "_inc" - increase existing value by new value.
 * "_dec" - decrease existing value by new value.
 *
 * To avoud any misunderstanding, here are examples.
 * Let say, the "box->used" value is 7 in the beginning of each examples below:
 *
 * 1. box_used_take(box) will return 7
 * 2. box_used_set(box, 12) - after this the "box->used" == 12
 * 3. box_used_inc(box, 2) - after this the "box->used" == 9
 * 4. box_used_dec(box, 2) - after this the "box->used" == 5
 *
 * Of course, one could misuse it. For example:
 * 1. box_used_inc(box, -2) - after this the "box->used" == 5
 * 2. box_used_dec(box, -3) - after this the "box->used" == 10
 *
 * These two examples will work, they are mathematically correct,
 * however it make the code undestanding harder.
 *
 */

#ifdef ENABLE_BOX_DUMP
void box_dump(const box_t *box, const char *mes)
{
	TESTP_ABORT(box);
	TESTP_ABORT(mes);

	if (NULL == mes) {
		MES_ABORT("Got mes == NULL");
	}

	DD("box_t dump:           %s\n", mes);
	DD("========================\n");
	DD("box_t ptr:            %p\n", box);
	if (NULL == box) {
		return;
	}

	DD("box_t data ptr:       %p\n", box_data_take(box));
	DD("box_t used:           %ld\n", box_used_take(box));
	DD("box_t room:           %ld\n", box_room_take(box));
	DD("========================\n");
}
#else
void box_dump(__attribute__((unused)) const box_t *box, __attribute__((unused)) const char *mes)
{
}
#endif /* BOX_DUMP */

/*** Get and Set value of 'used' field in the box_t ***/

box_s64_t box_used_take(const box_t *box)
{
	TESTP_ABORT(box);
	return (box->used);
}

void box_used_set(box_t *box, box_s64_t used)
{
	TESTP_ABORT(box);
	box->used = used;
}

void box_used_inc(box_t *box, box_s64_t inc)
{
	TESTP_ABORT(box);
	box->used += inc;
}

void box_used_dec(box_t *box, box_s64_t dec)
{
	TESTP_ABORT(box);
	if (dec > box_used_take(box)) {
		DE("Tried to decrement 'box->used' such that it will be < 0: current %zu, asked decrement %zu\n",
		   box->room, dec);
		TRY_ABORT();
	}

	if (box->used >= dec) {
		box->used -= dec;
	} else {
		DE("Tried to decrement 'box->used' such that it will be < 0: current %zu, asked decrement %zu\n",
		   box->room, dec);
		TRY_ABORT();
		box->used = 0;
	}
}

/* Validate sanity of box_t - common for all boxes */
ret_t box_is_valid(const box_t *box, const char *who, const int line)
{
	TESTP_ABORT(box);

	/* box->used always <= box->room */
	if (box_used_take(box) > box_room_take(box)) {
		DE("/%s +%d/ : Invalid box: box->used (%ld) > box->room (%ld)\n",
		   who, line, box_used_take(box), box_room_take(box));
		box_dump(box, "from box_is_valid(), before terminating 1");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/* The box->data may be NULL if and only if both box->used and box->room == 0; However, we don't
	   check box->used: we tested that it <= box->room already */
	if ((NULL == box->data) && (box_room_take(box) > 0)) {
		DE("/%s +%d/ : Invalid box: box->data == NULL but box->room > 0 (%ld) / box->used (%ld)\n",
		   who, line, box_room_take(box), box_used_take(box));
		box_dump(box, "from box_is_valid(), before terminating 2");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/* And vice versa: if box->data != NULL the box->room must be > 0 */
	if ((NULL != box->data) && (0 == box_room_take(box))) {
		DE("/%s +%d/: Invalid box: box->data != NULL but box->room == 0\n", who, line);
		box_dump(box, "from box_is_valid(), before terminating 3");
		TRY_ABORT();
		return (-ECANCELED);
	}

	DDD0("/%s +%d/: Box is valid\n", who, line);
	return (OK);
}

box_t *box_new(box_s64_t size)
{
	box_t  *box;
	size_t real_size;

	if (size < 0) {
		ABORT_OR_RETURN(NULL);
	}

	real_size = size;

	box = (box_t *)zmalloc(sizeof(box_t));
	T_RET_ABORT(box, NULL);

	/* If a size is given than allocate a data */
	if (size > 0) {

		box->data = (char *)zmalloc(real_size);
		TESTP_ASSERT(box->data, "Can't allocate box->data");
	}

	/* Assigned value to box->room field */
	box_room_set(box, size);

	/* Assigned value to box->used field */
	box_used_set(box, 0);

	/* Just in case, check that the box_t looks valid */
	if (OK != box_is_valid(box, __func__, __LINE__)) {
		DE("Box is invalid right after allocation!\n");
		if (OK != box_free(box)) {
			DE("Can not free the box\n");
		}
		TRY_ABORT();
		return (NULL);
	}

	return (box);
}

ret_t box_data_set(box_t *box, char *data, const box_s64_t size, const box_s64_t len)
{
	TESTP_ABORT(box);
	TESTP_ABORT(data);

	box->data = data;
	box_room_set(box, size);
	box_used_set(box, len);
	return (OK);
}

void *box_data_steal(box_t *box)
{
	void *data;
	TESTP_ABORT(box);
	data = box->data;
	box->data = NULL;
	box_room_set(box, 0);
	box_used_set(box, 0);
	return (data);
}

void *box_data_steal_and_release(box_t *box)
{
	void *data;
	TESTP_ABORT(box);
	data = box_data_steal(box);
	if (OK != box_free(box)) {
		DE("Warning! Memory leak: can't clean box_t!");
		TRY_ABORT();
	}
	return (data);
}

void *box_data_take(const box_t *box)
{
	TESTP_ABORT(box);
	return (box->data);
}

ret_t box_is_data_null(box_t *box)
{
	TESTP_ABORT(box);
	if (NULL == box->data) {
		return YES;
	}
	return NO;
}

/* This is an internal function. Here we realloc the internal box_t buffer */
static ret_t box_realloc(box_t *box, size_t new_size)
{
	size_t room         = 0;
	size_t size_to_copy = 0;
	void   *tmp         = NULL;

	TESTP_ABORT(box);

	room         = (size_t)box->room;
	size_to_copy = MIN(room, new_size);

	// box_dump(box, "in Box Realloc, before");

	//DDD("Going to allocate new buffer %zu size; room = %zu, size_to_copy = %zu\n", new_size, room, size_to_copy);
	tmp = malloc(new_size);
	//DDD("Allocated size %zu\n", new_size);

	if (NULL == tmp) {
		DE("New memory alloc failed: current size = %zu, asked size = %zu\n", box->room, new_size);
		ABORT_OR_RETURN(-ENOMEM);
	}

	/* The new size can be less than previous; so we use minimal between bif->room and new_size to copy data */
	if (box->data) {
		memcpy(tmp, box->data, size_to_copy);
		free(box->data);
		box->data = NULL;
	}

	box->data = tmp;
	return OK;
}

/* This is an internal function. Here we realloc the internal box_t buffer */
#if 0 /* SEB */
static ret_t box_realloc_old(box_t *box, size_t new_size){
	void   *tmp;
	TESTP_ABORT(box);
	tmp = realloc(box->data, new_size);

	/* Case 1: realloc can't reallocate */
	if (NULL == tmp) {
		DE("Realloc failed: current size = %zu, asked size = %zu\n",
		   box->room, new_size);
		TRY_ABORT();
		return (-ENOMEM);
	}

	/* Case 2: realloc succeeded, new memory returned */
	/* No need to free the old memory - done by realloc */
	if (box->data != tmp) {
		free(box->data);
		box->data = (char *)tmp;
	}
	return OK;
}
#endif

ret_t box_room_add_memory(box_t *box, box_s64_t sz)
{
	size_t original_room_size;
	TESTP_ABORT(box);

	box_dump(box, "box_room_add_memory(): Before adding memory");
	if (0 == sz) {
		DE("Bad arguments: box == NULL (%p) or size == 0 (%ld)\b", box, sz);
		ABORT_OR_RETURN(-EINVAL);
	}

	/* Save the */
	original_room_size = box_room_take(box);

	if (OK != box_realloc(box, original_room_size + sz)) {
		DE("Can not reallocate box->data\n");
		ABORT_OR_RETURN(-ENOMEM);
	}

	box_dump(box, "box_room_add_memory(): After adding memory");

	/* Clean newely allocated memory */
	memset(box->data + original_room_size, 0, sz);

	box_dump(box, "box_room_add_memory(): After cleaning new memory");

	/* Case 3: realloc succeeded, the same pointer - we do nothing */
	/* <Beeep> */

	/* Increase box->room */
	box_room_inc(box, sz);
	// box_dump(box, "After adding memory");

	BOX_TEST(box);
	return (OK);
}

/* Return how much bytes is available in the box_t */
box_s64_t box_room_avaialable_take(box_t *box)
{
	box_s64_t m_used = -1;
	box_s64_t m_room = -1;
	TESTP_ABORT(box);
	m_used = box_used_take(box);
	if (m_used < 0) {
		MES_ABORT("box_used_take(box) returned a value < 0");
	}
	m_room = box_room_take(box);
	if (m_room < 0) {
		MES_ABORT("box_room_take(box) returned a value < 0");
	}

	if (m_room < m_used) {
		DE("A bug: box->room (%ld) can not be < box->used (%ld) ; probably memory coruption or misuse\n",
		   m_room, m_used);
	}
	return m_room - m_used;
}

ret_t box_room_assure(box_t *box, box_s64_t expect)
{
	TESTP_ABORT(box);

	/* It is meanless to pass expected == 0 */
	if (expect < 1) {
		DE("'expected' size <= 0 : %ld\n", expect);
		TRY_ABORT();
		return (-EINVAL);
	}

	/* If we have enough room, return OK */
	if (box_room_avaialable_take(box) >= expect) {
		return (OK);
	}

	return (box_room_add_memory(box, expect));
}

ret_t box_clean_and_reset(box_t *box)
{
	TESTP_ABORT(box);

	if (OK != box_is_valid(box, __func__, __LINE__)) {
		DE("Warning: box is invalid\n");
	}

	if (box->data) {
		/* Security: zero memory before it freed */
		DDD("Cleaning before free, data %p, size %ld\n", box->data, box_room_take(box));
		memset(box->data, 0, box_room_take(box));
		free(box->data);
		box->data = NULL;
	}

	memset(box, 0, sizeof(box_t));
	return (OK);
}

ret_t box_free(box_t *box)
{
	TESTP_ABORT(box);

	/* Just in case, test that the box_t is valid */
	if (OK != box_is_valid(box, __func__, __LINE__)) {
		DE("Warning: box is invalid\n");
	}

	/* If there's an internal buffer, release it */
	if (NULL != box->data) {
		TFREE_SIZE(box->data, box_used_take(box));
	}
	/* Release the box_t struct */
	TFREE_SIZE(box, sizeof(box_t));
	return (OK);
}

/* Copy the given buffer "new_data" at tail of the buf_t */
ret_t box_add(box_t *box /* box_t to add into */,
			  const char *new_data /* Buffer to add */,
			  const box_s64_t sz /* Size of the buffer to add */)
{
	TESTP_ABORT(box);
	TESTP_ABORT(new_data);

	/* We can not add buffer of 0 bytes or less; probably , if we here it is a bug */
	if (sz < 1) {
		DE("Wrong argument(s): box = %p, buf = %p, size = %ld\n", box, new_data, sz);
		ABORT_OR_RETURN(-EINVAL);
	}

	/* Assure that we have enough room to add this buffer */
	if (OK != box_room_assure(box, sz)) {
		DE("Can't add room into box_t\n");
		ABORT_OR_RETURN(-ENOMEM);
	}

	box_dump(box, "From buf_add");

	/* And now we are adding the buffer at the tail */
	memcpy(box->data + box_used_take(box), new_data, sz);
	/* Increase the box->used */
	box_used_inc(box, sz);

	BOX_TEST(box);
	return (OK);
}

ret_t box_merge(box_t *dst, box_t *src)
{
	ret_t rc;
	TESTP_ABORT(dst);
	TESTP_ABORT(src);
	rc = box_add(dst, src->data, src->used);

	if (OK != rc) {
		DE("The box_add() failed\n");
		ABORT_OR_RETURN(rc);
	}

	if (OK != box_clean_and_reset(src)) {
		DE("Could not clean 'src' box\n");
		ABORT_OR_RETURN(-ECANCELED);
	}
	return rc;
}

/* Replace the internal box buffer with the buffer "new_data" */
ret_t box_replace(box_t *box /* box_t to replace data in */,
				  const char *new_data /* Buffer to copy into the box_t */,
				  const box_s64_t size /* Size of the new buffer to set */)
{
	box_s64_t current_room_size;

	/* NOTE: This function is not dedicated to reset the box_t:
	 * It means, this function does not accept new_data == NULL + size == 0.
	   If one needs to reset the box_t, there is a dedicated funtion for this task */
	TESTP_ABORT(box);
	TESTP_ABORT(new_data);

	/* We can not add buffer of 0 bytes or less; probably , if we here it is a bug */
	if (size < 1) {
		DE("Wrong argument(s): box = %p, buf = %p, size = %ld\n", box, new_data, size);
		TRY_ABORT();
		return (-EINVAL);
	}

	/* We use this variable to minimize number of calls of box_room_take(box) */
	current_room_size = box_room_take(box);

	/* Assure that we have enough room to set the new buffer */
	if (size > current_room_size &&  /* If size of the new buffer bigger than current buffer */
		OK != box_room_assure(box, size - current_room_size)) { /* And if we could not allocated additional memory */
		DE("Can't add room into box_t\n");
		TRY_ABORT();
		return (-ENOMEM);
	}

	/* And now we are adding the buffer at the tail */
	memcpy(box->data, new_data, size);

	/* Set the new box->used */
	box_used_set(box, size);

	BOX_TEST(box);
	return (OK);
}

box_s64_t box_room_take(const box_t *box)
{
	TESTP_ABORT(box);
	return (box->room);
}

void box_room_set(box_t *box, box_s64_t room)
{
	TESTP_ABORT(box);
	DDD("Setting box room: %ld\n", room);
	box->room = room;
}

void box_room_inc(box_t *box, box_s64_t inc)
{
	TESTP_ABORT(box);
	DDD("Inc box room: %ld + %ld\n", box->room, inc);
	box->room += inc;
}

void box_room_dec(box_t *box, box_s64_t dec)
{
	TESTP_ABORT(box);
	if (dec > box_room_take(box)) {
		DE("Asked to decrement box->room to a negative value");
		TRY_ABORT();
	}

	DDD("Dec box room: %ld - %ld\n", box->room, dec);

	if (box->room >= dec) {
		box->room -= dec;
	} else {
		box->room = 0;
	}
}

ret_t box_pack(box_t *box)
{
	TESTP_ABORT(box);

	/*** If box is empty we have nothing to do */

	if (NULL == box->data) {
		return (OK);
	}

	/*** Sanity check: dont' process invalide box */

	if (OK != box_is_valid(box, __func__, __LINE__)) {
		DE("Box is invalid - can't proceed\n");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/*** Should we really pack it? */
	if (box_used_take(box) == box_room_take(box)) {
		/* No need to pack it */
		return (OK);
	}

	/* Here shrink the box */
	if (OK != box_realloc(box, box_used_take(box))) {
		DE("Can not realloc box->data\n");
		return (BAD);
	}

	box_room_set(box, box_used_take(box));

	/* Here we are if box->used == box->room */
	BOX_TEST(box);
	return (OK);
}


