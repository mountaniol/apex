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
#include "optimization.h"

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
void bx_dump(const box_t *box, const char *mes)
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

	DD("box_t data ptr:       %p\n",   bx_data_take(box));
	DD("box_t used:           %lld\n", bx_used_take(box));
	DD("box_t room:           %lld\n", bx_room_take(box));
	DD("box_t members:        %lld\n", bx_members_take(box));
	DD("========================\n");
}
#else
void bx_dump(__attribute__((unused)) const box_t *box, __attribute__((unused)) const char *mes)
{ }
#endif /* BOX_DUMP */

/* This function tests size against the chosen box_typee_t;
   if the given size is in limits of the size, it returns 0, else it will return 1 */
int bx_if_size_fits_box_type(ssize_t size)
{
	if (size < 0) {
		return -1;
	}

	if ((size_t)size > (size_t)MAX_VAL_BOX_TYPE) {
		return 1;
	}

	return 0;
}

/*************************************************************************
 *** Get (Take), Set, INcrease and Decrease value of 'used' and 'room' ***
 *************************************************************************/

/*** box->used field ***/

__attribute__((warn_unused_result, pure))
box_s64_t bx_used_take(const box_t *box)
{
	TESTP_ABORT(box);
	return (box->used);
}

void bx_used_set(box_t *box, const box_s64_t used)
{
	TESTP_ABORT(box);

	if (bx_if_size_fits_box_type(used)) {
		DE("The asked size is too large for the box\n");
		abort();
	}
	box->used = used;
}

void bx_used_inc(box_t *box, const box_s64_t inc)
{
	TESTP_ABORT(box);

	/* Test that the new value fits current data type */
	if (bx_if_size_fits_box_type(box->used + inc)) {
		DE("The asked size is too large for the box\n");
		abort();
	}
	box->used += inc;
}

void bx_used_dec(box_t *box, const box_s64_t dec)
{
	TESTP_ABORT(box);

	if (bx_if_size_fits_box_type(dec)) {
		DE("The asked size is too large for the box\n");
		abort();
	}
	if (dec > bx_used_take(box)) {
		DE("Tried to decrement 'box->used' such that it will be < 0: current %ld, asked decrement %ld\n",
		   (uint64_t)box->used, dec);
		TRY_ABORT();
	}

	if (box->used >= (box_type_t)dec) {
		box->used -= (box_type_t)dec;
	} else {
		DE("Tried to decrement 'box->used' such that it will be < 0: current %ld, asked decrement %ld\n",
		   (uint64_t)box->used, dec);
		TRY_ABORT();
		box->used = 0;
	}
}

/*** End of box->used field ***/

/*** box->members field ***/

__attribute__((warn_unused_result, pure))
box_s64_t bx_members_take(const box_t *box)
{
	TESTP_ABORT(box);
	return (box->members);
}

void bx_members_set(box_t *box, const box_s64_t members)
{
	TESTP_ABORT(box);

	if (bx_if_size_fits_box_type(members)) {
		DE("The asked number of members is too large for the current type\n");
		abort();
	}
	box->members = members;
}

void bx_members_inc(box_t *box, const box_s64_t inc)
{
	TESTP_ABORT(box);

	/* Test that the new value fits the current data type */
	if (bx_if_size_fits_box_type(box->members + inc)) {
		DE("The number of members is too large for the box\n");
		abort();
	}
	box->members += inc;
}

void bx_members_dec(box_t *box, const box_s64_t dec)
{
	TESTP_ABORT(box);

	if (bx_if_size_fits_box_type(dec)) {
		DE("The asked size is too large for the box\n");
		abort();
	}
	if (dec > bx_members_take(box)) {
		DE("Tried to decrement 'box->members' such that it will be < 0: current %ld, asked decrement %ld\n",
		   (uint64_t)box->members, dec);
		TRY_ABORT();
	}

	if (box->members >= (box_type_t)dec) {
		box->members -= (box_type_t)dec;
	} else {
		DE("Tried to decrement 'box->members' such that it will be < 0: current %ld, asked decrement %ld\n",
		   (uint64_t)box->members, dec);
		TRY_ABORT();
		box->members = 0;
	}
}

size_t bx_member_size(box_t *box)
{
	TESTP_ABORT(box);
	if (0 == bx_members_take(box)) {
		return 0;
	}

	return (size_t)bx_used_take(box) / (size_t)bx_members_take(box);
}

void *bx_member_ptr(box_t *box, size_t member)
{
	TESTP_ABORT(box);
	size_t num_members = bx_members_take(box);
	char   *data;

	if (0 == num_members) {
		DDE("There are 0 members in the box; looks like the box is empty\n");
		return NULL;
	}

	if (member > num_members) {
		DDE("Asked member (%zu) > number of members (%zu) in the box\n",
			member, num_members);
		return NULL;
	}


	data = bx_data_take(box);
	return (data + member);
}

ret_t bx_member_copy(box_t *box, size_t member, void *buf, size_t buf_size)
{
	TESTP_ABORT(box);
	void   *mem_ptr    = bx_member_ptr(box, member);
	size_t member_size;

	if (NULL == mem_ptr) {
		DE("Could not get member ptr\n");
		return -1;
	}

	member_size = bx_member_size(box);
	if (0 == member_size) {
		DE("Could not get member size");
		return -1;
	}

	if (buf_size < member_size) {
		DE("Can not copy: member size (%zu) > user's buffer size (%zu)\n",
		   member_size, buf_size);
	}

	memcpy(buf, mem_ptr, member_size);
	return 0;
}


/*** End of box->members field ***/

/*** box->room field ***/

__attribute__((warn_unused_result, pure))
box_s64_t bx_room_take(const box_t *box)
{
	TESTP_ABORT(box);
	return (box->room);
}

void bx_room_set(box_t *box, const box_s64_t room)
{
	TESTP_ABORT(box);
	if (bx_if_size_fits_box_type(room)) {
		DE("The asked size of room (%ld) is too large for the current data type (%ld)\n",
		   room, (uint64_t)MAX_VAL_BOX_TYPE);
		abort();
	}
	DDD("Setting box room: %ld\n", room);
	box->room = room;
}

void bx_room_inc(box_t *box, const box_s64_t inc)
{
	TESTP_ABORT(box);
	if (bx_if_size_fits_box_type(inc)) {
		DE("The asked size is too large for the box\n");
		abort();
	}

	DDD("Inc box room: %ld + %ld\n", (uint64_t)box->room, inc);

	box->room += inc;
}

void bx_room_dec(box_t *box, const box_s64_t dec)
{
	TESTP_ABORT(box);

	if (bx_if_size_fits_box_type(dec)) {
		DE("The asked size is too large for the box\n");
		abort();
	}

	if (dec > bx_room_take(box)) {
		DE("Asked to decrement box->room to a negative value");
		TRY_ABORT();
	}


	DDD("Dec box room: %ld - %ld\n", (uint64_t)box->room, dec);

	if (box->room >= (box_type_t)dec) {
		box->room -= (box_type_t)dec;
	} else {
		box->room = 0;
	}
}

/*** End of box->room field ***/

/* Validate sanity of box_t - common for all boxes */
__attribute__((warn_unused_result, pure))
ret_t bx_is_valid(const box_t *box, const char *who, const int line)
{
	TESTP_ABORT(box);

	/* box->used always <= box->room */
	if (bx_used_take(box) > bx_room_take(box)) {
		DE("/%s +%d/ : Invalid box: box->used (%ld) > box->room (%ld)\n",
		   who, line, bx_used_take(box), bx_room_take(box));
		bx_dump(box, "from box_is_valid(), before terminating 1");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/* Always: box->members <= box->used box->used always <= box->room */
	if (bx_used_take(box) < bx_members_take(box)) {
		DE("/%s +%d/ : Invalid box: box->used (%ld) < box->members (%ld)\n",
		   who, line, bx_used_take(box), bx_members_take(box));
		bx_dump(box, "from box_is_valid(), before terminating 1");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/* Always: if box->used > 0, the box->members > 0 as well */
	if (0 != bx_used_take(box) && 0 == bx_members_take(box)) {
		DE("/%s +%d/ : Invalid box: box->used (%ld) > 0, box->members (%ld) == 0\n",
		   who, line, bx_used_take(box), bx_members_take(box));
		bx_dump(box, "from box_is_valid(), before terminating 1");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/* The box->data can be NULL if and only if (box->used + box->room + box->members) == 0;
	 * However, we don't check box->used: we tested that it <= box->room already */
	if ((NULL == box->data) &&
		((bx_room_take(box) +  bx_used_take(box) + bx_members_take(box)) > 0)) {
		DE("/%s +%d/ : Invalid box: box->data == NULL but box->room > 0 (%ld) / box->used (%ld)\n",
		   who, line, bx_room_take(box), bx_used_take(box));
		bx_dump(box, "from box_is_valid(), before terminating 2");
		TRY_ABORT();
		return (-ECANCELED);
	}

	/* And vice versa: if box->data != NULL the box->room must be > 0 */
	if ((NULL != box->data) && (0 == bx_room_take(box))) {
		DE("/%s +%d/: Invalid box: box->data != NULL but box->room == 0\n", who, line);
		bx_dump(box, "from box_is_valid(), before terminating 3");
		TRY_ABORT();
		return (-ECANCELED);
	}



	DDD0("/%s +%d/: Box is valid\n", who, line);
	return (A_OK);
}

__attribute__((warn_unused_result))
box_t *bx_new(const box_s64_t size)
{
	box_t  *box;

	if (size < 0) {
		ABORT_OR_RETURN(NULL);
	}

	if (bx_if_size_fits_box_type(size)) {
		DE("The asked size is too large for the box\n");
		abort();
	}

	box = (box_t *)zmalloc(sizeof(box_t));
	T_RET_ABORT(box, NULL);

	/* If a size is given than allocate a data */
	if (size > 0) {

		box->data = (char *)zmalloc(size);
		TESTP_ASSERT(box->data, "Can't allocate box->data");
	}

	/* Assigned value to box->room field */
	bx_room_set(box, size);

	/* Assigned value to box->used field */
	bx_used_set(box, 0);

	/* Used == 0, means members == 0 */
	bx_members_set(box, 0);

	/* Just in case, check that the box_t looks valid */
	if (A_OK != bx_is_valid(box, __func__, __LINE__)) {
		DE("Box is invalid right after allocation!\n");
		if (A_OK != bx_free(box)) {
			DE("Can not free the box\n");
		}
		TRY_ABORT();
		return (NULL);
	}

	return (box);
}

__attribute__((warn_unused_result))
ret_t bx_data_set(box_t *box, char *data, const box_s64_t size, const box_s64_t len)
{
	TESTP_ABORT(box);
	TESTP_ABORT(data);

	if (bx_if_size_fits_box_type(size)) {
		DE("The asked size is too large for the box\n");
		abort();
	}

	if (bx_if_size_fits_box_type(len)) {
		DE("The asked size is too large for the box\n");
		abort();
	}

	box->data = data;
	bx_room_set(box, size);
	bx_used_set(box, len);

	/* In case the box was set data from no data state, we increase members to 1 */
	if (bx_used_take(box) > 0 && 0 == bx_members_take(box)) {
		bx_members_set(box, 1);
	}

	/* In case the box was set data to NULL, we reset members to 0 */
	if (0 == bx_used_take(box) && bx_members_take(box) > 0) {
		bx_members_set(box, 0);
	}

	return (A_OK);
}

__attribute__((warn_unused_result))
void *bx_data_steal(box_t *box)
{
	/* Keep temporarly pointer of intennal data buffer */
	void *data;
	TESTP_ABORT(box);
	data = box->data;
	box->data = NULL;
	bx_room_set(box, 0);
	bx_used_set(box, 0);
	bx_members_set(box, 0);
	return (data);
}

__attribute__((warn_unused_result, pure))
void *bx_data_take(const box_t *box)
{
	TESTP_ABORT(box);
	return (box->data);
}

__attribute__((warn_unused_result, pure))
ret_t bx_is_data_null(const box_t *box)
{
	TESTP_ABORT(box);
	if (NULL == box->data) {
		return YES;
	}
	return NO;
}

/* This is an internal function. Here we realloc the internal box_t buffer */
__attribute__((warn_unused_result))
static ret_t bx_realloc(box_t *box, const size_t new_size)
{
	size_t size_to_copy = 0;
	void   *tmp         = NULL;

	TESTP_ABORT(box);

	size_to_copy = MIN((size_t)box->room, new_size);

	// box_dump(box, "in Box Realloc, before");

	//DDD("Going to allocate new buffer %zu size; room = %zu, size_to_copy = %zu\n", new_size, room, size_to_copy);
	tmp = malloc(new_size);
	//DDD("Allocated size %zu\n", new_size);

	if (NULL == tmp) {
		DE("New memory alloc failed: current size = %ld, asked size = %zu\n", (uint64_t)box->room, new_size);
		ABORT_OR_RETURN(-ENOMEM);
	}

	/* The new size can be less than previous; so we use minimal between bif->room and new_size to copy data */
	if (box->data) {
		memcpy(tmp, box->data, size_to_copy);
		free(box->data);
		box->data = NULL;
	}

	box->data = tmp;
	return A_OK;
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
	return A_OK;
}
#endif

__attribute__((warn_unused_result))
ret_t bx_room_add_memory(box_t *box, const box_s64_t sz)
{
	size_t original_room_size;
	TESTP_ABORT(box);

	if (bx_if_size_fits_box_type(sz)) {
		DE("The asked size is too large for the box\n");
		abort();
	}

	bx_dump(box, "box_room_add_memory(): Before adding memory");
	if (0 == sz) {
		DE("Bad arguments: box == NULL (%p) or size == 0 (%ld)\b", box, sz);
		ABORT_OR_RETURN(-EINVAL);
	}

	/* Save the */
	original_room_size = bx_room_take(box);

	if (A_OK != bx_realloc(box, original_room_size + sz)) {
		DE("Can not reallocate box->data\n");
		ABORT_OR_RETURN(-ENOMEM);
	}

	bx_dump(box, "box_room_add_memory(): After adding memory");

	/* Clean newely allocated memory */
	memset(box->data + original_room_size, 0, sz);

	bx_dump(box, "box_room_add_memory(): After cleaning new memory");

	/* Case 3: realloc succeeded, the same pointer - we do nothing */
	/* <Beeep> */

	/* Increase box->room */
	bx_room_inc(box, sz);
	// box_dump(box, "After adding memory");

	BOX_TEST(box);
	return (A_OK);
}

/* Return how much bytes is available in the box_t */
__attribute__((warn_unused_result, pure))
box_s64_t bx_room_avaialable_take(const box_t *box)
{
	box_s64_t m_used = -1;
	box_s64_t m_room = -1;
	TESTP_ABORT(box);
	m_used = bx_used_take(box);
	if (m_used < 0) {
		MES_ABORT("box_used_take(box) returned a value < 0");
	}
	m_room = bx_room_take(box);
	if (m_room < 0) {
		MES_ABORT("box_room_take(box) returned a value < 0");
	}

	if (m_room < m_used) {
		DE("A bug: box->room (%ld) can not be < box->used (%ld) ; probably memory coruption or misuse\n",
		   m_room, m_used);
	}
	return m_room - m_used;
}

__attribute__((warn_unused_result))
ret_t bx_room_assure(box_t *box, const box_s64_t expect)
{
	TESTP_ABORT(box);

	if (bx_if_size_fits_box_type(box->room + expect)) {
		DE("The asked size is too large for the box\n");
		abort();
	}

	/* It is meanless to pass expected == 0 */
	if (expect < 1) {
		DE("'expected' size <= 0 : %ld\n", expect);
		TRY_ABORT();
		return (-EINVAL);
	}

	/* If we have enough room, return A_OK */
	if (bx_room_avaialable_take(box) >= expect) {
		return (A_OK);
	}

	return (bx_room_add_memory(box, expect));
}

__attribute__((warn_unused_result))
ret_t bx_clean_and_reset(box_t *box)
{
	TESTP_ABORT(box);

	if (A_OK != bx_is_valid(box, __func__, __LINE__)) {
		DE("Warning: box is invalid\n");
	}

	if (box->data) {
		/* Security: zero memory before it freed */
		DDD("Cleaning before free, data %p, size %ld\n", box->data, bx_room_take(box));
		memset(box->data, 0, bx_room_take(box));
		free(box->data);
		box->data = NULL;
	}

	memset(box, 0, sizeof(box_t));
	return (A_OK);
}

__attribute__((warn_unused_result))
ret_t bx_free(box_t *box)
{
	TESTP_ABORT(box);

	/* Just in case, test that the box_t is valid */
	if (A_OK != bx_is_valid(box, __func__, __LINE__)) {
		DE("Warning: box is invalid\n");
		return -1;
	}

	/* If there's an internal buffer, release it */
	if (NULL != box->data) {
		TFREE_SIZE(box->data, bx_used_take(box));
	}
	/* Release the box_t struct */
	TFREE_SIZE(box, sizeof(box_t));
	return (A_OK);
}

/* Copy the given buffer "new_data" at tail of the buf_t */
__attribute__((warn_unused_result))
ret_t bx_add(box_t *box /* box_t to add into */,
			 const char *new_data /* Buffer to add */,
			 const box_s64_t sz /* Size of the buffer to add */)
{
	TESTP_ABORT(box);
	TESTP_ABORT(new_data);

	if (bx_if_size_fits_box_type(box->room + sz)) {
		DE("The asked size is too large for the box\n");
		abort();
	}

	/* We can not add buffer of 0 bytes or less; probably , if we here it is a bug */
	if (sz < 1) {
		DE("Wrong argument(s): box = %p, buf = %p, size = %ld\n", box, new_data, sz);
		ABORT_OR_RETURN(-EINVAL);
	}

	/* Assure that we have enough room to add this buffer */
	if (A_OK != bx_room_assure(box, sz)) {
		DE("Can't add room into box_t\n");
		ABORT_OR_RETURN(-ENOMEM);
	}

	bx_dump(box, "From buf_add");

	/* And now we are adding the buffer at the tail */
	memcpy(box->data + bx_used_take(box), new_data, sz);

	/* Increase the box->used */
	bx_used_inc(box, sz);


	/* If the box was empty, and now it increased, we need to set members == 1 */
	if (bx_used_take(box) > 0 && 0 == bx_members_take(box)) {
		bx_members_set(box, 1);
	}

	BOX_TEST(box);
	return (A_OK);
}

__attribute__((warn_unused_result))
ret_t bx_merge(box_t *dst, box_t *src)
{
	/* rc keeps the return value from box_add() */
	ret_t rc;
	TESTP_ABORT(dst);
	TESTP_ABORT(src);
	rc = bx_add(dst, src->data, src->used);

	if (A_OK != rc) {
		DE("The box_add() failed\n");
		ABORT_OR_RETURN(rc);
	}

	if (A_OK != bx_clean_and_reset(src)) {
		DE("Could not clean 'src' box\n");
		ABORT_OR_RETURN(-ECANCELED);
	}
	return rc;
}

/* Replace the internal box buffer with the buffer "new_data" */
__attribute__((warn_unused_result))
ret_t bx_replace_data(box_t *box /* box_t to replace data in */,
					  const char *new_data /* Buffer to copy into the box_t */,
					  const box_s64_t size /* Size of the new buffer to set */)
{
	box_s64_t current_room_size;

	/* NOTE: This function is not dedicated to reset the box_t:
	 * It means, this function does not accept new_data == NULL + size == 0.
	   If one needs to reset the box_t, there is a dedicated funtion for this task */
	TESTP_ABORT(box);
	TESTP_ABORT(new_data);

	if (bx_if_size_fits_box_type(size)) {
		DE("The asked size is too large for the box\n");
		abort();
	}

	/* We can not add buffer of 0 bytes or less; probably , if we here it is a bug */
	if (size < 1) {
		DE("Wrong argument(s): box = %p, buf = %p, size = %ld\n", box, new_data, size);
		TRY_ABORT();
		return (-EINVAL);
	}

	/* We use this variable to minimize number of calls of box_room_take(box) */
	current_room_size = bx_room_take(box);

	/* Assure that we have enough room to set the new buffer */
	if (size > current_room_size &&  /* If size of the new buffer bigger than current buffer */
		A_OK != bx_room_assure(box, size - current_room_size)) { /* And if we could not allocated additional memory */
		DE("Can't add room into box_t\n");
		TRY_ABORT();
		return (-ENOMEM);
	}

	/* And now we are adding the buffer at the tail */
	memcpy(box->data, new_data, size);

	/* Set the new box->used */
	bx_used_set(box, size);

	BOX_TEST(box);
	return (A_OK);
}


