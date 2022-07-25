#ifndef _BUF_T_H_
#define _BUF_T_H_

#include <stddef.h>
#include <sys/types.h>

#define BUF_NOISY
#include <stdint.h>

#ifndef MIN
	#define MIN(a,b) ((a < b) ? a : b)
#endif

enum {
 	AGN = -2, /* "Try again" status */
 	BAD = -1,  /* Error status */
 	OK = 0,    /* Success status */
 	YES = 0,   /* YES in case a function returns YES or NO */
 	NO = 1,    /* NO in case a function returns YES or NO */
};

typedef int ret_t;

#define TESTP(x, ret) do {if(NULL == x) { DDE("Pointer %s is NULL\n", #x); return ret; }} while(0)
#define TESTP_ASSERT(x, mes) do {if(NULL == x) { DE("[[ ASSERT! ]] %s == NULL: %s\n", #x, mes); abort(); } } while(0)

/* This is a switchable version; depending on the global abort flag it will abort or rturn an error */
// extern int bug_get_abort_flag(void);
//#define T_RET_ABORT(x, ret) do {if(NULL == x) {if(0 != bug_get_abort_flag()) {DE("[[ ASSERT! ]] %s == NULL\n", #x);abort();} else {DDE("[[ ERROR! ]]: Pointer %s is NULL\n", #x); return ret;}}} while(0)
#define T_RET_ABORT(x, ret) do {if(NULL == x) {DE("[[ ASSERT! ]] %s == NULL\n", #x);abort(); }} while(0)
#define T_RET_ABORT_VOID(x) do {if(NULL == x) {DE("[[ ASSERT! ]] %s == NULL\n", #x);abort(); }} while(0)

#ifdef TFREE_SIZE
	#undef TFREE_SIZE
#endif

#define TFREE_SIZE(x,sz) do { if(NULL != x) {memset(x,0,sz);free(x); x = NULL;} else {DE(">>>>>>>> Tried to free_size() NULL: %s (%s +%d)\n", #x, __func__, __LINE__);} }while(0)

/* Size of 'room' and 'used':
 * 1. If this type is "uint64", the max size of data buffer is:
 * 18446744073709551615 bytes,
 * or 18446744073709552 KB,
 * or 18446744073709.550781 Mb
 * or 18446744073.709552765 Gb
 * or 18446744.073709551245 Tb
 *
 * Large enough to keep whatever you want, at least for the next 10 years )
 *
 * 2. In case of CIRCULAR buffer we use half of the 'used' field to keep head of the buffer,
 * another half to keep the tail. In this case max value of the head tail is:
 *
 * 4294967295 bytes,
 * or 4294967.295 Kb
 * or 4294.967295 Mb
 * or 4.294967295 Gb
 *
 * Not as impressive as uint64. In case we need more, the type should be increased to uint128
 * 
 * 2. If this size redefined to uint16, the max size of data buffer is:
 * 65535 bytes, or 65 Kilobyte
 * 
 * Be careful if you do resefine this type.
 * If you plan to used buf_t for file opening / reading / writing, you may have a problem if this
 * type is too small.
 * 
 */

typedef int64_t buf_s64_t;
typedef uint32_t buf_u32_t;

/**
 * Simple struct to hold a buffer / string and its size / lenght
 */
typedef struct {
	buf_s64_t room;	/**< Allocated size */
	buf_s64_t used;	/**< Used size */
	char *data;		/**< Pointer to data */
} buf_t;

/** If there is 'abort on error' is set, this macro stops
 *  execution and generates core file */
// #define TRY_ABORT() do{ if(0 != bug_get_abort_flag()) {DE("Abort in %s +%d\n", __FILE__, __LINE__);abort();} } while(0)

/**
 * @author Sebastian Mountaniol (7/21/22)
 * @brief Print out a buf internals - used, room, pointers
 * @param const buf_t* buf   
 * @param const char* mes   
 * @details 
 */
extern void buf_dump(const buf_t *buf, const char *mes);

/**
 * @author Sebastian Mountaniol (01/06/2020)
 * @func err_t buf_data_set(buf_t *buf, char *data, size_t size,
 *  	 size_t len)
 * @brief Set new data into buffer. The buf_t *buf must be clean, i.e. buf->data == NULL and
 *  buf->room == 0; After the new buffer 'data' set, the buf->len also set to 'len'
 * @param buf_t * buf 'buf_t' buffer to set new data 'data'
 * @param char * data Data to set into the buf_t
 * @param size_t size Size of the new 'data'
 * @param size_t len Length of data in the buffer, user must provide it
 * @return err_t Returns EOK on success
 *  Return -EACCESS if the buffer is read-only
 *  Return -EINVAL if buffer or data is NULL
 */
extern ret_t buf_data_set(buf_t *buf, char *data, const buf_s64_t size, const buf_s64_t len);

/**
 * @author Sebastian Mountaniol (12/19/21)
 * void* buf_data_take(buf_t *buf)
 * 
 * @brief Returns buf->data
 * @param buf - Buf to return data from
 * 
 * @return void* Returns buf->data; NULL on an error
 * @details After this operation the "buf_t" stays unchaged. The
 *  		"data" buffer still belongs to the "buf_t"; the
 *  		caller just gets pointer of this buffer.
 */
extern void *buf_data_take(const buf_t *buf);

/**
 * @author Sebastian Mountaniol (12/19/21)
 * ret_t buf_is_data_null(buf_t *buf)
 * 
 * @brief Test that the buf->data is NULL
 * @param buf - Buffer to test
 * 
 * @return ret_t YES if data is NULL, NO if data not NULL,
 *  	   -EINVAL if the buffer is NULL pointer
 * @details 
 */
extern ret_t buf_is_data_null(buf_t *buf);

/**
 * @author Sebastian Mountaniol (15/06/2020)
 * @func err_t buf_is_valid(buf_t *buf)
 * @brief Test bufer validity
 * @param buf_t * buf Buffer to test
 * @return err_t Returns EOK if the buffer is valid.
 * 	Returns EINVAL if the 'buf' == NULL.
 * 	Returns EBAD if this buffer is invalid.
 */
extern ret_t buf_is_valid(const buf_t *buf, const char *who, const int line);

/**
 * @func buf_t* buf_new(size_t size)
 * @brief Allocate buf_t. A new buffer of 'size' will be
 *    allocated.
 * @author se (03/04/2020)
 * @param size_t size Data buffer size, may be 0
 * @return buf_t* New buf_t structure.
 */
extern buf_t *buf_new(buf_s64_t size);

/**
 * @author Sebastian Mountaniol (01/06/2020)
 * @func void* buf_data_steal(buf_t *buf)
 * @brief 'Steal' data from buffer. After this operation the internal buffer 'data' returned to
 *    caller. After this function buf->data set to NULL, buf->len = 0, buf->size = 0
 * @param buf_t * buf Buffer to extract data buffer
 * @return void* Data buffer pointer on success, NULL on error. Warning: if the but_t did not have a
 * 	buffer (i.e. buf->data was NULL) the NULL will be returned.
 */
extern void *buf_data_steal(buf_t *buf);

/**
 * @author Sebastian Mountaniol (01/06/2020)
 * @func void* buf_data_steal_and_release(buf_t *buf)
 * @brief Return data buffer from the buffer and release the buffer. After this operation the buf_t
 *    structure will be completly destroyed. WARNING: disregarding to the return value the buf_t
 *    will be destoyed!
 * @param buf_t * buf Buffer to extract data
 * @return void* Pointer to buffer on success (buf if the buffer is empty NULL will be returned),
 */
extern void *buf_data_steal_and_release(buf_t *buf);

/**
 * @brief Remove data from buffer (and free the data), set buf->room = buf->len = 0
 * @author se (16/05/2020)
 * @param buf Buffer to remove data from
 * @return err_t EOK if all right
 * 	EINVAL if buf is NULL pointer
 * 	EACCESS if the buffer is read-only, buffer kept untouched
 * @details If the buffer is invalid (see buf_is_valid()),
 * @details the opreration won't be interrupted and buffer will be cleaned.
 */
extern ret_t buf_clean_and_reset(buf_t *buf);

/**
 * @func int buf_room_add_memory(buf_t *buf, size_t size)
 * @brief Allocate additional 'size' in the tail of buf_t data
 *    buffer; existing content kept unchanged. The new
 *    memory will be cleaned. The 'size' argument must be >
 *    0. For removing buf->data use 'buf_free_force()'
 * @author se (06/04/2020)
 * @param buf_t * buf Buffer to grow
 * @param size_t size How many byte to add
 * @return int EOK on success
 * 	-EINVAL if buf == NULL
 * 	-EACCESS if buf is read-only
 *  -ENOMEM if allocation of additional space failed. In this
 *  case the buffer kept untouched. -ENOKEY if the buffer marked
 *  as CAANRY but CANARY work can't be added.
 */
extern ret_t buf_room_add_memory(buf_t *buf, buf_s64_t size);

/**
 * @author Sebastian Mountaniol (6/10/22)
 * @brief Measure how much memory available in the buffer
 * @param buf_t* buf   Buffer to take measurement
 * @return buf_s64_t   Number of bytes available in the internal
 *  	   buffer
 * @details There are two metrics, in the buf_t structure. The
 *  		first is "room" which is size of internal buffer.
 *  		The second is "used" whcih is how much of the "room"
 *  		is used. This function returns the "room" remains:
 *  		"room - used"
 */
extern buf_s64_t buf_room_avaialable_take(buf_t *buf);

/**
 * @func int buf_room_assure(buf_t *buf, size_t expect)
 * @brief The function accept size in bytes that caller wants to
 *    add into the buf. It chaecks either additional room is
 *    needed. If so, it calls buf_add_room() to increase room.
 *    The 'expect' must be > 0
 * @author se (06/04/2020)
 * @param buf_t * buf Buffer to test
 * @param size_t expect How many bytes expected to be added
 * @return int EOK if the buffer has sufficient room or if room added succesfully
 * 	EINVAL if buf is NULL or 'expected' == 0
 * 	Also can return all error statuses of buf_add_room()
 */
extern ret_t buf_room_assure(buf_t *buf, buf_s64_t expect);

/**
 * @func int buf_t_free_force(buf_t *buf)
 * @brief Free buf; if buf->data is not empty, free buf->data
 * @author se (03/04/2020)
 * @param buf_t * buf Buffer to remove
 * @return int EOK on success
 * 	EINVAL is the buf is NULL pointer
 * 	EACCESS if the buf is read-only
 * 	ECANCELED if the buffer is invalid
 */
extern ret_t buf_free(buf_t *buf);

/**
 * @func err_t buf_add(buf_t *buf, const char *new_data, const size_t size)
 * @brief Append (copy) buffer "new_data" of size "size" to the tail of buf_t->data
 *    New memory allocated if needed.
 * @author se (06/04/2020)
 * @param buf_t * buf Buffer to append to buf->data
 * @param const char* new_data This buffer will be appended (copied) to the tail of buf->data
 * @param const size_t size Size of 'new_data' in bytes
 * @return int EOK on success
 * 	EINVAL if: 'buf' == NULL, or 'new_data' == NULL, or 'size' == 0
 * 	EACCESS if the 'buf' is read-only
 * 	ENOMEM if new memory can't be allocated
 */
extern ret_t buf_add(buf_t *buf, const char *new_data, const buf_s64_t size);

/**
 * @author Sebastian Mountaniol (7/24/22)
 * @brief Merge add data from 'src' buf to 'dst'
 * @param buf_t* dst   Buffer to add data from 'src'
 * @param const buf_t* src   Buffer to read data and add to
 *  			'src'
 * @return ret_t OK on success, all statuses of buf_add() on an
 *  	   error
 * @details The 'src' buffer will be freed after succesful
 *  		operation;<br>
 *  		-ECANCELED in case the 'src' buffer can not be
 *  		cleaned.<br>
 *  		In case the operation failed, the status of both
 *  		'src' and 'dst' is undefined.
 */
extern ret_t buf_merge(buf_t *dst, buf_t *src);
/**
 * @author Sebastian Mountaniol (7/17/22)
 * @brief Replace current data in the Buf_t to a new buffer
 * @param buf_t* buf     Buf_t to replace data buffer. Can not
 *  		   be NULL.
 * @param const char* new_data New data buffer to set. Must be >
 *  			0
 * @param const buf_s64_t size    Size of the new data buffer
 * @return ret_t OK on success, -EINVAL on an invalid argument,
 *  	   -ENOMEM if could not allocate memory
 * @details NOT TESTED YET. This function dedicated to replace
 *  		Buf_t data with a new buffer. The new data buffer
 *  		must be not NULL, and its size must be > 0,
 *  		otherwise this function fails or returns an error.
 */
ret_t buf_replace(buf_t *buf, const char *new_data, const buf_s64_t size );

/**
 * @author Sebastian Mountaniol (14/06/2020)
 * @func buf_usize_t buf_used_take(buf_t *buf)
 * @brief Return size in bytes of used memory (which is buf->used)
 * @param buf_t * buf Buffer to check
 * @return ssize_t Number of bytes used on success
 * 	EINVAL if the 'buf' == NULL
 */
extern buf_s64_t buf_used_take(const buf_t *buf);

/**
 * @author Sebastian Mountaniol (12/16/21)
 * void buf_used_set(buf_t *buf, buf_usize_t used)
 * 
 * @brief Set a new value of the buf->used
 * @param buf - Buffer to set the new value
 * @param used - The new value to set
 * 
 * @details 
 */
extern void buf_used_set(buf_t *buf, buf_s64_t used);

/**
 * @author Sebastian Mountaniol (12/16/21)
 * void buf_used_inc(buf_t *buf, buf_usize_t inc)
 * 
 * @brief Increment the buf->used value by 'inc'
 * @param buf - The buffer to increment the value of the
 *  		  buf->used
 * @param inc - The value to add to the buf->used
 * @details 
 */
extern void buf_used_inc(buf_t *buf, buf_s64_t used);

/**
 * @author Sebastian Mountaniol (12/16/21)
 * void buf_used_dec(buf_t *buf, buf_usize_t dec)
 * 
 * @brief Decrement value of the buf->used 
 * @param buf - Buffer to decrement the '->used'
 * @param dec - Decrement ->used by this value; the 'used' must
 *  		   be > 0 (it can not be < 0, however)
 * 
 * @details Be aware: this function will not set the value of
 *  		"used" a negative. If you try to set "used" value 
 *  		negative, it set value to 0. If the buf_t compiled
 *  		with ABORT ON ERROR mode, it will terminate on
 *  		attempt to set "used" a negative number.
 *
 */
extern void buf_used_dec(buf_t *buf, buf_s64_t dec);

/**
 * @author Sebastian Mountaniol (14/06/2020)
 * @brief Return size of memory currently allocated for this 'buf' (which is buf->room)
 * @param buf_t * buf Buffer to test
 * @return ssize_t How many bytes allocated for this 'buf'
 * 	EINVAL if the 'buf' == NULL
 */
extern buf_s64_t buf_room_take(const buf_t *buf);

/**
 * @author Sebastian Mountaniol (12/16/21)
 * @brief Set new buf->room value
 * @param buf - Buffer to set the a value
 * @param room - The new value of buf->room to set
 * 
 * @details 
 */
extern void buf_room_set(buf_t *buf, buf_s64_t room);

/**
 * @author Sebastian Mountaniol (12/16/21)
 * @brief Increment value of buf->room by 'inc' value
 * @param buf - Buffer to increment the buf->room value
 * @param inc - The value to add to buf->room
 * 
 * @return ret_t OK on sucess, BAD on an error
 * @details 
 */
extern void buf_room_inc(buf_t *buf, buf_s64_t inc);

/**
 * @author Sebastian Mountaniol (12/16/21)
 * void buf_dec_room(buf_t *buf, buf_usize_t dec)
 * 
 * @brief Decrement the value of buf->room by 'dec' value
 * @param buf - The buffer to decrement buf->room
 * @param dec - The value to substract from nuf->room
 * 
 * @return ret_t OK on sucess, BAD on an error
 * @details The 'dec' must be less or equal to the buf->room,
 *  		else BAD error returned and no value decremented
 */
extern void buf_room_dec(buf_t *buf, buf_s64_t dec);

/**
 * @author Sebastian Mountaniol (01/06/2020)
 * @func err_t buf_pack(buf_t *buf)
 * @brief Shrink buf->data to buf->len.
 * @brief This function may be used when you finished with the buf_t and
 *    its size won't change anymore.
 *    The unused memory will be released.
 * @param buf_t * buf Buffer to pack
 * @return err_t EOK on success;
 * 	EOK if this buffer is empty (buf->data == NULL) EOK returned
 * 	EOK if this buffer should not be packed (buf->used == buf->room)
 * 	EINVAL id 'buf' == NULL
 * 	ECANCELED if this buffer is invalid (see buf_is_valid)
 * 	ENOMEM if internal realloc can't reallocate / shring memory
 * 	Also can return one of buf_set_canary() errors
 */
extern ret_t buf_pack(buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_detect_used(buf_t *buf)
 * @brief If you played with the buffer's data (for example, copied / replaced tezt in the
 *    buf->data) this function will help to detect right buf->used value.
 * @param buf_t * buf Buffer to analyze
 * @return err_t EOK on succes + buf->used set to a new value
 * 	EINVAL is 'buf' is NULL
 * 	ECANCELED if the buf is invalid or if the buf is empty
 */
extern ret_t buf_detect_used(buf_t *buf);

/* Additional defines */
#ifdef BUF_DEBUG
	#define BUF_TEST(buf) do {if (0 != buf_is_valid(buf, __func__, __LINE__)){fprintf(stderr, "######>>> Buffer invalid here: func: %s file: %s + %d [allocated here: %s +%d %s()]\n", __func__, __FILE__, __LINE__, buf->filename, buf->line, buf->func);}} while (0)
	#define BUF_DUMP(buf) do {DD("[BUFDUMP]: [%s +%d] buf = %p, data = %p, room = %u, used = %u [allocated here: %s +%d %s()]\n", __func__, __LINE__, buf, buf->data, buf->room, buf->used, buf->filename, buf->line, buf->func);} while(0)
	#define BUF_DUMP_ERR(buf) do {DD("[BUFDUMP]: [%s +%d] buf = %p, data = %p, room = %u, used = %u [allocated here: %s +%d %s()]\n", __func__, __LINE__, buf, buf->data, buf->room, buf->used, buf->filename, buf->line, buf->func);} while(0)
#else
	#define BUF_TEST(buf) do {if (0 != buf_is_valid(buf, __func__, __LINE__)){fprintf(stderr, "######>>> Buffer test invalid here: func: %s file: %s + %d\n", __func__, __FILE__, __LINE__);}} while (0)
	#define BUF_DUMP(buf) do {DD("[BUFDUMP]: [%s +%d] buf = %p, data = %p, room = %u, used = %u\n", __func__, __LINE__, buf, buf->data, buf->room, buf->used);} while(0)
	#define BUF_DUMP_ERR(buf) do {DD("[BUFDUMP]: [%s +%d] buf = %p, data = %p, room = %u, used = %u\n", __func__, __LINE__, buf, buf->data, buf->room, buf->used);} while(0)

#endif

#ifndef BUF_NOISY
	#undef BUF_DUMP
	#define BUF_DUMP(buf) do{}while(0)
#endif

#endif /* _BUF_T_H_ */
