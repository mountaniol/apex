#ifndef APEX_H_
#define APEX_H_

#include <stdint.h>
#include "list.h"
#include "zhash3.h"

/* This macro defines how many file descriptors are allocated each time
   we need to grow it */
#define FD_CHUNK_SIZE 1024

/* The max length of file descriptor */
#define APEX_FD_NAME_MAX_LEN 256

/* Max length of an apex name */
#define APEX_NAME_MAX_LEN 256

/* The apex name_id type */
typedef uint32_t apex_name_id;

typedef struct apex_name_struct {
	/* The string name of an apex */
	char name[APEX_NAME_MAX_LEN];
	/* The hash generated from the name */
	apex_name_id name_id;
} apex_name_t;

enum apex_dst_enum {
	/* This means 'for you only' - receiver should use it and drop */
	APEX_DST_YOU = 0x0,
	/* This means 'get it and forward to everyone' */
	APEX_DST_ALL = 0xFFFFFFFF
};

/* This structure supposed to be transfered between
   different machines. They can be 32 or 64 bits, we don't know */
typedef struct __attribute__((__packed__))apex_header_buffer {
	/* The destination apex ID */
	uint32_t dst;
	/* Following buffer size */
	uint32_t size;
} apex_header_t;

/* This enum defines APEX control packet types */
enum apex_cmd_enum {
	APEX_CTR_APEX_CONNECTED = 1,
	APEX_CTR_APEX_DISCONNECTED = 2,
};

/* This structure contains control information
   and transfered from apex to apex */
typedef struct __attribute__((__packed__))apex_control_buffer {
	/* The destination apex ID */
	uint32_t cmd;
	/* Following buffer size */
	uint32_t size;
} apex_ctl_buf_t;

/* The read function - read from file descriptor */
typedef ssize_t (*apex_read_func_t)(int fd_data, void *data, size_t cnt);

/* Write function - write data to a file descriptor */
typedef ssize_t (*apex_write_func_t)(int fd_data, const void *data, size_t cnt);

/* Generate an event on a given file descriptor of event stream */
typedef ssize_t (*apex_set_event_func_t)(int fd_data);

typedef struct apex_fd_struct {
	/* Data file descriptor */
	int fd_data;
	/* Event file descriptor, can be -1 */
	int fd_event;
	/* The outgoing buffer. can be NULL if there is no aggregation */
	char *fd_buf;
	/* The size of the buffer; 0 if the buffer is not used */
	size_t fd_buf_size;
	/* How many bytes to aggregate before it seent? */
	size_t fd_aggregate_bytes;
	/* How many MicroSeconds it should aggregate packets before send it out */
	size_t fd_aggregate_usec;

	/* Used to read data from the file descriptor; by defailt it is read(2) */
	apex_read_func_t read_data;
	/* Used to write data to the file descriptor; by defailt it is write(2) */
	apex_write_func_t write_data;
	/* Used to generate an event of event descriptor; can be NULL, otherwise used must define it */
	apex_set_event_func_t generate_event;

	/* Read and write functions */
	/* The file descriptor name, for debug, information and statistics */
	char fd_name[APEX_FD_NAME_MAX_LEN];
} apex_fd_t;

/* */

typedef struct apex_struct {

	/*** THE APEX CORE ***/

	/*** LEFT SIDE ***/

	/* Array of file descriptors:
	 * Every such a structure contains:
	 * - The file descriptor,
	 * - The event descriptor,
	 * - Associated buffer,
	 * - Associated buffer size,
	 * - The connection na,e (for information and statistics only)
	 */
	apex_fd_t fd_l;
	/* Number of the left descriptors: we can grow this array if we need */
	unsigned int fd_count_l;

	/* Routing table, left side */
	ztable_t *rtable_l;

	/* The incoming list */
	EXA_list_ctl_t *in_list_l;


	/*** RIGHT SIDE ***/

	/* Array of file descriptors */
	apex_fd_t fd_r;
	/* Number of the right descriptors */
	unsigned int fd_count_r;

	/* Routing table, left side */
	ztable_t *rtable_r;

	/* The incoming list */
	EXA_list_ctl_t *in_list_r;


} apex_t;

#endif /* APEX_H_ */
