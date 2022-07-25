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

#ifdef ENABLE_BUF_DUMP
void box_dump(const box_t *box, const char *mes)
{
	TESTP_ABORT(box);
	TESTP_ABORT(mes);

	if (NULL == mes) {
		MES_ABORT("Got mes == NULL");
	}

	DD("Buf_t dump:           %s\n", mes);
	DD("========================\n");
	DD("Buf_t ptr:            %p\n", box);
	if (NULL == box) {
		return;
	}

	DD("Buf_t data ptr:       %p\n", box_data_take(box));
	DD("Buf_t used:           %ld\n", box_used_take(box));
	DD("Buf_t room:           %ld\n", box_room_take(box));
	DD("========================\n");
}
#else
void box_dump(__attribute__((unused)) const box_t *buf, __attribute__((unused)) const char *mes)
{
}
#endif /* BUF_DUMP */

/*** Get and Set value of 'used' field in the buf_t ***/

box_s64_t box_used_take(const box_t *buf)
{
	TESTP_ABORT(buf);
	return (buf->used);
}

void box_used_set(box_t *buf, box_s64_t used)
{
	TESTP_ABORT(buf);
	buf->used = used;
}

void box_used_inc(box_t *buf, box_s64_t inc)
{
	TESTP_ABORT(buf);
	buf->used += inc;
}

void box_used_dec(box_t *buf, box_s64_t dec)
{
	TESTP_ABORT(buf);
	if (dec > box_used_take(buf)) {
		DE("Tried to decrement 'box->used' such that it will be < 0: current %zu, asked decrement %zu\n",
		   buf->room, dec);
		TRY_ABORT();
	}

	if (buf->used >= dec) {
		buf->used -= dec;
	} else {
		DE("Tried to decrement 'box->used' such that it will be < 0: current %zu, asked decrement %zu\n",
		   buf->room, dec);
		TRY_ABORT();
		buf->used = 0;
	}
}

/* Validate sanity of buf_t - common for all buffers */
ret_t box_is_valid(const box_t *buf, const char *who, const int line)
{
	TESTP_ABORT(buf);

	/* buf->used always <= buf->room */
	/* TODO: not in case of CIRC buffer */
	if (box_used_take(buf) > box_room_take(buf)) {
		DE("/%s +%d/ : Invalid buf: buf->used (%ld) > buf->room (%ld)\n",
		   who, line, box_used_take(buf), box_room_take(buf));
		box_dump(buf, "from buf_is_valid(), before terminating 1");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/* The buf->data may be NULL if and only if both buf->used and buf->room == 0; However, we don't
	   check buf->used: we tested that it <= buf->room already */
	if ((NULL == buf->data) && (box_room_take(buf) > 0)) {
		DE("/%s +%d/ : Invalid buf: buf->data == NULL but buf->room > 0 (%ld) / buf->used (%ld)\n",
		   who, line, box_room_take(buf), box_used_take(buf));
		box_dump(buf, "from buf_is_valid(), before terminating 2");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/* And vice versa: if buf->data != NULL the buf->room must be > 0 */
	if ((NULL != buf->data) && (0 == box_room_take(buf))) {
		DE("/%s +%d/: Invalid buf: buf->data != NULL but buf->room == 0\n", who, line);
		box_dump(buf, "from buf_is_valid(), before terminating 3");
		TRY_ABORT();
		return (-ECANCELED);
	}

	DDD0("/%s +%d/: Buffer is valid\n", who, line);
	return (OK);
}

box_t *box_new(box_s64_t size)
{
	box_t  *buf;

	/* The real size of allocated  buffer can be more than used asked,
	   the canary and the checksum space could be added */
	size_t real_size = size;

	buf = (box_t *)zmalloc(sizeof(box_t));
	T_RET_ABORT(buf, NULL);

	/* If a size is given than allocate a data */
	if (size > 0) {

		buf->data = (char *)zmalloc(real_size);
		TESTP_ASSERT(buf->data, "Can't allocate buf->data");
	}

	/* Assigned value to buf->room field */
	box_room_set(buf, size);

	/* Assigned value to buf->used field */
	box_used_set(buf, 0);

	/* Just in case, check that the buf_t looks valid */
	if (OK != box_is_valid(buf, __func__, __LINE__)) {
		DE("Buffer is invalid right after allocation!\n");
		if (OK != box_free(buf)) {
			DE("Can not free the buffer\n");
		}
		TRY_ABORT();
		return (NULL);
	}

	return (buf);
}

ret_t box_data_set(box_t *buf, char *data, const box_s64_t size, const box_s64_t len)
{
	TESTP_ABORT(buf);
	TESTP_ABORT(data);

	buf->data = data;
	box_room_set(buf, size);
	box_used_set(buf, len);
	return (OK);
}

void *box_data_steal(box_t *buf)
{
	void *data;
	TESTP_ABORT(buf);
	data = buf->data;
	buf->data = NULL;
	box_room_set(buf, 0);
	box_used_set(buf, 0);
	return (data);
}

void *box_data_steal_and_release(box_t *buf)
{
	void *data;
	TESTP_ABORT(buf);
	data = box_data_steal(buf);
	if (OK != box_free(buf)) {
		DE("Warning! Memory leak: can't clean buf_t!");
		TRY_ABORT();
	}
	return (data);
}

void *box_data_take(const box_t *buf)
{
	TESTP_ABORT(buf);
	return (buf->data);
}

ret_t box_is_data_null(box_t *buf)
{
	TESTP_ABORT(buf);
	if (NULL == buf->data) {
		return YES;
	}
	return NO;
}

/* This is an internal function. Here we realloc the internal buf_t buffer */
static ret_t box_realloc(box_t *buf, size_t new_size)
{
	size_t room         = 0;
	size_t size_to_copy = 0;
	void   *tmp         = NULL;

	TESTP_ABORT(buf);

	room         = (size_t)buf->room;
	size_to_copy = MIN(room, new_size);

	// buf_dump(buf, "in Buf Realloc, before");

	//DDD("Going to allocate new buffer %zu size; room = %zu, size_to_copy = %zu\n", new_size, room, size_to_copy);
	tmp = malloc(new_size);
	//DDD("Allocated size %zu\n", new_size);

	if (NULL == tmp) {
		DE("New memory alloc failed: current size = %zu, asked size = %zu\n", buf->room, new_size);
		ABORT_OR_RETURN(-ENOMEM);
	}

	/* The new size can be less than previous; so we use minimal between bif->room and new_size to copy data */
	if (buf->data) {
		memcpy(tmp, buf->data, size_to_copy);
		free(buf->data);
		buf->data = NULL;
	}

	buf->data = tmp;
	return OK;
}

/* This is an internal function. Here we realloc the internal buf_t buffer */
static ret_t box_realloc_old(box_t *buf, size_t new_size)
{
	void   *tmp;
	TESTP_ABORT(buf);
	tmp = realloc(buf->data, new_size);

	/* Case 1: realloc can't reallocate */
	if (NULL == tmp) {
		DE("Realloc failed: current size = %zu, asked size = %zu\n",
		   buf->room, new_size);
		TRY_ABORT();
		return (-ENOMEM);
	}

	/* Case 2: realloc succeeded, new memory returned */
	/* No need to free the old memory - done by realloc */
	if (buf->data != tmp) {
		free(buf->data);
		buf->data = (char *)tmp;
	}
	return OK;
}

ret_t box_room_add_memory(box_t *buf, box_s64_t sz)
{
	size_t original_room_size;
	TESTP_ABORT(buf);

	box_dump(buf, "buf_room_add_memory(): Before adding memory");
	if (0 == sz) {
		DE("Bad arguments: buf == NULL (%p) or size == 0 (%ld)\b", buf, sz);
		ABORT_OR_RETURN(-EINVAL);
	}

	/* Save the */
	original_room_size = box_room_take(buf);

	if (OK != box_realloc(buf, original_room_size + sz)) {
		DE("Can not reallocate buf->data\n");
		ABORT_OR_RETURN(-ENOMEM);
	}

	box_dump(buf, "buf_room_add_memory(): After adding memory");

	/* Clean newely allocated memory */
	memset(buf->data + original_room_size, 0, sz);

	box_dump(buf, "buf_room_add_memory(): After cleaning new memory");

	/* Case 3: realloc succeeded, the same pointer - we do nothing */
	/* <Beeep> */

	/* Increase buf->room */
	box_room_inc(buf, sz);
	// buf_dump(buf, "After adding memory");

	BUF_TEST(buf);
	return (OK);
}

/* Return how much bytes is available in the buf_t */
box_s64_t box_room_avaialable_take(box_t *buf)
{
	box_s64_t m_used = -1;
	box_s64_t m_room = -1;
	TESTP_ABORT(buf);
	m_used = box_used_take(buf);
	if (m_used < 0) {
		MES_ABORT("buf_used_take(buf) returned a value < 0");
	}
	m_room = box_room_take(buf);
	if (m_room < 0) {
		MES_ABORT("buf_room_take(buf) returned a value < 0");
	}

	if (m_room < m_used) {
		DE("A bug: buf->room (%ld) can not be < buf->used (%ld) ; probably memory coruption or misuse\n",
		   m_room, m_used);
	}
	return m_room - m_used;
}

ret_t box_room_assure(box_t *buf, box_s64_t expect)
{
	TESTP_ABORT(buf);

	/* It is meanless to pass expected == 0 */
	if (expect < 1) {
		DE("'expected' size <= 0 : %ld\n", expect);
		TRY_ABORT();
		return (-EINVAL);
	}

	/* If we have enough room, return OK */
	if (box_room_avaialable_take(buf) >= expect) {
		return (OK);
	}

	return (box_room_add_memory(buf, expect));
}

ret_t box_clean_and_reset(box_t *buf)
{
	TESTP_ABORT(buf);

	if (OK != box_is_valid(buf, __func__, __LINE__)) {
		DE("Warning: buffer is invalid\n");
	}

	if (buf->data) {
		/* Security: zero memory before it freed */
		DDD("Cleaning before free, data %p, size %ld\n", buf->data, box_room_take(buf));
		memset(buf->data, 0, box_room_take(buf));
		free(buf->data);
		buf->data = NULL;
	}

	memset(buf, 0, sizeof(box_t));
	return (OK);
}

ret_t box_free(box_t *buf)
{
	TESTP_ABORT(buf);

	/* Just in case, test that the buf_t is valid */
	if (OK != box_is_valid(buf, __func__, __LINE__)) {
		DE("Warning: buffer is invalid\n");
	}

	/* If there's an internal buffer, release it */
	if (NULL != buf->data) {
		TFREE_SIZE(buf->data, box_used_take(buf));
	}
	/* Release the buf_t struct */
	TFREE_SIZE(buf, sizeof(box_t));
	return (OK);
}

/* Copy the given buffer "new_data" at tail of the buf_t */
ret_t box_add(box_t *buf /* Buf_t to add into */,
			  const char *new_data /* Buffer to add */,
			  const box_s64_t sz /* Size of the buffer to add */)
{
	TESTP_ABORT(buf);
	TESTP_ABORT(new_data);

	/* We can not add buffer of 0 bytes or less; probably , if we here it is a bug */
	if (sz < 1) {
		DE("Wrong argument(s): b = %p, buf = %p, size = %ld\n", buf, new_data, sz);
		ABORT_OR_RETURN(-EINVAL);
	}

	/* Assure that we have enough room to add this buffer */
	if (OK != box_room_assure(buf, sz)) {
		DE("Can't add room into buf_t\n");
		ABORT_OR_RETURN(-ENOMEM);
	}

	/* And now we are adding the buffer at the tail */
	memcpy(buf->data + box_used_take(buf), new_data, sz);
	/* Increase the buf->used */
	box_used_inc(buf, sz);

	BUF_TEST(buf);
	return (OK);
}

ret_t box_merge(box_t *dst, box_t *src)
{
	ret_t rc;
	TESTP_ABORT(dst);
	TESTP_ABORT(src);
	rc = box_add(dst, src->data, src->used);

	if (OK != rc) {
		DE("The buf_add() failed\n");
		ABORT_OR_RETURN(rc);
	}

	if (OK != box_clean_and_reset(src)) {
		DE("Could not clean 'src' buffer\n");
		ABORT_OR_RETURN(-ECANCELED);
	}
	return rc;
}

/* Copy the given buffer "new_data" at tail of the buf_t */
ret_t box_replace(box_t *buf /* Buf_t to replace data in */,
				  const char *new_data /* Buffer to copy into the buf_t */,
				  const box_s64_t size /* Size of the new buffer to set */)
{
	box_s64_t current_room_size;

	/* NOTE: This function is not dedicated to reset the buf_t:
	 * It means, this function does not accept new_data == NULL + size == 0.
	   If one needs to reset the buf_t, there is a dedicated funtion for this task */
	TESTP_ABORT(buf);
	TESTP_ABORT(new_data);

	/* We can not add buffer of 0 bytes or less; probably , if we here it is a bug */
	if (size < 1) {
		DE("Wrong argument(s): b = %p, buf = %p, size = %ld\n", buf, new_data, size);
		TRY_ABORT();
		return (-EINVAL);
	}

	/* We use this variable to minimize number of calls of buf_room_take(buf) */
	current_room_size = box_room_take(buf);

	/* Assure that we have enough room to set the new buffer */
	if (size > current_room_size &&  /* If size of the new buffer bigger than current buffer */
		OK != box_room_assure(buf, size - current_room_size)) { /* And if we could not allocated additional memory */
		DE("Can't add room into buf_t\n");
		TRY_ABORT();
		return (-ENOMEM);
	}

	/* And now we are adding the buffer at the tail */
	memcpy(buf->data, new_data, size);

	/* Set the new buf->used */
	box_used_set(buf, size);

	BUF_TEST(buf);
	return (OK);
}

box_s64_t box_room_take(const box_t *buf)
{
	TESTP_ABORT(buf);
	return (buf->room);
}

void box_room_set(box_t *buf, box_s64_t room)
{
	TESTP_ABORT(buf);
	DDD("Setting buffer room: %ld\n", room);
	buf->room = room;
}

void box_room_inc(box_t *buf, box_s64_t inc)
{
	TESTP_ABORT(buf);
	DDD("Inc buffer room: %ld + %ld\n", buf->room, inc);
	buf->room += inc;
}

void box_room_dec(box_t *buf, box_s64_t dec)
{
	TESTP_ABORT(buf);
	if (dec > box_room_take(buf)) {
		DE("Asked to decrement buf->room to a negative value");
		TRY_ABORT();
	}

	DDD("Dec buffer room: %ld - %ld\n", buf->room, dec);

	if (buf->room >= dec) {
		buf->room -= dec;
	} else {
		buf->room = 0;
	}
}

ret_t box_pack(box_t *buf)
{
	TESTP_ABORT(buf);

	/*** If buffer is empty we have nothing to do */

	if (NULL == buf->data) {
		return (OK);
	}

	/*** Sanity check: dont' process invalide buffer */

	if (OK != box_is_valid(buf, __func__, __LINE__)) {
		DE("Buffer is invalid - can't proceed\n");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/*** Should we really pack it? */
	if (box_used_take(buf) == box_room_take(buf)) {
		/* No need to pack it */
		return (OK);
	}

	/* Here shrink the buffer */
	if (OK != box_realloc(buf, box_used_take(buf))) {
		DE("Can not realloc buf->data\n");
		return (BAD);
	}

	box_room_set(buf, box_used_take(buf));

	/* Here we are if buf->used == buf->room */
	BUF_TEST(buf);
	return (OK);
}


