#ifndef LIST_H_
#define LIST_H_

#include <stdlib.h>
#include <string.h>
#include "codes.h"

#ifndef MIN
	#define MIN(a,b) ((a < b) ? a : b)
#endif

struct EXA_list_node_structure;

typedef struct EXA_list_node_structure {
	struct EXA_list_node_structure *next; /**< The next member */
	size_t size; /**< Size of the copied buffer; can be 0 if the node keeps a pointer */
	void *data; /**< Pointer to a data or to a pointer */
} EXA_list_node_t;

typedef struct list_control_struct {
	EXA_list_node_t *root; /**< Root of the linked list */
	EXA_list_node_t *producer; /**< Pointer to producer's node */
	EXA_list_node_t *consumer; /**< Pointer to consumer's node */
	size_t increase_rate; /**< How many nodes to allocate when there's no space in the list */
	size_t size_limit; /**< What is the maximum size of the linked list, how many members */
	size_t current_size; /**< The size of the list, how many members */
	size_t buffer_size; /**< The buffersize of a node, used in node allocation */
	void *priv; /**< Additional pointer, for user's needs */
} EXA_list_ctl_t;

/**
 * Cast a pointer to list type pointer
 */
#define V2L(lst) ((EXA_list_ctl_t *)lst)

/**
 * @author Sebastian Mountaniol (2/23/22)
 * @brief Allocate and init lockless linked list
 * @param size_t increase_rate How many members to add into the
 *  			 linked list if the list is full. See details.
 * @param size_t size_limit   The upper limit of the linked list
 *  			 size. See details.
 * @param size_t buffer_size  Buffer size in bytes, allocated
 *  			 with every new node. See details.
 * @return list_ctl_t* Pointer to a newly created linked list;
 *  	   NULL on error
 * @details The linked list init depends on parameters:<br>
 * If 'increase_rate' > 0, this numbers of the new modes will
  be allocated when the list is full.<br>
 * - If 'increase_rate' == 0, this linked list is fixed, and no
 * new allocation will happen. In this case, on the list init 
 * 'size_limit' nodes are allocated and inserted into the
 * list.<br>
 * - If 'size_limit' > 0, it is the maximal number of members in
 * the linked list. When this number of nodes allocated, no
 * further grow is allowed, and all inserts will be dropped
 * with error status.<br>
 * - If 'size_limit' == 0, the list has no upper limit, and can
 *   grow infinitely.<br>
 * - Both 'size_limit' == 0 AND 'increase_rate' == 0 is illigal.
 * The function will reject it, no new list will be created,
 * NULL returned.<br>
 * - The  'buffer_size' is a max size of one node buffer, in
 * bytes. buffer_size == 0 is a legal value; in this case the
 * linked list can be used only for pointers, see
 * ::list_insert() for details.<br>
 * <br>
 * In a normal situation, when both the 'increase_rate' and
 * 'size_limit' values are given and > 0, the freshly allocated
 * list will not contain
 * nodes.<br>
 * On the first ::list_insert() call the 'increase_rate' will be
 * added.<br>
 * When the linked list is depleted, the new portion of
 * 'increase_rate' nodes is added. It happens until 'size_limit'
 * nodes allocated. When this maximal number of nodes is
 * allocated, ::list_insert() will start drop new
 * buffers/pointers.
 */
EXA_list_ctl_t *EXA_list_init(size_t increase_rate, size_t size_limit, size_t buffer_size);

/**
 * @author Sebastian Mountaniol (2/24/22)
 * @brief Finish and release the list 
 * @param list_ctl_t* lst   List to release
 * @details All allocated nodes will be released. After that,
 *  		the list structure will be released. If the list
 *  		contains pointers, all these buffers will be lost
 *  		and leaked. So the user must empty the list before
 *  		release it.
 */
void EXA_list_release(EXA_list_ctl_t *lst);

/**
 * @author Sebastian Mountaniol (2/23/22)
 * @brief Extract poiner or buffer from the list
 * @param list_ctl_t* lst   Poiner to the list control struct
 * @param void* buf   Poiner to buffer, in case a buffer must be
 *  		  extracted; ignored in case of pointer extraction
 * @param size_t* size  In case of buffer the size of buffer
 *  			will be returned in this variable
 * @return void* Returns: In case of pointer extraction the
 *  	   pointer returned on success, and the 'buf' and 'size'
 *  	   ignored. In case of a buffer: the pointer to 'buf'
 *  	   returned, the 'size' contains the buffer size. In
 *  	   both cases NULL returned as an error status.
 * @details 
 */
void *EXA_list_extract(EXA_list_ctl_t *lst, void *buf, size_t *size);

/* This is a simple wrapper of the list_extract() : extract a pointer,
   if you know that there is a pointer */
#define EXA_list_extract_pointer(lst) EXA_list_extract(lst, NULL, NULL)

/**
 * @author Sebastian Mountaniol (2/23/22)
 * @brief Insert a new pointer or a buffer into lined list
 * @param list_ctl_t* lst   THe list to insert pointer / buffer
 * @param void* data  Pointer to insert / Buffer to copy into
 * @param size_t size  Size of the buffer; if 0, the 'buf' is
 *  			 pointer
 * @return ret_code_t OK on success; EXA_ERROR if the list is
 *  	   full and can not be extended, because reached the
 *  	   'size_limit' number of members; Also EXA_ERROR on
 *  	   another error, for example either 'data' or 'lst' is
 *  	   NULL pointer
 * @details This is twofold purpose linked list:<br>
 * It can keep user's pointers without coping them;<br>
 * in this case on 'extract' the user returned just a
 * pointer.<br>
 * Or, it can copy a buffer from the user<br>
 * to a node on 'insert', and copy it back
 * to a user's buffer on 'extract'. <br>
 * The trigger between 'pointer' and 'buffer' is 'size':<br>
 * if the 'size' == 0, the 'data' is a pointer.<br>
 * If the 'size' > 0, the 'data' is a buffer<br>
 * and must be copied.<br>
 */
ret_code_t EXA_list_insert(EXA_list_ctl_t *lst, void *data, size_t size);


void *EXA_list_producer_buffer_get(EXA_list_ctl_t *lst, size_t *size);
ret_code_t EXA_list_producer_advance(EXA_list_ctl_t *lst, size_t size);
void *EXA_list_consumer_buffer_get(EXA_list_ctl_t *lst, size_t *size);

ret_code_t EXA_list_consumer_advance(EXA_list_ctl_t *lst);

/**
 * @author Sebastian Mountaniol (3/14/22)
 * @brief Count how many members can be extracted from the list
 * @param list_ctl_t* lst   List to count
 * @return size_t How many members can be extracted
 * @details Thsi function just runs over the list and counts how
 *  		mant buffers and pointers are available for
 *  		extraction.
 */
size_t EXA_list_count(EXA_list_ctl_t *lst);

#define EXA_list_insert_pointer(lst, pnt)  EXA_list_insert(lst, pnt, 0)

/**
 * @author Sebastian Mountaniol (2/23/22)
 * @brief Get size of the current consumer's buffer
 * @param list_ctl_t* lst   List to test
 * @return ssize_t Size of the node wich will be returned on the
 *  	   next 'extract': 0 or more. In case of error -1
 *  	   returned.
 * @details If the user wants to know what is the size of the
 *  		next buffer, which will be returned from 'extract'
 *  		function, he/she can get the size using this
 *  		function. This function only returns the size,
 *  		without advancing the pointer.
 */

#define EXA_list_current_consumer_size(l) ( l ? (ssize_t) lst->consumer->size : (ssize_t) -1 )

/**
 * @author Sebastian Mountaniol (2/23/22)
 * @brief Test either the list is empty, means - nothing to
 *  	  extract
 * @param list_ctl_t* lst   List to test
 * @return int YES (True) if the list is empty, NO (False) if
 *  	   the list is not empty
 * @details The YES and NO defined in this header file and YES =
 *  		1, NO = 0
 */
#define EXA_is_list_empty(lst) (V2L(lst)->consumer == V2L(lst)->producer)

/**
 * @brief Test either the list contains exactly one node
 * @param list_ctl_t* lst   List to test
 * @return True if the list contains exactly one node
 * @details We need it in FIFO, when FIFO decide either it
 *  		should produce a signal.
 */
//#define is_list_contains_one_node(lst) ((NO == is_list_empty(((list_ctl_t*)lst))) && ( ((list_ctl_t *)lst)->consumer->next == ((list_ctl_t *)lst)->producer))
//#define is_list_contains_one_node(lst) ((NO == is_list_empty(V2L(lst))) && ( (V2L(lst))->consumer->next == (V2L(lst))->producer))
//#define is_list_contains_one_node(lst) ((V2L(lst))->current_size > 1 && ( (V2L(lst))->consumer->next == (V2L(lst))->producer))
#define EXA_is_list_contains_one_node(lst) ((V2L(lst))->consumer->next == (V2L(lst))->producer)

/**
 * @author Sebastian Mountaniol (2/24/22)
 * @brief Test either the list is full, means list expansion
 *  	  needed, if possible
 * @param list_ctl_t* lst   List to test
 */
#define EXA_is_list_full(lst) (lst->producer->next == lst->consumer)

/**
 * @author Sebastian Mountaniol (2/23/22)
 * @brief Test if another node can be added into the list
 * @param list_ctl_t* lst   List to test
 * @return int Returns True (YES) if the list is out of
 *  	   capacity, False (NO) otherwise
 * @details The 'out of capacity' means 'nothing can be added';
 *  		in case of lst->size_limit is set to 0, the list
 *  		size is unlimited and can grow infinite
 */
#define EXA_is_list_out_of_capacity(lst) ((lst->size_limit > 0) && (lst->current_size == lst->size_limit) && EXA_is_list_full(lst))

/* A simple test that returns YES if the you will extract a pointer, or NO if you will extract a buffer on the next 'list_extract' call */
#define EXA_is_next_pointer(lst) ((lst->consumer->size) ? NO : YES)

/* A simple test that returns YES if the you will extract a buffer, or NO if you will extract a pointer on the next 'list_extract' call */
#define EXA_is_next_buffer(lst) ((lst->consumer->size) ? YES : NO)

#endif /* LIST_H_ */
