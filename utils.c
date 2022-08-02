#include "debug.h"
#include "tests.h"
#include "utils.h"

ret_t buf_realloc(void **mem, size_t old_size, size_t new_size)
{
	size_t room          = (size_t)buf->room;
	size_t size_to_copy  = MIN(room, new_size);
	size_t size_to_clean = 0;
	void   *tmp;

	// buf_dump(buf, "in Buf Realloc, before");

	DD("Going to allocate new buffer %zu size; room = %zu, size_to_copy = %zu\n", new_size, room, size_to_copy);
	tmp = malloc(new_size);
	DD("Allocated size %zu\n", new_size);

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
	buf->room = new_size;

	if (buf->used > buf->room) {
		DE("Shrink buf->used (%ld) to size of buf->room (%ld)\n", buf->used, buf->room);
		buf->used = buf->room;
	}
	return OK;
}
