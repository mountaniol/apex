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
 * The Multi-Box structure holds zero or more boxes.
 * Every box contains a memory buffer.
 *
 * --------------------------
 * BOX 1
 * --------------------------
 * BOX 2
 * --------------------------
 * ...
 * --------------------------
 * BOX N
 * --------------------------
 *
 *
 * This memory buffer in a box can be growed automatically.
 * For example, when user adds another buffer in the tail of the buffer in the box.
 *
 * The idea of the Multi-Box is very simple.
 * Think about it as of collection of memory buffers.
 * You build this collection of different elements.
 * Then, you can send this Multi-Box object to another host.
 * Or you can merge all memory from all boxes into one contignoius buffer.
 *
 * WARNING: All operations will copy buffers into Mbox. Always.
 * You can not "set" a buffer into 
 *
 */

typedef struct {
	buf_t **bufs; /**< Array of buf_t structs */
	buf_s64_t bufs_num; /**< Number of bufs in the array */
} mbox_t;

typedef struct {
	uint32_t box_size; /** The< size of the box, not include ::box_size and ::box_checksum fields */
	uint32_t box_checksum; /**< The checksum of box buffer, means ::box_dump field; This field is optional, and ignored if == 0 */
	int8_t box_dump[]; /**< The dump of a box */
} box_dump_t;

typedef struct {
	uint32_t total_len; /**< Total length of this buffer, including 'total_len' field */
	uint32_t boxes_num; /**< Number of Boxed in this buffer */
	box_dump_t boxes_arr[]; /**< Array of box dumps: the size of the arrays is boxes_num */
} mbox_send_header_t;

/*** Mbuf create / release ***/

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Allocate a new mbox_t object with "number_of_boxes" boxes
 * @param uint32_t number_of_boxes - Number of boxes to allocate;
 * 		you do not have to do it, boxes will be allocated dynamically
 * @return ret_t Pointer to a new Mbox on success, NULL on error
 * @details 
 */
extern mbox_t *mbox_new(uint32_t number_of_boxes);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Release the mbox_t object and all boxes it holds
 * @param mbox_t* mbox  The mbox object to release
 * @return ret_t OK on success, other (negative) value on
 *  	   failure
 * @details This call will release everything
 */
extern ret_t mbox_free(mbox_t *mbox);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Remove all boxes from the mbox_t object
 * @param mbox_t* mbox  The mbox object to remove boxes from
 * @return ret_t Return OK on success, other (negative) value on
 *  	   an error
 * @details After this operation the mbox_t object will contain
 *  		0 boxes. All memory of the boxes will be released.
 */
extern ret_t mbox_clean(mbox_t *mbox);


/*** Mbox boxes manipulations ***/

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Add additional "num" of box(es)
 * @param mbox_t* mbox  The mbox object to add new mbox(s)
 * @param uint32_t *num Number of mboxes to add, must be > 0
 * @return ret_t OK on success
 * @details Add one or more mboxes.
 */
extern ret_t mbox_boxes_add(mbox_t *mbox, size_t num);

/**
 * @author Sebastian Mountaniol (7/11/22)
 * @brief Return number of boxes this mbox object holds
 * @param mbox_t* mbox  Mbox object to count boxes
 * @return ssize_t Number of boxes, 0 or more; A negative value
 *  	   is an error
 * @details 
 */
extern ssize_t mbox_boxes_count(mbox_t *mbox);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Insert a new box after the box "num"
 * @param mbox_t* mbox  The Mbox object to insert the new box
 * @param uint32_t* after The number of mbox after which the new
 *  			  mbox should be inserted. WARNING: The boxes
 *  			  after the box 'after' will be shifted and
 *  			  change their numbers. WARNING: The first box
 *  			  is '0'
 * @param uint32_t* num How many mboxes to insert
 * @return ret_t OK on success
 * @details If you have mbox with 5 boxes, and you insert one
 *  		new box after the existing box 3, then box 4 becomes
 *  		box 5, and the box 5 becomes box 6; It means, this
 *  		operation shifts all boxes after the box 'after'. 
 */
extern ret_t mbox_boxes_insert_after(mbox_t *mbox, uint32_t after, uint32_t num);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Swap two boxes
 * @param mbox_t* mbox  The Mbox object to swap boxes
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
extern ret_t mbox_boxes_swap(mbox_t *mbox, size_t first, size_t second);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Remove a box. All boxes stay untouched.
 * @param mbox_t* mbox  The Mbox object to remove a box from
 * @param uint32_t num   Number of box to remove
 * @return ret_t OK on success
 * @details If you have 5 boxes in the mbox, and you remove box
 *  		number 3, the box 4 stays box 4, the box 5 stays
 *  		box 5. The box 3 after this operation will be an
 *  		empty box.
 */
extern ret_t mbox_box_remove(mbox_t *mbox, size_t num);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Merge two boxes into one single box
 * @param mbox_t* mbox  The Mbox object to merge boxes 
 * @param size_t src   Add this "src" box at the tail of "dst"
 *  			   box; then, the "src" box becomes an empty
 *  			   box.
 * @param size_t dst   The destination box, the "src" box will
 *  			   be added at the tail of this box
 * @return ret_t OK on success
 * @details If you have a Mbox with 5 boxes, and you merge the
 *  		box 2 and 3, after this operation box 2 will contain
 *  		memory of (2 + 3), the box 3 will be an empty box,
 *  		box 4 and 5 will stay boxes 4 and 5
 */
extern ret_t mbox_boxes_merge(mbox_t *mbox, size_t src, size_t dst);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Bisect (cut) a mbox in point "offset" and create
 *  	  two mboxes from this one. The new mbox will be
 *  	  inserted after the bisected box. WARNING: The boxes
 *  	  after the bisected box are shifted.
 * @param mbox_t* mbox   The mbox object holding the mbox to
 *  			be bisected
 * @param size_t box_num Box to bisect
 * @param size_t from_offset  The offset in bytes of bisection
 *  			   point.
 * @return ret_t OK in case of success
 * @details You have a mbox with 5 mboxes. The mbox 3 is
 *  		256 bytes long. You want to bisect (cut) the mbox
 *  		3 this way: from byte 0 to byte 128 it stays mbox
 *  		3, but the byte 129 to the byte 256 will become a
 *  		new mbox. You call this function as:
 *  		mbox_mbox_bisect(mbox, 3, 128). After this
 *  		function finished: The mbox 3 contains bytes
 *  		0-128, a new mbox inserted after mbox 3,
 *  		containing bytes 129-256; the previous mbox 4
 *  		becomes mbox 5; the previous mbox 5 becomes
 *  		mbox 6. TODO: An diagramm should be added to
 *  		explain it visually.
 */
extern ret_t mbox_box_bisect(mbox_t *mbox, size_t box_num, size_t from_offset);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Unite (merge) all boxes into one continugeous box
 * @param mbox_t* mbox  The mbox object to collapse sectors.
 *  			WARNING: This operation is irreversable. You can
 *  			not restore boxes when them collapsed. If you
 *  			need a restorable version, use
 *  			::mbox_prepare_for_sending()
 * @return ret_t OK on success
 * @details After this operation only one box remains, which
 *  		includes merged memory of all other boxes. The boxes
 *  		are united one by one sequentionally, i.e., the
 *  		memory of sector 2 added in tail of sector one, then
 *  		added memory of secor 3 and so on. 
 */
extern ret_t mbox_boxes_collapse_in_place(mbox_t *mbox);

/**
 * @author Sebastian Mountaniol (6/16/22)
 * @brief Prepare the Mbox for sending to another host through a
 *  	  socket.
 * @param mbox_t* mbox  Mbox to prepare
 * @return buf_t* The buf_t structure contains the memory buffer
 *  	   to send, and the memory size. The buf_t->room will be
 *  	   equial to ->used, and this is the number of bytes for
 *  	   sending. This buffer can be sent to any other
 *  	   machine, and it will be restored on the other side.
 * @details This function creates restorable one big memory
 *  		buffer which you can send to another machine. You
 *  		can restore the exact same Mbox using
 *  		::mbox_from_buf() function.
 */
extern buf_t *mbox_to_buf(mbox_t *mbox);

/**
 * @author Sebastian Mountaniol (6/16/22)
 * @brief Restore the Mbox object from a buffer received (for
 *  	  example) by socket from another process / machine
 * @param buf_t* buf   The buf_t structure containing the
 *  		   received memory in buf_t->data, and the received
 *  		   buffer size in the buf_t->used.
 * @return mbox_t* Restored Mvox structure. All bixes are stored
 *  	   as they were before it transfered into the buf_t
 *  	   form.
 * @details 
 */
extern mbox_t *mbox_from_buf(buf_t *buf);

/*** A single sector operation - add, remove, modifu sector's memory ***/

/**
 * @author Sebastian Mountaniol (7/11/22)
 * @brief Create a new box and put the buffer into the box
 * @param mbox_t* mbox       Mbox to add buffer to
 * @param void* buffer     Buffer to add
 * @param size_t buffer_sizeSize of the buffer 
 * @return ret_t The number of created box on success, which is 0 or more.
 * A negative value on failure.
 * @details The new box will have sequentional number. If you already have 5 boxes,
 * the newely created box will have index "5," means it will be the sixts box.
 */
extern ssize_t mbox_box_new(mbox_t *mbox, void *buffer, size_t buffer_size);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Copy the memory buffer into the tail of the "box_num"
 *  	  internal buffer
 * @param mbox_t* mbox       	Mbox containing the box
 * @param size_t box_num    		Box number to add the buffer
 * @param void* buffer     		Buffer to copy
 * @param size_t buffer_size 	Buffer size to copy
 * @return ret_t OK on success, negative value on failure.
 * @details If the box 1 contains memory with the string "Blue
 *  		car", and you call mbox_box_add(mbox, 1, " and
 *  		yellow bike", strlen(" and yellow bike")), then
 *  		after this operation the box 1 will contain "Blue
 *  		car and yellow bike"
 */
extern ret_t mbox_box_add(mbox_t *mbox, size_t box_num, void *buffer, size_t buffer_size);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Remove everything from a box, and set a new memory
 *  	  buffer into it. It is the same as call to
 *  	  ::mbox_box_free_mem() and then to ::mbox_box_add()
 * @param mbox_t* mbox       Mbox object
 * @param size_t box_num        Box number to replace memory
 *  		  with the new buffer
 * @param void* buffer     The new buffer to set into the buffer
 * @param size_t buffer_size The size of the new buffer
 * @return ret_t OK on success.
 * @details 
 */
extern ret_t mbox_box_replace(mbox_t *mbox, size_t box_num, void *buffer, size_t buffer_size);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief This function returns a pointer to internal buffer of
 *  	  the box. You can use the internal buffer without
 *  	  copying it.
 * @param mbox_t* mbox   Mbox containing a box
 * @param size_t box_num Number of box you want to use
 * @return vois * - Pointer to internal buffer 
 * @details Use function ::mbox_box_size() to get size of this
 *  		buffer
 */
extern void *mbox_box_ptr(mbox_t *mbox, size_t box_num);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Returns size in bytes of the buffer in "box_num" box
 * @param mbox_t* mbox   Mbox object containing the asked box
 * @param size_t box_num Number of box to measure the memory
 *  			 size
 * @return ssize_t Size of the internal buffer, in bytes 
 * @details You probably need this function when use
 *  		::mbox_box_ptr() function
 */
extern ssize_t mbox_box_size(mbox_t *mbox, size_t box_num);

/**
 * @author Sebastian Mountaniol (7/12/22)
 * @brief Free memory in the given box. The box is not deleted,
 *  	  just becones empty.
 * @param mbox_t* mbox   Mbox object containing the box
 * @param size_t box_num Box number to free memory
 * @return ret_t OK on success, a negative error on failure
 * @details 
 */
extern ret_t mbox_box_free_mem(mbox_t *mbox, size_t box_num);

/**
 * @author Sebastian Mountaniol (7/14/22)
 * @brief "Steal" the memory buffer from a box
 * @param mbox_t* mbox   Mbox object 
 * @param size_t box_num    Number of box to steal memory buffer
 *  		  from
 * @return void* Memory buffer address. NULL if the box is empty
 *  	   or on an error
 * @details 
 */
extern void *mbox_box_steal_mem(mbox_t *mbox, size_t box_num);

#endif /* MBUF_H_ */
