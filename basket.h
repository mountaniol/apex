#ifndef MBUF_H_
#define MBUF_H_

/*
 * We use definitions and types from buf_t:
 * typedef int64_t buf_s64_t;
 * typedef int ret_t;
 */
#include <stdint.h>
#include "buf_t.h"

/*
 * The Basket structure holds zero or more boxes.
 * Every box contains a memory buffer.
 *
 * --------------------------
 * BOX 1 [ memory       ]
 * --------------------------
 * BOX 2 [ memory ]
 * --------------------------
 * ...
 * --------------------------
 * BOX N [ mem ] 
 * --------------------------
 *
 *
 * This memory buffer(s) in boxes are managed automatically.
 * For example, when user adds another buffer in the tail of the buffer in the box, the memory will be allocated.
 *
 * The idea of the Basket is simple.
 * Think about it as of collection of memory buffers, every buffer placed in its box.
 * You build this collection of different elements - structures, strings, whatever you need.
 * Then, you can send this Basket object to another host. Or save into a file.
 * Or you can merge all memory from all boxes into one contignoius buffer.
 *
 * The usage scenario defined by you. The Basket cares about the free/allocate/add memory/remove memory hassle.
 * Moreover, it can create for you a flat memory buffer from the Busket,
 * and later (or on another machine) you can restore the Basket from this memory buffer.
 *
 * WARNING: All operations will copy buffers into Busket. Always.
 * You can not "set" a buffer into a box.
 * Also, you never need to release buffers manually. When you finished with a Basket,
 * just release it, and all the memory in all boxes will be released.
 *
 */

#define WATERMARK_BASKET (0xBAFFA779)
#define WATERMARK_BOX (0xBAFFA773)

typedef struct {
	buf_t **bufs; /**< Array of buf_t structs */
	buf_s64_t bufs_num; /**< Number of bufs in the array */
} basket_t;

typedef struct {
	uint32_t watermark; /**< Watermark: filled with a predefined pattern WATERMARK_BOX */
	uint32_t box_size; /** The< size of the box, not include ::box_size and ::box_checksum fields */
	uint32_t box_checksum; /**< The checksum of box buffer, means ::box_dump field; This field is optional, and ignored if == 0 */
} __attribute__((packed)) box_dump_t;

typedef struct {
	uint32_t watermark; /**< Watermark: filled with a predefined pattern WATERMARK_BASKET */
	uint32_t total_len; /**< Total length of this buffer, including 'total_len' field */
	uint32_t boxes_num; /**< Number of Boxed in this buffer */
	uint32_t basket_checksum; /**< The checksum of box buffer, means ::box_dump field; This field is optional, and ignored if == 0 */
} __attribute__((packed)) basket_send_header_t;

/*** Mbuf create / release ***/

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Allocate a new basket_t object with "number_of_boxes" boxes
 * @param uint32_t number_of_boxes - Number of boxes to allocate;
 * 		you do not have to do it, boxes will be allocated dynamically
 * @return ret_t Pointer to a new Basket object on success, NULL on error
 * @details 
 */
extern basket_t *basket_new(const uint32_t number_of_boxes);

/**
 * @author Sebastian Mountaniol (7/15/22)
 * @brief Return total size of Mvox object in bytes
 * @param basket_t* basket  Basket to measure
 * @return size_t Size of the Basket object and all buffers in
 *  	   all boxes
 * @details 
 */
extern size_t basket_size(const basket_t *basket);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Release the basket_t object and all boxes it holds
 * @param basket_t* basket  The basket object to release
 * @return ret_t OK on success, other (negative) value on
 *  	   failure
 * @details This call will release everything
 */
extern ret_t basket_release(basket_t *basket);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Remove all boxes from the basket_t object
 * @param basket_t* basket  The basket object to remove boxes
 *  			  from
 * @return ret_t Return OK on success, other (negative) value on
 *  	   an error
 * @details After this operation the basket_t object will
 *  		contain 0 boxes. All memory of the boxes will be
 *  		released.
 */
extern ret_t basket_clean(basket_t *basket);

/*** Box(es) manipulations ***/

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Add additional "num" of box(es)
 * @param basket_t* basket  The basket object to add new basket(s)
 * @param uint32_t *num Number of boxes to add, must be > 0
 * @return ret_t OK on success
 * @details Add one or more boxes. If you want just save a buffer into a box, use ::box_new() function
 */
extern ret_t box_allocate(basket_t *basket, const size_t num);

/**
 * @author Sebastian Mountaniol (7/11/22)
 * @brief Return number of boxes this basket object holds
 * @param basket_t* basket  Basket object to count boxes
 * @return ssize_t Number of boxes, 0 or more; A negative value
 *  	   is an error
 * @details 
 */
extern ssize_t basket_boxes_count(const basket_t *basket);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Insert a new box after the box "num"
 * @param basket_t* basket  The Basket object to insert the new
 *  			  box
 * @param uint32_t* after The number of box after which the new
 *  			  box should be inserted. WARNING: The boxes
 *  			  after the box 'after' will be shifted and
 *  			  change their numbers. WARNING: The first box
 *  			  is '0'
 * @param uint32_t* num How many boxes to insert
 * @return ret_t OK on success
 * @details If you have a Basket with 5 boxes, and you insert
 *  		one new box after the existing box 3, then box 4
 *  		becomes box 5, and the box 5 becomes box 6; It
 *  		means, this operation shifts all boxes after the box
 *  		'after'.
 */
extern ret_t box_insert_after(basket_t *basket, const uint32_t after, const uint32_t num);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Swap two boxes
 * @param basket_t* basket  The Basket object to swap boxes
 * @param size_t first The first box, after operation it will
 *  			  become the second
 * @param size_t second The second box to swap, after this
 *  			  operation it will become the first
 * @return ret_t OK on success, another (negative) value on
 *  	   failure
 * @details Swap two boxes. For example, if you have 5 boxes and
 *  		run this function for boxes 2 and 4, the box 4 moved
 *  		to place of box 2, and the previous box 2 will
 *  		become box 4. All other boxes stay untouched.
 */
extern ret_t box_swap(basket_t *basket, const size_t first, const size_t second);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Remove a box. All boxes stay untouched.
 * @param basket_t* basket  The Basket object to remove a box
 *  			  from
 * @param uint32_t num   Number of box to remove
 * @return ret_t OK on success
 * @details If you have 5 boxes in the Basket, and you remove
 *  		box number 3, the box 4 stays box 4, the box 5 stays
 *  		box 5. The box 3 after this operation will be an
 *  		empty box.
 */
extern ret_t box_remove(basket_t *basket, const size_t num);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Merge two boxes into one single box
 * @param basket_t* basket  The Basket object to merge boxes 
 * @param size_t src   Add this "src" box at the tail of "dst"
 *  			   box; then, the "src" box becomes an empty
 *  			   box.
 * @param size_t dst   The destination box, the "src" box will
 *  			   be added at the tail of this box
 * @return ret_t OK on success
 * @details If you have a Basket with 5 boxes, and you merge the
 *  		box 2 and 3, after this operation box 2 will contain
 *  		memory of (2 + 3), the box 3 will be an empty box,
 *  		box 4 and 5 will stay boxes 4 and 5
 */
extern ret_t box_merge(basket_t *basket, const size_t src, const size_t dst);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Bisect (cut) a mbox in point "offset" and create two
 *  	  boxes from this one. The new box will be inserted
 *  	  after the bisected box. WARNING: The boxes after the
 *  	  bisected box are shifted.
 * @param basket_t* basket   The Basket object holding the box
 *  			to be bisected
 * @param size_t box_num Box to bisect
 * @param size_t from_offset  The offset in bytes of bisection
 *  			   point.
 * @return ret_t OK in case of success
 * @details You have a Basket with 5 mboxes. The box 3 is 256
 *  		bytes long. You want to bisect (cut) the box 3 this
 *  		way: from byte 0 to byte 128 it stays box 3, memory
 *  		from the byte 129 to the byte 256 will become a new
 *  		box. You call this function as: box_bisect(mbox, 3,
 *  		128). After this function: The box 3 contains bytes
 *  		0-128, a new box inserted after box 3, containing
 *  		bytes 129-256; the previous box 4 becomes box 5; the
 *  		previous box 5 becomes box 6. TODO: An diagramm
 *  		should be added to explain it visually.
 */
extern ret_t box_bisect(basket_t *basket, const size_t box_num, const size_t from_offset);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Unite (merge) all boxes into one continugeous box
 * @param basket_t* basket  The Basket object to collapse
 *  			all boxes into box[0]. WARNING: This operation
 *  			is irreversable. You can not restore boxes after
 *  			them collapsed. If you need a restorable
 *  			version, use ::basket_to_buf() or
 *  			::busket_to_buf_t()
 * @return ret_t OK on success
 * @details After this operation only one box with index 0
 *  		remains, which includes merged memory of all other
 *  		boxes. The boxes are united one by one
 *  		sequentionally, i.e., the memory of the box 1 added
 *  		in tail of the box 0, then added memory of the box
 *  		3 and so on.
 */
extern ret_t basket_collapse_in_place(basket_t *basket);

/**
 * @author Sebastian Mountaniol (6/16/22)
 * @brief Prepare the Basket for sending to another host through
 *  	  a socket.
 * @param basket_t* basket Basket to prepare
 * @return buf_t* The buf_t structure contains the memory buffer
 *  	   to send, and the memory size. The buf_t->room will be
 *  	   equial to buf_t->used, and this is the number of
 *  	   bytes for sending. This buffer can be sent to any
 *  	   other machine, and it will be restored on the other
 *  	   side.
 * @details This function creates a restorable memory buffer
 *  		which you can send to another machine. You can
 *  		restore the exact same Basket using
 *  		::basket_from_buf() function. The 'Basket' passed to
 *  		this function not changed and not released.
 */
void *basket_to_buf(const basket_t *basket, size_t *size);

/**
 * @author Sebastian Mountaniol (7/17/22)
 * @brief Create contignoius buffer containing restorable
 *  	  content of a basket; this basket object can be
 *  	  restored by function
 * @param basket_t* basket  
 * @return buf_t* 
 * @details 
 */
extern buf_t *basket_to_buf_t(basket_t *basket);

/**
 * @author Sebastian Mountaniol (7/17/22)
 * @brief Restore Basket object from the regular memory buffer.
 *  	  The buffer should be a result of function
 *  	  ::basket_to_buf() or basket_to_buf_t()
 * @param void* buf   The memory containing the buffer from
 *  		  which the Basket object will be restored.
 * @param size_t size  Size of the memory 
 * @return basket_t* The Basket object returned. On error NULL
 *  	   returned.
 * @details The 'buf' buffer is untouched and not released
 *  		disregarding the success or failure of this
 *  		operation. The caller is own the memory and the
 *  		caller should release it.
 */
extern basket_t *basket_from_buf(void *buf, const size_t size);

/**
 * @author Sebastian Mountaniol (6/16/22)
 * @brief Restore the Basket object from a buffer received (for
 *  	  example) by socket from another process / machine
 * @param buf_t* buf   The buf_t structure containing the
 *  		   received memory in buf_t->data, and the received
 *  		   buffer size in the buf_t->used.
 * @return basket_t* Restored Baasket structure. All boxes are
 *  	   restored as they were before it transfered into the
 *  	   buf_t form. Even boxes with 0 bytes memory are
 *  	   restored.
 * @details The buf_t is untouched, disregarding the operation
 *  		success or failure. The caller owns this memory.
 */
extern basket_t *basket_from_buf_t(const buf_t *buf);

/*** A single sector operation - add, remove, modifu sector's memory ***/

/**
 * @author Sebastian Mountaniol (7/11/22)
 * @brief Create a new box and put the buffer into the box
 * @param basket_t* basket       Basket to add buffer to
 * @param void* buffer     Buffer to add
 * @param size_t buffer_size Size of the buffer 
 * @return ret_t The number of created box on success, which is 0 or more.
 * A negative value on failure.
 * @details The new box will have sequentional number. If you already have 5 boxes,
 * the newely created box will have index "5," means it will be the sixts box. The boxes index starts from 0.
 */
extern ssize_t box_new_from_data(basket_t *basket, const void *buffer, const size_t buffer_size);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Copy the memory buffer into the tail of the "box_num"
 *  	  internal buffer
 * @param basket_t* basket       	Basket containing the box
 * @param size_t box_num    		Box number to add the buffer
 * @param void* buffer     		Buffer to copy
 * @param size_t buffer_size 	Buffer size to copy
 * @return ret_t OK on success, negative value on failure.
 * @details If the box 1 contains memory with the string "Blue
 *  		car", and you call box_add_to_tail(basket, 1, " and
 *  		yellow bike", strlen(" and yellow bike")), then
 *  		after this operation the box 1 will contain "Blue
 *  		car and yellow bike"
 */
extern ret_t box_add_to_tail(basket_t *basket, const buf_s64_t box_num, const void *buffer, const size_t buffer_size);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Remove everything from a box, and set a new memory
 *  	  buffer into it. It is the same as call to
 *  	  ::box_remove() and then to ::box_add_to_tale()
 * @param basket_t* basket       Basket object
 * @param size_t box_num        Box number to replace memory
 *  		  with the new buffer
 * @param void* buffer     The new buffer to set into the buffer
 * @param size_t buffer_size The size of the new buffer
 * @return ret_t OK on success.
 * @details THe memory is copied from the 'buffer' into the box.
 *  		The box will allocate enough memory to hold this
 *  		data. THe original 'buffer' is unouched and it is up
 *  		to caller to release it.
 */
extern ret_t box_replace_data(basket_t *basket, const size_t box_num, const void *buffer, const size_t buffer_size);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief This function returns a pointer to internal buffer of
 *  	  the box. You can use the internal buffer without
 *  	  copying it.
 * @param basket_t* basket   Basket containing a box
 * @param size_t box_num Number of box you want to use
 * @return vois * - Pointer to internal buffer 
 * @details Use function ::mbox_box_size() to get size of this
 *  		buffer. Please be very careful with this function.
 *  		WARNING: Do not release this memory! You do not own
 *  		it! If you do, the basket_release will fail.
 */
extern void *box_data_ptr(const basket_t *basket, const size_t box_num);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Returns size in bytes of the buffer in "box_num" box
 * @param basket_t* basket   Basket object containing the asked
 *  			  box
 * @param size_t box_num Number of box to measure the memory
 *  			 size
 * @return ssize_t Size of the internal buffer, in bytes 
 * @details You probably need this function when use
 *  		::box_data_ptr() function
 */
extern ssize_t box_data_size(const basket_t *basket, const size_t box_num);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Free memory in the given box. The box is not deleted,
 *  	  just becones empty.
 * @param basket_t* basket  Basket object containing the box
 * @param size_t box_num Box number to free memory
 * @return ret_t OK on success, a negative error on failure
 * @details 
 */
extern ret_t box_data_free(basket_t *basket, const size_t box_num);

/**
 * @author Sebastian Mountaniol (7/14/22)
 * @brief "Steal" the memory buffer from a box
 * @param basket_t* basket   Basket object 
 * @param size_t box_num    Number of box to steal memory buffer
 *  		  from
 * @return void* Memory buffer address. NULL if the box is empty
 *  	   or on an error
 * @details 
 */
extern void *box_steal_data(basket_t *basket, const size_t box_num);

#endif /* MBUF_H_ */