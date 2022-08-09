#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../basket.h"

/**
 * @file
 * @details
 * This file demonstrates the usage of basket.
 * A simple basket is created.
 * Into box[0] added a string.
 * Into box[1] added an integer.
 * Then, this basket converted to a buffer.
 * The buffer passed to another function, where a basket object
 * restored, and data read from the boxes. At the end, all
 * memory is released.
 */

#define BASKET_TICKET (0xDEAD1177)

/**
 * @author Sebastian Mountaniol (8/8/22)
 * @brief This is a very primitive function, no return values
 *  	  tested. Please don't use it in production.
 * @return basket_t* Created basked with a couple of boxes
 * @details This function create a basket, then add into box[0]
 *  		a string, and into box[1] a long.
 */
void *create_basket(void)
{
	__attribute__((unused)) ssize_t rc;
	char *a_string;
	long a_long;


	/* Create a basket */
	basket_t *basket = basket_new();

	/* Let's add a string into a box, including the terminating \0 */
	a_string = "This is a string in the first box";
	rc = box_new(basket, a_string, strnlen(a_string, 32) + 1);
	
	/* Let's add an long integer into a box */
	a_long = 25;
	rc = box_new(basket, &a_long, sizeof(a_long));

	/* Set ticket */
	basket_set_ticket(basket, BASKET_TICKET);
	printf("The size of the original basket in memory is: %zu bytes\n", basket_memory_size(basket));
	printf("The size of the data in the basket is: %zu bytes\n", basket_data_size(basket));
	
	return basket;
}

/**
 * @author Sebastian Mountaniol (8/8/22)
 * @brief This function converts the basket object into a 'flat
 *  	  memory buffer' - a regulat continous buffer which can
 *  	  be send or even saved to file and restored later
 * @param const void* basket Baset object
 * @return void* Memory buffer
 * @details 
 */
void *convert_basket_to_buffer(const void *basket)
{
	void *buf;
	size_t size;
	/* We pass the pointer to size: after the function finished, in 'size' reported the final buffer size */
	buf = basket_to_buf(basket, &size);
	printf("Created a 'flat memory buffer', size: %zu\n", size);

	/* Note, we don't pass to caller the size of the buffer. Keep reading! */
	return buf;
}

/**
 * @author Sebastian Mountaniol (8/8/22)
 * @brief THis function gets a memory buffer, restores basket,
 *  	  extarcts values from boxes and prints them.
 * @param void* buf   A flat memory buffer.
 * @return int 0 on success
 * @details 
 */
int restore_basket_extract_values(void *buf)
{
	long * a_long;
	char *a_string;
	basket_t *basket;
	__attribute__((unused)) ssize_t rc;

	/* First, let's validate the the 'flat buffer' is valid */
	if (basket_validate_flat_buffer(buf)) {
		printf("Wrong buffer!\n");
		abort();
	}

	printf("The buffer is a valid basket flat buffer\n");
	printf("The size of the buffer is: %zu bytes\n", basket_get_size_from_flat_buffer(buf));

	/* Restore basket object; we don't have the buffer size, so we put 0 instead, it is valid situation */
	basket = basket_from_buf(buf, 0);

	printf("The size of the restored basket in memory is: %zu bytes\n", basket_memory_size(basket));

	/* Release the flat buffer, we don't need anymore  */
	free(buf);

	/* Get pointer to string from box 0 */
	a_string = box_data_ptr(basket, 0);

	/* Get pointer to long from box 1 */
	a_long = box_data_ptr(basket, 1);

	printf("Extracted from the box[0] and box[1]:\n");
	printf("String: |%s|\n", a_string);
	printf("Long:   |%ld|\n", *a_long);

	/* Release the basket */
	basket_release(basket);
	return 0;
}

int main()
{
	/* Create a simple basket with two boxes */
	void *basket = create_basket();

	/* Convert the basket into a 'flat memory buffer' */
	void *flat_buffer = convert_basket_to_buffer(basket);

	/* Pass the flat memory buffer to function which will validate it, restore the basket object and extract values from boxes */
	restore_basket_extract_values(flat_buffer);

	/* Release the basket */
	basket_release(basket);
}

