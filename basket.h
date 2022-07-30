#ifndef BASKET_H_
#define BASKET_H_

/*
 * We use definitions and types from buf_t:
 * typedef int64_t buf_s64_t;
 * typedef int ret_t;
 */
#include <stdint.h>
#include "box_t.h"

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

/**
 * @def
 * @details Used to mark basket structure when creating a flat mamory buffer from a basket
 */
#define WATERMARK_BASKET (0xBAFFA779)
/**
 * @def
 * @details Used to mark a box structure when dumped it into a
 *  		flat memory buffer
 */
#define WATERMARK_BOX (0xBAFFA773)

/**
 * @def
 * @details How many basket->bufs pointers allocated at once
 */
#define BASKET_BUFS_GROW_RATE (32)

typedef struct {
	void **boxes; /**< Array of buf_t structs */
	box_u32_t boxes_used; /**< Number of bufs in the array */
	box_u32_t boxes_allocated; /**< For internal use: how many buf_t pointers are allocated in the 'bufs' */
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

/*** Getter / Setter functions ***/
/* We populate these function for test purposes. Should not be used out of test */

/**
 * @author Sebastian Mountaniol (7/27/22)
 * @brief Get pointer to a box in the basket
 * @param const void* basket   Basket to get box from
 * @param const box_u32_t box_index Box index, the first box is '0' index
 * @return box_t* Pointer to a box, NULL on an error or if the box is not allocated yet
 * @details If NULL returned it is not an error. On a critical error this function will call abort().
 */
extern void *basket_get_box(const void *basket, const box_u32_t box_index);

/*** Basket create / release ***/

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Allocate a new basket_t object with "number_of_boxes" boxes
 * @return ret_t Pointer to a new Basket object on success, NULL on error
 * @details 
 */
extern void *basket_new(void);

/**
 * @author Sebastian Mountaniol (7/15/22)
 * @brief Return total size of Mvox object in bytes
 * @param const void *basket  Basket to measure
 * @return size_t Size of the Basket object and all buffers in
 *  	   all boxes
 * @details 
 */
extern size_t basket_size(const void *basket);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Release the basket_t object and all boxes it holds
 * @param void *basket  The basket object to release
 * @return ret_t OK on success, other (negative) value on
 *  	   failure
 * @details This call will release everything - the basket, all
 *  		boxes in the basket, and all buffers in  the boxes.
 *  		The whole basket object released completely.
 */
extern ret_t basket_release(void *basket);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Remove all boxes from the basket_t object
 * @param void* basket  The basket object to remove boxes
 *  			  from
 * @return ret_t Return OK on success, other (negative) value on
 *  	   an error
 * @details After this operation the basket_t object will
 *  		contain 0 boxes. All memory of the boxes will be
 *  		released.
 */
extern ret_t basket_clean(void *basket);

/*** Box(es) manipulations ***/

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Insert a new box after the box "after"
 * @param void* basket  The Basket object to insert the new
 *  			  box
 * @param uint32_t* after_index The index of box after which the new
 *  			  box should be inserted.<br>
 *  			  WARNING: The boxes
 *  			  after the box 'after' will be shifted and
 *  			  change their numbers.<br>
 *  			  WARNING: The index of a box starts from '0'
 * @param const void *buffer - Buffer to copy into the new box.<br>
 * 				  WARNING: The buffer is copied, it up to caller to release it after the operation.
 * @param const box_u32_t buffer_size - Size of buffer to copy into the new box
 * @return ret_t OK on success
 * @details If you have a Basket with 5 boxes, and you insert
 *  		a new box after the existing box 3, then box 4
 *  		becomes box 5, and the box 5 becomes box 6; It
 *  		means, this operation shifts all boxes after the box
 *  		'after'.
 */
extern ret_t box_insert_after(void *basket, const uint32_t after_index, const void *buffer, const box_u32_t buffer_size);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Swap two boxes
 * @param void* basket  The Basket object to swap boxes
 * @param buf_u32_t first The first box, after operation it will
 *  			  become the second
 * @param buf_u32_t second The second box to swap, after this
 *  			  operation it will become the first
 * @return ret_t OK on success, another (negative) value on
 *  	   failure
 * @details Swap two boxes. For example, if you have 5 boxes and
 *  		run this function for boxes 2 and 4, the box 4 moved
 *  		to place of box 2, and the previous box 2 will
 *  		become box 4. All other boxes stay untouched.
 */
extern ret_t box_swap(void *basket, const box_u32_t first_index, const box_u32_t second_index);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Remove content of one box by box index (starts from
 *  	  0). All other boxes stay untouched
 * @param void* basket  The Basket object to clean a box
 * @param uint32_t num   index of box to clean
 * @return ret_t OK on success
 * @details The box won't be removed, just cleaned: its internal
 *  		buffer removed, and its size will be 0 after this
 *  		operation. You may reuse this box if you wish.
 */
extern ret_t box_clean(void *basket, const box_u32_t box_index);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Merge two boxes into one single box
 * @param void* basket  The Basket object to merge boxes 
 * @param buf_u32_t src   Add this "src" box at the tail of
 *  			   "dst" box; then, the "src" box becomes an
 *  			   empty box.
 * @param buf_u32_t dst   The destination box, the "src" box
 *  			   will be added at the tail of this box
 * @return ret_t OK on success
 * @details If you have a Basket with 5 boxes, and you merge the
 *  		box 2 and 3, after this operation box 2 will contain
 *  		memory of (2 + 3), the box 3 will be an empty box,
 *  		box 4 and 5 will stay boxes 4 and 5
 */
extern ret_t box_merge_box(void *basket, const box_u32_t src, const box_u32_t dst);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Bisect (cut) a mbox in point "offset" and create two
 *  	  boxes from this one. The new box will be inserted
 *  	  after the bisected box. WARNING: The boxes after the
 *  	  bisected box are shifted.
 * @param void* basket   The Basket object holding the box
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
extern ret_t box_bisect(void *basket, const box_u32_t box_num, const size_t from_offset);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Unite (merge) all boxes into one continugeous box
 * @param void* basket  The Basket object to collapse
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
extern ret_t basket_collapse(void *basket);

/**
 * @author Sebastian Mountaniol (6/16/22)
 * @brief Prepare the Basket for sending to another host through
 *  	  a socket.
 * @param void* basket Basket to prepare
 * @return void* The memory buffer, packed, and ready to send.
 *  	   This buffer can be sent to any other machine, and it
 *  	   will be restored on the other side using
 *  	   ::basket_from_buf() function
 * @details This function creates a restorable memory buffer
 *  		which you can send to another machine. You can
 *  		restore the exact same Basket using
 *  		::basket_from_buf() function. The 'Basket' passed to
 *  		this function not changed and not released.
 */
extern void *basket_to_buf(const void *basket, size_t *size);

/**
 * @author Sebastian Mountaniol (7/17/22)
 * @brief Restore Basket object from the regular memory buffer.
 *  	  The buffer should be a result of function
 *  	  ::basket_to_buf().
 * @param void* buf   The memory containing the buffer from
 *  		  which the Basket object will be restored.
 * @param size_t size  Size of the memory passed as 'buf'
 *  			 pointer
 * @return void* The Basket object returned. NULL returned on an
 *  	   error.
 * @details The 'buf' buffer is untouched and not released
 *  		disregarding the success or failure of this
 *  		operation. The caller owns the memory and the caller
 *  		responsible to release it.
 */
extern void *basket_from_buf(void *buf, const size_t size);

/**
 * @author Sebastian Mountaniol (7/26/22)
 * @brief Compare tow baskets, including box data. 
 * @param void* basket_left The first basket
 * @param void* basket_right The second basket
 * @return int 0 if two baskets are equal, 1 if if they are
 *  	   differ from each other, -1 on an error
 * @details The baskets passed to this function are basket
 *  		objects, and not result of ::basket_to_buf()
 *  		function.
 */
extern int basket_compare_basket(const void *basket_right, const void *basket_left);

/*** A single sector operation - add, remove, modifu sector's memory ***/

/**
 * @author Sebastian Mountaniol (7/11/22)
 * @brief Create a new box and put the buffer into the box
 * @param void* basket       Basket to add a new box
 * @param void* buffer     	 Buffer to add into the new box
 * @param size_t buffer_size Size of the buffer to add to new box
 * @return ret_t The number of created box on success, which is 0 or more.
 * A negative value on failure.
 * @details The new box will have sequentional number. If you already have 5 boxes,
 * the newely created box will have index "5," means it will be the sixts box. The boxes index starts from 0.
 */
extern ssize_t box_new(void *basket, const void *buffer, const box_u32_t buffer_size);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Copy the memory buffer into the tail of the "box_num"
 *  	  internal buffer
 * @param void* basket       	Basket containing the box
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
extern ret_t box_add(void *basket, const box_u32_t box_num, const void *buffer, const size_t buffer_size);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Remove everything from a box, and set a new memory
 *  	  buffer into it. It is the same as call to
 *  	  ::box_remove() and then to ::box_add_to_tale()
 * @param void* basket       Basket object
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
extern ret_t box_data_replace(void *basket, const box_u32_t box_num, const void *buffer, const size_t buffer_size);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief This function returns a pointer to internal buffer of
 *  	  the box. You can use the internal buffer without
 *  	  copying it.
 * @param void* basket   Basket containing a box
 * @param size_t box_num Number of box you want to use
 * @return vois * - Pointer to internal buffer 
 * @details Use function ::mbox_box_size() to get size of this
 *  		buffer. Please be very careful with this function.
 *  		WARNING: Do not release this memory! You do not own
 *  		it! If you do, the basket_release will fail.
 */
extern void *box_data_ptr(const void *basket, const box_u32_t box_num);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Returns size in bytes of the buffer in "box_num" box
 * @param void* basket   Basket object containing the asked
 *  			  box
 * @param size_t box_num Number of box to measure the memory
 *  			 size
 * @return ssize_t Size of the internal buffer, in bytes. A
 *  	   negative value on an error.
 * @details You probably need this function when use
 *  		::box_data_ptr() function
 */
extern ssize_t box_data_size(const void *basket, const box_u32_t box_num);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Free memory in the given box. The box is not deleted,
 *  	  just becones empty.
 * @param void* basket  Basket object containing the box
 * @param size_t box_num Box number to free memory
 * @return ret_t OK on success, a negative error on failure
 * @details 
 */
extern ret_t box_data_free(void *basket, const box_u32_t box_num);

/**
 * @author Sebastian Mountaniol (7/14/22)
 * @brief "Steal" the memory buffer from a box
 * @param void* basket   Basket object 
 * @param size_t box_num    Number of box to steal memory buffer
 *  		  from
 * @return void* Memory buffer address. NULL if the box is empty
 *  	   or on an error
 * @details 
 */
extern void *box_steal_data(void *_basket, const box_u32_t box_num);

#endif /* BASKET_H_ */
