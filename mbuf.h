#ifndef MBUF_H_
#define MBUF_H_

/*
 * We use definitions and types from buf_t:
 * typedef int64_t buf_s64_t;
 * typedef int ret_t;
 */

#include "buf_t.h"

typedef struct {
	buf_t *bufs; /**< Array of buf_t structs */
	buf_s64_t bufs_num; /**< Number of bufs in the array */
} mbuf_t;

/*** Mbuf create / release ***/

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Allocate a new mbuf object with "segments" number of
 *  	  segments
 * @param uint32_t segments - Number of segments to allocate
 * @return ret_t OK in case of success, other value on failure
 * @details 
 */
extern ret_t mbuf_new(uint32_t segments);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Release the mbuf_t object and all segments it holds
 * @param mbuf_t* mbuf  The mbuf object to release
 * @return ret_t OK on success
 * @details This call will release everything
 */
extern ret_t mbuf_free(mbuf_t *mbuf);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Remove all segments from the mbuf object
 * @param mbuf_t* mbuf  The mbuf object to remove segments from
 * @return ret_t Return OK on success
 * @details After this operation the mbuf object will contain 0
 *  		segments. All memory of the segments will be
 *  		released.
 */
extern ret_t mbuf_clean(mbuf_t *mbuf);


/*** Mbuf segment manipulations ***/

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Add additional "num" of segment(s)
 * @param mbuf_t* mbuf  The mbuf object to add new segment(s)
 * @param uint32_t *num Number of segments to add, must be > 0
 * @return ret_t OK on success
 * @details Add one or more segments.
 */
extern ret_t mbuf_segments_add(mbuf_t *mbuf, uint32_t *num);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Insert a new segment after the segment "num"
 * @param mbuf_t* mbuf  The mbuf object to insert the new
 *  			segment
 * @param uint32_t* after The number of segment after which the
 *  			  new segment to be inserted
 * @param uint32_t* num How many segments to insert
 * @return ret_t OK on success
 * @details If you have mbuf with 5 segment, and you insert one
 *  		new segment after segment 3, then previous segment 4
 *  		will become segment 5, and the previous segment 5
 *  		will become segment 6
 */
extern ret_t mbuf_segments_insert_after(mbuf_t *mbuf, uint32_t *after, uint32_t num);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Swap two segments
 * @param mbuf_t* mbuf  The mbuf object to swap segments
 * @param uint32_t* first The first segment, after operation it
 *  			  will become the second
 * @param uint32_t* second The second segment to swap, after
 *  			  this operation it will become the first
 * @return ret_t OK on success
 * @details Swap two segments. For example, if you have 5
 *  		segments and run this function for segment 2 and 4,
 *  		the segments 4 moved to place of segment 2, and the
 *  		previous segment 2 will become segment 4
 */
extern ret_t mbuf_segments_swap(mbuf_t *mbuf, uint32_t *first, uint32_t *second);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Remove a segment; all segments after removed will be
 *  	  shifted to close the gap
 * @param mbuf_t* mbuf  The mbuf object to remove a segment from
 * @param uint32_t num   Number of segment to remove
 * @return ret_t OK on success
 * @details If you have 5 segments in  mbuf, and you remove
 *  		segment number 3, the segment 4 will become segment
 *  		3, and the segment 5 will become segment 4
 */
extern ret_t mbuf_segment_remove(mbuf_t *mbuf, uint32_t num);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Merge two segment into one single segment
 * @param mbuf_t* mbuf  The mbuf object to merge segments 
 * @param uint32_t src   Add this segment at the tail of "dst"
 *  			   segment; then, the "src" segment will be
 *  			   removed.
 * @param uint32_t dst   The destination segment, the "src"
 *  			   segment will be added at the tail of this
 *  			   segment
 * @return ret_t OK on success
 * @details If yoi have a mbbuf with 5 segments, and you merge
 *  		the segment 2 and 3, then after this operation
 *  		segment 2 will contain memory of (2 + 3), the
 *  		segment 3 will be removed, segment 4 and 5 will be
 *  		shifted and became segments 3 and 4
 */
extern ret_t mbuf_segments_merge(mbuf_t *mbuf, uint32_t src, uint32_t dst);

/**
 * @author Sebastian Mountaniol (6/12/22)
 * @brief Bisect (cut) a segment in point "offset" and create
 *  	  two segments from this one. The new segment will be
 *  	  inserted after bisecting segment.
 * @param mbuf_t* mbuf   The mbuf object holding the segment to
 *  			be bisected
 * @param uint32_t segment Segment to bisect
 * @param uint32_t offset  The offset in bytes of bisection
 *  			   point.
 * @return ret_t OK in case of success
 * @details You have a mbuf with 5 segments. The segment 3 is
 *  		256 bytes long. You want to bisect (cut) the segment
 *  		3 this way: from byte 0 to byte 128 it stays segment
 *  		3, but the byte 129 to the byte 256 will beocome a
 *  		new segment. You call this function as:
 *  		mbuf_segment_bisect(mbuf, 3, 128); After this
 *  		operation: The segment 3 contains bytes 0-128, a new
 *  		segment inserted after segment 3, containing bytes
 *  		129-256; the previous segment 4 becomes segment 5;
 *  		the previous segment 5 becomes segment 6. An
 *  		diagramm should be added to explain it visually.
 */
extern ret_t mbuf_segment_bisect(mbuf_t *mbuf, uint32_t segment, uint32_t offset);

#endif /* MBUF_H_ */
