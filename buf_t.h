#ifndef _BUF_T_H_
#define _BUF_T_H_

#include <stddef.h>
#include <sys/types.h>
//#define BUF_DEBUG
#define BUF_NOISY

/*
 * buf_t is an implementation of abstract buffer.
 * This buffer keeps data and its size.
 * buf->data - data.
 * buf->room - allocated memory
 * buf->used - size of used memory
 * 
 * A set of function help to manipulate the buffer: allocate / delete / add data and more.
 * Here is several examples:
 * 
 * ==== Example 1 ====
 * // Allocate buffer
 * buf_t *buf = buf_new(NULL, 0);
 * 
 * // Add data to the buffer
 * buf_add(buf, "Aloha", 5);
 * buf_add(buf, "/", 1);
 * buf_add(buf, "Shalom", 6);
 * 
 * // Now buffer contains string "Aloha/Shalom" without null terminator.
 * // The buf->room == 12
 * // The buf->used == 12
 * 
 * // Destroy buffer
 * buf_free(buf);
 * 
 * // The buffer and its memory securely destroyed: all data filled with 0 before it releaased.
 * // The 'buf' structure as well filled with '0' before destroyed.
 * 
 * ==== Example 2 ====
 * 
 * // Allocate buffer with memory == 32 bytes
 * buf_t = buf_new(NULL, 32);
 * // buf->data is buffer 32 bytes filled with '0'
 * 
 * // Add data
 * buf_add(buf, "Aloha", 5);
 * // buf->data contains "Aloha"
 * // buf->used == 5
 * // buf->room == 32
 * 
 * // Shrink memory to size of used area
 * buf_pack(buf)
 * // buf->data contains "Aloha"
 * // buf->used == 5
 * // buf->room == 5
 * 
 * buf_free(buf);
 * 
 * ==== Example 3 ====
 * 
 * // print string into buf_t
 * buf_t *buf = buf_sprintf("%s %s %s", "Lemon", "is", "yellow"); // buf_t allocated; // string
 * length measured and buf->data allocated, including terminating '\0'
 * 
 * 
 * // Now buf->data contains "Lemon is yellow\0"
 * // buf->used == 16
 * // buf->room == 16
 * // The used length contains terminating \0
 * 
 * // Print this string
 * printf("buf->data == %s; buf->used = %u\n", buf->data, buf->used);
 * 
 * // Release buffer
 * buf_free(buf);
 * 
 * ==== Example 4 ====
 * 
 * // print string into buf_t
 * buf_t *buf = buf_sprintf("%s %s %s", "Lemon", "is", "yellow"); // buf_t allocated; // string
 * length measured and buf->data allocated, including terminating '\0'
 * 
 * 
 * // Now buf->data contains "Lemon is yellow\0"
 * // buf->used == 16
 * // buf->room == 16
 * // The used length contains terminating \0
 * 
 * // Steal string from buffer
 * char *str = buf_steal_data(buf);
 * // Now str == "Lemon is yellow\0"
 * // buf->used == 0
 * // buf->room == 0
 * // buf->data == NULL
 * 
 * // Print this string
 * printf("string == %s\n", str);
 * 
 * // Release buffer
 * buf_free(buf);
 * 
 * // Release string
 * free(str);
 * 
 */


/* For uint32_t / uint8_t */
//#include <linux/types.h>
/*@-skipposixheaders@*/
#include <stdint.h>
/*@=skipposixheaders@*/
/* For err_t */
//#include "mp-common.h"

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

/* Whidth of the flags field */
typedef uint8_t buf_t_flags_t;

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

/* Simple struct to hold a buffer / string and its size / lenght */
typedef struct {
	buf_s64_t room;	/* Allocated size */
	buf_s64_t used;	/* Used size */
	char *data;		/* Pointer to data */
} buf_t;

/* buf_t flags */

/* How many bits are reserved for buffer type */
#define BUF_T_TYPE_WIDTH 3

/* We use mask to isolate type from other flags */
#define BUF_T_TYPE_MASK  0x07

/** Types **/
/* This is just a regular buffer, keeping user's raw data.
   User knows what to do with it, we don't care */
#define BUF_T_RAW        	0

/* String buffer. In this case, additional tests enabled */
#define BUF_T_STRING        1

/* Bit buffer */
#define BUF_T_BIT      		2

/* Circular buffer */
#define BUF_T_CIRC			3
#define BUF_T_CIRC_HEAD_WIDTH (32)
#define BUF_T_CIRC_MASK (0x0000FFFF)

/** Flags **/

/* Buffer is read only; for example you may keep a static char * / const char * in buf_t */
#define BUF_T_READONLY     (1 << BUF_T_TYPE_WIDTH)

/* Buffer is compressed */
#define BUF_T_COMPRESSED (1 << (BUF_T_TYPE_WIDTH + 1))

/* Buffer is enctypted */
#define BUF_T_ENCRYPTED  (1 << (BUF_T_TYPE_WIDTH + 2))

/* Buffer is enctypted */
#define BUF_T_CANARY  (1 << (BUF_T_TYPE_WIDTH + 3))

/* Buffer is crc32 protected */
#define BUF_T_CRC  (1 << (BUF_T_TYPE_WIDTH + 4))

/*@access buf_t@*/
#define BUF_TYPE(buf) (buf->flags & BUF_T_TYPE_MASK)
#define IS_BUF_STRING(buf) ( BUF_TYPE(buf) == BUF_T_STRING )
#define IS_BUF_BIT(buf) ( BUF_TYPE(buf) == BUF_T_BIT )
#define IS_BUF_CIRC(buf) ( BUF_TYPE(buf) == BUF_T_CIRC )
#define IS_BUF_RO(buf) (buf->flags & BUF_T_READONLY)
#define IS_BUF_COMPRESSED(buf) (buf->flags & BUF_T_COMPRESSED)
#define IS_BUF_ENCRYPTED(buf) (buf->flags & BUF_T_ENCRYPTED)
#define IS_BUF_CANARY(buf) (buf->flags & BUF_T_CANARY)
#define IS_BUF_CRC(buf) (buf->flags & BUF_T_CRC)

#define SET_BUF_STRING(buf) (buf->flags |= BUF_T_STRING)
#define SET_BUF_RO(buf) (buf->flags |= BUF_T_READONLY)
#define SET_BUF_COMPRESSED(buf) (buf->flags |= BUF_T_COMPRESSED)
#define SET_BUF_ENCRYPTED(buf) (buf->flags |= BUF_T_ENCRYPTED)
/*@noaccess buf_t@*/

/* CANARY: Set a mark after allocated buffer*/
/* PRO and CONTRA of this method:*/
/* PRO: It can help to catch memory problems */
/* Contras: The buffer increased, and buffer validation should be run on every buffer operation */
/* The mark we set at the end of the buf if PROTECTED flag is enabled */

/* Size of canary */
//typedef uint32_t buf_t_canary_t;


//typedef uint32_t buf_t_canary_t;
/* We use 2 characters as canary tail = 1 short */
//typedef uint16_t buf_t_canary_t;
typedef uint8_t buf_t_canary_t;
typedef uint8_t buf_t_checksum_t;

/* Canary size: the size of special buffer added after data to detect data tail corruption */
#define BUF_T_CANARY_SIZE (sizeof(buf_t_canary_t))

//#define BUF_T_CANARY_WORD ((buf_t_canary_t) 0xFEE1F4EE)
#define BUF_T_CANARY_WORD ((buf_t_canary_t) 0x31415926)

// The CANARY char pattern is : 10101010 = 0XAA
#define BUF_T_CANARY_CHAR_PATTERN 0XAA
#define BUF_T_CANARY_SHORT_PATTERN 0XAAAA

//0x12345678)

#define BUF_T_CRC_SIZE (sizeof(buf_t_checksum_t))

/* Size of a regular buf_t structure */
#define BUF_T_STRUCT_SIZE (sizeof(buf_t))

/* Size of buf_t structure for network transmittion: without the last 'char *data' pointer */
#define BUF_T_STRUCT_NET_SIZE (sizeof(buf_t) - sizeof(char*))

/* How much bytes will be transmitted to send buf_t + its actual data */
#define BUF_T_NET_SEND_SIZE(b) (BUF_T_STRUCT_NET_SIZE + b->used)


#if 0 /* SEB */
/**
 * @author Sebastian Mountaniol (12/17/21)
 * buf_t_flags_t bug_get_abort_flag(void)
 * 
 * @brief Get the status of global "abort" flag. If this flag is
 *  	  set, the buf_t will abort on errors and generate a
 *  	  core file
 * @param void  
 * 
 * @return buf_t_flags_t Status of 'abort' flag. 0 means it is
 *  	   not ebabled.
 * @details 
 */
extern int bug_get_abort_flag(void);
#endif

/** If there is 'abort on error' is set, this macro stops
 *  execution and generates core file */
// #define TRY_ABORT() do{ if(0 != bug_get_abort_flag()) {DE("Abort in %s +%d\n", __FILE__, __LINE__);abort();} } while(0)

/**
 * @author Sebastian Mountaniol (16/06/2020)
 * @func void buf_set_abort(void)
 * @brief set "abort on errors" state
 * @param void
 */
extern void buf_set_abort_flag(void);

/**
 * @author Sebastian Mountaniol (16/06/2020)
 * @func void buf_unset_abort(void)
 * @brief Unset "abort on errors" state
 * @param void
 */
extern void buf_unset_abort_flag(void);

/**
 * @author Sebastian Mountaniol (14/06/2020)
 * @func void buf_default_flags(buf_t_flags_t f)
 * @brief Set default buf_t flags. Will be applied for every allocated new buf_t struct.
 * @param buf_t_flags_t f
 */
extern void buf_default_flags(buf_t_flags_t f);

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
extern ret_t buf_data_set(/*@temp@*//*@in@*//*@special@*/buf_t *buf, /*@null@*/ /*@only@*/ /*@in@*/char *data, const buf_s64_t size, const buf_s64_t len);

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
extern void /*@temp@*/ *buf_data_take(/*@temp@*//*@in@*//*@special@*/buf_t *buf);


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
extern ret_t buf_is_data_null(/*@temp@*//*@in@*//*@special@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (15/06/2020)
 * @func err_t buf_is_valid(buf_t *buf)
 * @brief Test bufer validity
 * @param buf_t * buf Buffer to test
 * @return err_t Returns EOK if the buffer is valid.
 * 	Returns EINVAL if the 'buf' == NULL.
 * 	Returns EBAD if this buffer is invalid.
 */
extern ret_t buf_is_valid(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @func buf_t* buf_new(size_t size)
 * @brief Allocate buf_t. A new buffer of 'size' will be
 *    allocated.
 * @author se (03/04/2020)
 * @param size_t size Data buffer size, may be 0
 * @return buf_t* New buf_t structure.
 */
extern /*@null@*/ /*@in@*/ buf_t *buf_new(buf_s64_t size);

/**
 * @author Sebastian Mountaniol (16/06/2020)
 * @func err_t buf_set_data_ro(buf_t *buf, char *data, size_t size)
 * @brief Set a data into buffer and lock the buffer, i.e. turn the buf into "read-only".
 * @param buf_t * buf Buffer to set read-only
 * @param char * data Data the buf_t will contain. It is legal if this parameter == NULL
 * @param size_t size Size of the buffer. If data == NULL this argument must be 0
 * @return err_t EOK on success
 * 	Returns -ECANCELED if data == NULL but size > 0
 * 	Returns -EACCESS if this buffer already marked as read-only.
 */
extern ret_t buf_set_data_ro(/*@temp@*//*@in@*//*@special@*/buf_t *buf, /*@null@*//*@only@*//*@in@*/char *data, buf_s64_t size);

/**
 * @author Sebastian Mountaniol (01/06/2020)
 * @func void* buf_data_steal(buf_t *buf)
 * @brief 'Steal' data from buffer. After this operation the internal buffer 'data' returned to
 *    caller. After this function buf->data set to NULL, buf->len = 0, buf->size = 0
 * @param buf_t * buf Buffer to extract data buffer
 * @return void* Data buffer pointer on success, NULL on error. Warning: if the but_t did not have a
 * 	buffer (i.e. buf->data was NULL) the NULL will be returned.
 */
extern /*@null@*//*@only@*/void *buf_data_steal(/*@temp@*//*@in@*//*@special@*/buf_t *buf);

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
extern ret_t buf_clean_and_reset(/*@temp@*//*@in@*//*@special@*/buf_t *buf);

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
extern ret_t buf_room_add_memory(/*@temp@*//*@in@*//*@special@*/buf_t *buf, buf_s64_t size);

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
extern ret_t buf_room_assure(/*@temp@*//*@in@*/buf_t *buf, buf_s64_t expect);

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
extern ret_t buf_free(/*@only@*//*@in@*//*@special@*/buf_t *buf);

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
extern ret_t buf_add(/*@temp@*//*@in@*//*@special@*/buf_t *buf, /*@temp@*//*@in@*/const char *new_data, const buf_s64_t size);

/**
 * @author Sebastian Mountaniol (14/06/2020)
 * @func buf_usize_t buf_used_take(buf_t *buf)
 * @brief Return size in bytes of used memory (which is buf->used)
 * @param buf_t * buf Buffer to check
 * @return ssize_t Number of bytes used on success
 * 	EINVAL if the 'buf' == NULL
 */
extern buf_s64_t buf_used_take(/*@temp@*//*@in@*/buf_t *buf);

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
extern void buf_used_dec(/*@temp@*//*@in@*/buf_t *buf, buf_s64_t dec);

/**
 * @author Sebastian Mountaniol (14/06/2020)
 * @brief Return size of memory currently allocated for this 'buf' (which is buf->room)
 * @param buf_t * buf Buffer to test
 * @return ssize_t How many bytes allocated for this 'buf'
 * 	EINVAL if the 'buf' == NULL
 */
extern buf_s64_t buf_room_take(/*@temp@*//*@in@*/buf_t *buf);

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
extern void buf_room_inc(/*@temp@*//*@in@*/buf_t *buf, buf_s64_t inc);

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
extern void buf_room_dec(/*@temp@*//*@in@*/buf_t *buf, buf_s64_t dec);

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
extern ret_t buf_pack(/*@temp@*//*@in@*//*@special@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func buf_t_flags_t buf_save_flags(void)
 * @brief Return current value of gloabs buf_t flags (the flags added to every new allocate buf_t)
 * @param void
 * @return buf_t_flags_t - value of global buf_t flags
 * @details
 */
extern buf_t_flags_t buf_save_flags(void);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func void buf_restore_flags(buf_t_flags_t flags)
 * @brief Restore global buf_t flags
 * @param buf_t_flags_t flags - flags to set
 *  @details See buf_save_flags()
 */
extern void buf_restore_flags(buf_t_flags_t flags);

/**** Mark / Unmark flags */

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_mark_string(buf_t *buf)
 * @brief Mark (set flag) the buf as a
 * buffer containing string
 * @param buf_t * buf Buf to mark
 * @return err_t OK on success, EINVAL if
 * 	buf in NULL
 */
/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_mark_string(buf_t *buf)
 * @brief Mark (set flag) the buf as a buffer containing string buffer
 * data @param buf_t * buf Buffer to mark
 * @return err_t EOK on success, -EINVAL if buf is NULL
 */
extern ret_t buf_mark_string(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (12/17/21)
 * ret_t buf_set_flag(buf_t *buf, buf_t_flags_t f)
 * 
 * @brief Set the buffer flags 
 * @param buf - Buffer to set flags
 * @param f - Flag(s) to set
 *
 * @return ret_t OK on success, -EINVAL if NULL pointer received
 * @details 
 */
extern ret_t buf_set_flag(/*@temp@*//*@in@*/buf_t *buf, buf_t_flags_t f);

/**
 * @author Sebastian Mountaniol (12/17/21)
 * ret_t buf_rm_flag(buf_t *buf, buf_t_flags_t f)
 * 
 * @brief Unset a flag (or flags) from the buffer flags
 * @param buf - Buffer to unset flag(s)
 * @param f - Flag(s) to unset
 * 
 * @return ret_t 
 * @details 
 */
extern ret_t buf_rm_flag(/*@temp@*//*@in@*/buf_t *buf, buf_t_flags_t f);
/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_mark_ro(buf_t *buf)
 * @brief Mark (set flag) the buf as a buffer containing read-only data
 * @param buf_t * buf Buffer to mark
 * @return err_t EOK on success, -EINVAL if buf is NULL
 */
extern ret_t buf_mark_ro(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_mark_compresed(buf_t *buf)
 * @brief Mark (set flag) the buf as a buffer containing compressed data
 * @param buf_t * buf Buffer to mark
 * @return err_t EOK on success, EINVAL if buf is NULL
 */
extern ret_t buf_mark_compresed(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_mark_encrypted(buf_t *buf)
 * @brief Mark (set flag) the buf as a buffer containing encrypted data
 * @param buf_t * buf Buffer to mark
 * @return err_t EOK on success, EINVAL if buf is NULL
 */
extern ret_t buf_mark_encrypted(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_mark_canary(buf_t *buf)
 * @brief Mark (set flag) that the buf has canary word in the end of the buffer
 * @param buf_t * buf Buffer to mark
 * @return err_t EOK on success, EINVAL if buf is NULL
 */
extern ret_t buf_mark_canary(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_mark_canary(buf_t *buf)
 * @brief Mark (set flag) that the buf has CRC word in the end of the buffer
 * @param buf_t * buf Buffer to mark
 * @return err_t EOK on success, EINVAL if buf is NULL
 */
extern ret_t buf_mark_crc(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_unmark_string(buf_t *buf)
 * @brief Remove "string" mark (unset flag) from the buf
 * @param buf_t * buf Buffer to unmark
 * @return err_t OK on success, EINVAL if buf is NULL
 */
extern ret_t buf_unmark_string(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_unmark_ro(buf_t *buf)
 * @brief Remove "read-only" mark (unset flag) from the buf
 * @param buf_t * buf Buffer to unmark
 * @return err_t OK on success, EINVAL if buf is NULL
 */
extern ret_t buf_unmark_ro(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_unmark_compresed(buf_t *buf)
 * @brief Remove "compressed" mark (unset flag) from the buf
 * @param buf_t * buf Buffer to unmark
 * @return err_t OK on success, EINVAL if buf is NULL
 */
extern ret_t buf_unmark_compressed(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_unmark_encrypted(buf_t *buf)
 * @brief Remove "encryptes" mark (unset flag) from the buf
 * @param buf_t * buf Buffer to unmark
 * @return err_t OK on success, EINVAL if buf is NULL
 */
extern ret_t buf_unmark_encrypted(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_unmark_canary(buf_t *buf)
 * @brief Remove "canary" mark (unset flag) from the buf
 * @param buf_t * buf Buffer to unmark
 * @return err_t OK on success, EINVAL if buf is NULL
 */
extern ret_t buf_unmark_canary(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_unmark_crc(buf_t *buf)
 * @brief Remove "crc" mark (unset flag) from the buf
 * @param buf_t * buf Buffer to unmark
 * @return err_t OK on success, EINVAL if buf is NULL
 */
extern ret_t buf_unmark_crc(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_set_canary(buf_t *buf)
 * @brief Set the CANARY mark in the end of the buffer. The
 *  	  buf_t must be marked as CANARY, else an error
 *  	  returned.
 * @param buf_t * buf Buffer to set CANARy pattern
 * @return err_t EOK on success,
 * 	EINVAL if buf is NULL,
 *  ECANCELED if the buf does not have CANARY or if the CANARY
 *  pattern is not valid right after it set
 * @details If the buf doesn't have CANARY flag it will return ECANCELED.
 */
extern ret_t buf_set_canary(/*@temp@*//*@in@*//*@special@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_force_canary(buf_t *buf)
 * @brief Set CAANRY mark in the end of the buffer and apply ANARY flag on the buffer
 * @param buf_t * buf Buffer to set CANARY
 * @return err_t EOK on success,
 * 	-EINVAL if buf is NULL,
 * 	-ECANCELED if the buffer is too small to set CANARY mark,
 * 	or one of the but_set_canary() function errors
 * @details Pay attention, the buffer will be decreased by 1 byte, and the last byte of the buffer
 *    will be replaced with CANARY mark. You must reserve the last byte for it.
 */
extern ret_t buf_force_canary(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func err_t buf_test_canary(buf_t *buf)
 * @brief Check that CANARY mark is untouched
 * @param buf_t * buf Buffer to check
 * @return err_t EOK if CANARY word is untouched,
 * 	-EINVAL if the buf is NULL,
 * 	-ECANCELED if buf is not marked as a CANARY buffer,
 * 	EBAD if canary mark is invalid
 */
extern ret_t buf_test_canary(/*@temp@*//*@in@*//*@special@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func buf_t_canary_t buf_get_canary(buf_t *buf)
 * @brief Return current value of CANARY byte
 * @param buf_t * buf Buffer to read CANARY
 * @return buf_t_canary_t Value of CANARY byte, ((buf_t_canary_t)-1) on error
 * @details You may want to use this function if CANARY is damaged
 */
extern buf_t_canary_t buf_get_canary(/*@temp@*//*@in@*//*@special@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func void buf_print_flags(buf_t *buf)
 * @brief Print flags of the buffer. Useful for debug
 * @param buf_t * buf Buf to read flags
 */
extern void buf_print_flags(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func int buf_is_string(buf_t *buf)
 * @brief Test if the buffer is a string buffer
 * @param buf_t * buf Buffer to check
 * @return int EOK if the buf a string buffer
 * 	Returns -EINVAL if buf is NULL
 * 	Returns 1 if not
 */
extern int buf_is_string(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (12/16/21)
 * int buf_is_bit(buf_t *buf)
 * 
 * @brief Test if the buffer is a bit buffer
 * @param buf - Buffer to test
 * 
 * @return int OK if it is a bit buffer, -EINVAL if the buffer
 *  	   is NULL, 1 if the buffer is not a bit buffer
 * @details 
 */
extern int buf_is_bit(/*@temp@*//*@in@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (12/16/21)
 * int buf_is_circ(buf_t *buf)
 * 
 * @brief Test if the buffer is a circular buffer
 * @param buf - Buffer to test
 * 
 * @return int - OK  if the buffer is a circular buffer, 1 if
 *  	   not, -EINVAL if the buffer id NULL pointer
 * @details 
 */
extern int buf_is_circ(/*@temp@*//*@in@*/buf_t *buf);
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
extern ret_t buf_detect_used(/*@temp@*//*@in@*//*@special@*/buf_t *buf);

/**
 * @author Sebastian Mountaniol (18/06/2020)
 * @func size_t buf_recv(buf_t *buf, const int socket, const
 *  	 size_t expected, const int flags)
 * @brief Receive from socket into buffer
 * @param buf_t * buf Buffer to save received data
 * @param const int socket Opened socket
 * @param const size_t expected How many bytes expected
 * @param const int flags Flags to pass to recv() function
 * @return ssize_t Number of received bytes
 * 	EINVAL if buf is NULL, else returns status of recv() function
 */
extern size_t buf_recv(/*@temp@*//*@in@*//*@special@*/buf_t *buf, const int socket, const buf_s64_t expected, const int flags);

/* Additional defines */
#ifdef BUF_DEBUG

	#define BUF_TEST(buf) do {if (0 != buf_is_valid(buf)){fprintf(stderr, "######>>> Buffer invalid here: func: %s file: %s + %d [allocated here: %s +%d %s()]\n", __func__, __FILE__, __LINE__, buf->filename, buf->line, buf->func);}} while (0)
	#define BUF_DUMP(buf) do {DD("[BUFDUMP]: [%s +%d] buf = %p, data = %p, room = %u, used = %u [allocated here: %s +%d %s()]\n", __func__, __LINE__, buf, buf->data, buf->room, buf->used, buf->filename, buf->line, buf->func);} while(0)
	#define BUF_DUMP_ERR(buf) do {DD("[BUFDUMP]: [%s +%d] buf = %p, data = %p, room = %u, used = %u [allocated here: %s +%d %s()]\n", __func__, __LINE__, buf, buf->data, buf->room, buf->used, buf->filename, buf->line, buf->func);} while(0)
#else
	#define BUF_TEST(buf) do {if (0 != buf_is_valid(buf)){fprintf(stderr, "######>>> Buffer test invalid here: func: %s file: %s + %d\n", __func__, __FILE__, __LINE__);}} while (0)
	#define BUF_DUMP(buf) do {DD("[BUFDUMP]: [%s +%d] buf = %p, data = %p, room = %u, used = %u\n", __func__, __LINE__, buf, buf->data, buf->room, buf->used);} while(0)
	#define BUF_DUMP_ERR(buf) do {DD("[BUFDUMP]: [%s +%d] buf = %p, data = %p, room = %u, used = %u\n", __func__, __LINE__, buf, buf->data, buf->room, buf->used);} while(0)

#endif

#ifndef BUF_NOISY
	#undef BUF_DUMP
	#define BUF_DUMP(buf) do{}while(0)
#endif

#endif /* _BUF_T_H_ */
