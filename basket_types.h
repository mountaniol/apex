#ifndef BASKET_TYPES_H_
#define BASKET_TYPES_H_

#include <sys/types.h>

/*** Size of one box ***/

/* Small buffers: Up to 65535 bytes, means 63 Kb  */
#if defined BOX_16_BITS

typedef uint16_t box_type_t;
#define MAX_VAL_BOX_TYPE (0xFFFF)

#elif defined BOX_32_BITS
#define MAX_VAL_BOX_TYPE (0xFFFFFFFF)

/* Medium buffers: Up to 4,294,967,295 bytes, means 1 Gb  */
typedef uint32_t box_type_t;

#elif defined BOX_64_BITS
#define MAX_VAL_BOX_TYPE (0xFFFFFFFFFFFFFFFF)

/* Large buffers: Up to 18,446,744,073,709,551,615 bytes */
typedef uint64_t box_type_t;

#endif /* BOX_SMALL_BUFFER / BOX_MEDIUM_BUFFER / BOX_LARGE_BUFFER */


/*** Size of a ticket ****/

#if defined (TICKET_8_BITS)
typedef uint8_t ticket_t;
#define MAX_VAL_TICKET_TYPE (0xFF)
#elif defined (TICKET_16_BITS)
#define MAX_VAL_TICKET_TYPE (0xFFFF)
typedef uint16_t ticket_t;
#elif defined (TICKET_32_BITS)
#define MAX_VAL_TICKET_TYPE (0xFFFFFFFF)
typedef uint32_t ticket_t;
#elif defined (TICKET_64_BITS)
#define MAX_VAL_TICKET_TYPE (0xFFFFFFFFFFFFFFFF)
typedef uint64_t ticket_t;
#else
#error You must define size of ticket_t type
#endif /* TICKET_8/16/32/64_BITS */

/*** Number of boxes, bits ***/

#if defined (NUM_BOXES_8_BITS)
typedef uint8_t num_boxes_t;
#define MAX_VAL_NUM_BOXES_TYPE (0xFF)
#elif defined (NUM_BOXES_16_BITS)
typedef uint16_t num_boxes_t;
#define MAX_VAL_NUM_BOXES_TYPE (0xFFFF)
#elif defined (NUM_BOXES_32_BITS)
#define MAX_VAL_NUM_BOXES_TYPE (0xFFFFFFFF)
typedef uint32_t num_boxes_t;
#elif defined (NUM_BOXES_64_BITS)
typedef uint64_t num_boxes_t;
#define MAX_VAL_NUM_BOXES_TYPE (0xFFFFFFFFFFFFFFFF)
#else
#error You must define size of num_boxes_t type
#endif /* TICKET_8/16/32/64_BITS */

/*** Size of watermark, bits ***/

#if defined (WATERMARK_8_BITS)

typedef uint8_t watermark_t;
#define MAX_VAL_WATERMARK_TYPE 	(0xFF)
#define WATERMARK_BASKET 		(0xB9)
#define WATERMARK_BOX 			(0xB3)

#elif defined (WATERMARK_16_BITS)

typedef uint16_t watermark_t;
#define MAX_VAL_WATERMARK_TYPE 	(0xFFFF)
#define WATERMARK_BASKET 		(0xB977)
#define WATERMARK_BOX 			(0xB37F)

#elif defined (WATERMARK_32_BITS)

typedef uint32_t watermark_t;
#define MAX_VAL_WATERMARK_TYPE 	(0xFFFFFFFF)
#define WATERMARK_BASKET 		(0xB977AA35)
#define WATERMARK_BOX 			(0xB37FAH56)

#elif defined (WATERMARK_64_BITS)

typedef uint64_t watermark_t;
#define MAX_VAL_WATERMARK_TYPE 	(0xFFFFFFFFFFFFFFFF)
#define WATERMARK_BASKET 		(0xB977AA35E337137H)
#define WATERMARK_BOX    		(0xB37FAH5648B829CD)

#else
#error You must define size of watermark_t type
#endif /* TICKET_8/16/32/64_BITS */

/*** Size of checksum, bits ***/

#if defined (CHECKSUM_8_BITS)
typedef uint8_t checksum_t;
#define MAX_VAL_CHECKSUM_TYPE (0xFF)
#elif defined (CHECKSUM_16_BITS)
typedef uint16_t checksum_t;
#define MAX_VAL_CHECKSUM_TYPE (0xFFFF)
#elif defined (CHECKSUM_32_BITS)
#define MAX_VAL_CHECKSUM_TYPE (0xFFFFFFFF)
typedef uint32_t checksum_t;
#elif defined (CHECKSUM_64_BITS)
typedef uint64_t checksum_t;
#define MAX_VAL_CHECKSUM_TYPE (0xFFFFFFFFFFFFFFFF)
#else
#error You must define size of checksum_t type
#error Use one of defines: CHECKSUM_8_BITS / CHECKSUM_16_BITS / CHECKSUM_32_BITS / CHECKSUM_64_BITS
#endif /* TICKET_8/16/32/64_BITS */


#endif /* BASKET_TYPES_H_ */
