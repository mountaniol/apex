#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <netdb.h>
#include <errno.h>
#include <stddef.h>
#include <linux/errno.h>

#include "buf_t.h"

#include "buf_t_debug.h"
#include "buf_t_memory.h"
#include "tests.h"

/* Syntax Note:
 *
 * *** General ***
 *
 * A function name composed of:
 * 1. Module name: "buft", "mbuf" or something else.
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
 * Let say, the "buf->used" value is 7 in the beginning of each examples below:
 *
 * 1. buf_used_take(buf) will return 7
 * 2. buf_used_set(buf, 12) - after this the "buf->used" == 12
 * 3. buf_used_inc(buf, 2) - after this the "buf->used" == 9
 * 4. buf_used_dec(buf, 2) - after this the "buf->used" == 5
 *
 * Of course, one could misuse it. For example:
 * 1. buf_used_inc(buf, -2) - after this the "buf->used" == 5
 * 2. buf_used_dec(buf, -3) - after this the "buf->used" == 10
 *
 * These two examples will work, they are mathematically correct,
 * however it make the code undestanding harder.
 *
 */

/*** Get and Set value of 'used' field in the buf_t ***/

buf_s64_t buf_used_take(buf_t *buf)
{
	T_RET_ABORT(buf, -EINVAL);
	return (buf->used);
}

void buf_used_set(buf_t *buf, buf_s64_t used)
{
	T_RET_ABORT(buf, -EINVAL);
	buf->used = used;
}

void buf_used_inc(buf_t *buf, buf_s64_t inc)
{
	buf->used += inc;
}

void buf_used_dec(buf_t *buf, buf_s64_t dec)
{
	T_RET_ABORT(buf, -EINVAL);

	if (dec > buf_used_take(buf)) {
		TRY_ABORT();
	}

	if (buf->used >= dec) {
		buf->used -= dec;
	} else {
		TRY_ABORT();
		buf->used = 0;
	}
}

/* Validate sanity of buf_t - common for all buffers */
ret_t buf_is_valid(buf_t *buf)
{
	T_RET_ABORT(buf, -EINVAL);

	/* buf->used always <= buf->room */
	/* TODO: not in case of CIRC buffer */
	if (buf_used_take(buf) > buf_room_take(buf)) {
		DE("Invalid buf: buf->used > buf->room\n");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/* The buf->data may be NULL if and only if both buf->used and buf->room == 0; However, we don't
	   check buf->used: we tested that it <= buf->room already */
	if ((NULL == buf->data) && (buf_room_take(buf) > 0)) {
		DE("Invalid buf: buf->data == NULL but buf->room > 0 (%ld)\n", buf_room_take(buf));
		TRY_ABORT();
		return (-ECANCELED);
	}

	/* And vice versa: if buf->data != NULL the buf->room must be > 0 */
	if ((NULL != buf->data) && (0 == buf_room_take(buf))) {
		DE("Invalid buf: buf->data != NULL but buf->room == 0\n");
		TRY_ABORT();
		return (-ECANCELED);
	}

	DDD0("Buffer is valid\n");
	return (OK);
}

buf_t *buf_new(buf_s64_t size)
{
	buf_t  *buf;

	/* The real size of allocated  buffer can be more than used asked,
	   the canary and the checksum space could be added */
	size_t real_size = size;

	buf = (buf_t *) zmalloc(sizeof(buf_t));
	T_RET_ABORT(buf, NULL);

	/* If a size is given than allocate a data */
	if (size > 0) {

		buf->data = (char *) zmalloc(real_size);
		TESTP_ASSERT(buf->data, "Can't allocate buf->data");
	}

	/* Assigned value to buf->room field */
	buf_room_set(buf, size);

	/* Assigned value to buf->used field */
	buf_used_set(buf, 0);

	/* Just in case, check that the buf_t looks valid */
	if (OK != buf_is_valid(buf)) {
		DE("Buffer is invalid right after allocation!\n");
		if (OK != buf_free(buf)) {
			DE("Can not free the buffer\n");
		}
		TRY_ABORT();
		return (NULL);
	}

	return (buf);
}

ret_t buf_data_set(buf_t *buf, char *data, const buf_s64_t size, const buf_s64_t len)
{
	T_RET_ABORT(buf, -EINVAL);
	T_RET_ABORT(data, -EINVAL);

	buf->data = data;
	buf_room_set(buf, size);
	buf_used_set(buf, len);
	return (OK);
}

void *buf_data_steal(buf_t *buf)
{
	void *data;
	T_RET_ABORT(buf, NULL);
	data = buf->data;
	buf->data = NULL;
	buf_room_set(buf, 0);
	buf_used_set(buf, 0);
	return (data);
}

void *buf_data_steal_and_release(buf_t *buf)
{
	void *data;
	T_RET_ABORT(buf, NULL);
	data = buf_data_steal(buf);
	if (OK != buf_free(buf)) {
		DE("Warning! Memory leak: can't clean buf_t!");
		TRY_ABORT();
	}
	return (data);
}

void *buf_data_take(buf_t *buf)
{
	T_RET_ABORT(buf, NULL);
	return (buf->data);
}

ret_t buf_is_data_null(buf_t *buf)
{
	T_RET_ABORT(buf, -EINVAL);
	if (NULL == buf->data) {
		return YES;
	}
	return NO;
}

/* This is an internal function. Here we realloc the internal buf_t buffer */
static ret_t buf_realloc(buf_t *buf, size_t new_size)
{
	void   *tmp = realloc(buf->data, new_size);

	/* Case 1: realloc can't reallocate */
	if (NULL == tmp) {
		DE("Realloc failed\n");
		return (-ENOMEM);
	}

	/* Case 2: realloc succeeded, new memory returned */
	/* No need to free the old memory - done by realloc */
	if (buf->data != tmp) {
		free(buf->data);
		buf->data = (char *) tmp;
	}
	return OK;
}

ret_t buf_room_add_memory(buf_t *buf, buf_s64_t size)
{
	T_RET_ABORT(buf, -EINVAL);

	if (0 == size) {
		DE("Bad arguments: buf == NULL (%p) or size == 0 (%ld)\b", buf, size);
		TRY_ABORT();
		return (-EINVAL);
	}

	if (OK != buf_realloc(buf, buf_room_take(buf) + size)) {
		DE("Can not reallocate buf->data\n");
		return (-ENOMEM);
	}

	/* Clean newely allocated memory */
	memset(buf->data + buf_room_take(buf), 0, size);

	/* Case 3: realloc succeeded, the same pointer - we do nothing */
	/* <Beeep> */

	/* Increase buf->room */
	buf_room_inc(buf, size);

	BUF_TEST(buf);
	return (OK);
}

/* Return how much bytes is available in the buf_t */
buf_s64_t buf_room_avaialable_take(buf_t *buf)
{
	T_RET_ABORT(buf, -EINVAL);
	return buf_room_take(buf) - buf_used_take(buf);
}

ret_t buf_room_assure(buf_t *buf, buf_s64_t expect)
{
	buf_s64_t size_to_add;
	T_RET_ABORT(buf, -EINVAL);

	/* It is meanless to pass expected == 0 */
	if (expect == 0) {
		DE("'expected' size == 0\n");
		TRY_ABORT();
		return (-EINVAL);
	}

	/* If we have enough room, return OK */
	if (buf_room_avaialable_take(buf) >= expect) {
		return (OK);
	}

	/* There is not enough room to place 'expected' bytes;
	   increase the buffer to be enough for 'expected' buffer */
	size_to_add = expect - buf_room_avaialable_take(buf);
	return (buf_room_add_memory(buf, size_to_add));
}

ret_t buf_clean_and_reset(buf_t *buf)
{
	T_RET_ABORT(buf, -EINVAL);

	if (OK != buf_is_valid(buf)) {
		DE("Warning: buffer is invalid\n");
	}

	if (buf->data) {
		/* Security: zero memory before it freed */
		memset(buf->data, 0, buf_room_take(buf));
		free(buf->data);
	}

	memset(buf, 0, sizeof(buf_t));
	return (OK);
}

ret_t buf_free(buf_t *buf)
{
	T_RET_ABORT(buf, -EINVAL);

	/* Just in case, test that the buf_t is valid */
	if (OK != buf_is_valid(buf)) {
		DE("Warning: buffer is invalid\n");
	}

	/* If there's an internal buffer, release it */
	if (NULL != buf->data) {
		TFREE_SIZE(buf->data, buf_used_take(buf));
	}
	/* Release the buf_t struct */
	TFREE_SIZE(buf, sizeof(buf_t));
	return (OK);
}
/* Copy the given buffer "new_data" at tail of the buf_t */
ret_t buf_add(buf_t *buf /* Buf_t to add into */,
			  const char *new_data /* Buffer to add */,
			  const buf_s64_t size /* Size of the buffer to add */)
{
	TESTP_ASSERT(buf, "buf is NULL");
	TESTP_ASSERT(new_data, "new_data is NULL");

	/* We can not add buffer of 0 bytes or less; probably , if we here it is a bug */
	if (size < 1) {
		DE("Wrong argument(s): b = %p, buf = %p, size = %ld\n", buf, new_data, size);
		TRY_ABORT();
		return (-EINVAL);
	}

	/* Assure that we have enough room to add this buffer */
	if (0 != buf_room_assure(buf, size)) {
		DE("Can't add room into buf_t\n");
		TRY_ABORT();
		return (-ENOMEM);
	}

	/* And now we are adding the buffer at the tail */
	memcpy(buf->data + buf_used_take(buf), new_data, size);
	/* Increase the buf->used */
	buf_used_inc(buf, size);

	BUF_TEST(buf);
	return (OK);
}

buf_s64_t buf_room_take(buf_t *buf)
{
	T_RET_ABORT(buf, -EINVAL);
	return (buf->room);
}

void buf_room_set(buf_t *buf, buf_s64_t room)
{
	TESTP_ASSERT (buf, "buf is NULL");
	buf->room = room;
}

void buf_room_inc(buf_t *buf, buf_s64_t inc)
{
	TESTP_ASSERT (buf, "buf is NULL");
	buf->room += inc;
}

void buf_room_dec(buf_t *buf, buf_s64_t dec)
{
	TESTP_ASSERT (buf, "buf is NULL");
	if (dec > buf_room_take(buf)) {
		DE("Asked to decrement buf->room to a negative value");
		TRY_ABORT();
	}

	if (buf->room >= dec) {
		buf->room -= dec;
	} else {
		buf->room = 0;
	}
}

ret_t buf_pack(buf_t *buf)
{
	T_RET_ABORT(buf, -EINVAL);

	/*** If buffer is empty we have nothing to do */

	if (NULL == buf->data) {
		return (OK);
	}

	/*** Sanity check: dont' process invalide buffer */

	if (OK != buf_is_valid(buf)) {
		DE("Buffer is invalid - can't proceed\n");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/*** Should we really pack it? */
	if (buf_used_take(buf) == buf_room_take(buf)) {
		/* No need to pack it */
		return (OK);
	}

	/* Here shrink the buffer */
	if (OK != buf_realloc(buf, buf_used_take(buf))) {
		DE("Can not realloc buf->data\n");
		return (BAD);
	}

	buf_room_set(buf, buf_used_take(buf));

	/* Here we are if buf->used == buf->room */
	BUF_TEST(buf);
	return (OK);
}
