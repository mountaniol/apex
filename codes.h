#ifndef CODES_H_
#define CODES_H_

enum yes_no {
	NO = 0,
	YES = 1,
};

typedef enum status_struct {
	A_AGAIN = -2, /**< Try again later */
	A_ERROR = -1, /**< Error, can not contunue the operation */
	A_OK = 0, /**< Operation completed successfully */
} ret_code_t;

#endif /* CODES_H_ */
