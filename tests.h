#ifndef BUF_T_DEFINES_H_
#define BUF_T_DEFINES_H_

#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "debug.h"

#define MAX(a,b) ((a > b) ? a : b)
#define MIN(a,b) ((a < b) ? a : b)

/* ABORT MACROS */

/* If the first argument of the macro is NULL the macro prints a message and aborts execution */
#define TESTP_ASSERT(x, mes) do {if(NULL == x) { DE("[[ ASSERT! ]] %s == NULL: %s\n", #x, mes); abort(); } } while(0)

/* We can manage this macro by enable / disable ABORT_ON_ERROR compilation flag;
 * When it defined, this macro will abort execution; 
 * when it not defined, it will just printout an error message */
#if defined ABORT_ON_ERROR
#define TRY_ABORT() do{ {DE("Try/Abort: Abort ensabled in %s +%d\n", __FILE__, __LINE__); abort();} } while(0)
#else /* ABORT_ON_ERROR */
#define TRY_ABORT() do{  DE("Try/Abort: Abort disabled in %s +%d\n", __FILE__, __LINE__); } while(0)
#endif /* ABORT_ON_ERROR */

#define ABORT_OR_RETURN(x) do{TRY_ABORT(); return x;} while(0)
#define ABORT_OR_RETURN_VOID() do{TRY_ABORT(); return;} while(0)

/* This macro prints message and abort execution */
#define MES_ABORT(x) do { DDE("Abort: %s\n", #x); abort(); } while(0)
#define TESTP_ABORT(x) do { if(NULL == x) {DDE("Abort on illegal NULL: %s\n", #x); abort(); }} while(0)
								  

#define TESTP_MES(x, ret, mes) do {if(NULL == x) { DE("%s\n", mes); return ret; } } while(0)
#define TESTP_VOID_MES(x, mes) do {if(NULL == x) { DE("%s\n", mes); return; } } while(0)
#define TESTP(x, ret) do {if(NULL == x) { DDE("Pointer %s is NULL\n", #x); return ret; }} while(0)
#define TESTP_VOID(x) do {if(NULL == x) { DDE("Pointer %s is NULL\n", #x); return; }} while(0)
#define TESTP_GO(x, lable) do {if(NULL == x) { DE("Pointer %s is NULL\n", #x); goto lable; } } while(0)
#define TFREE_SIZE(x,sz) do { if(NULL != x) {memset(x,0,sz);free(x); x = NULL;} else {DE(">>>>>>>> Tried to free_size() NULL: %s (%s +%d)\n", #x, __func__, __LINE__);} }while(0)


/* This used to test and print time of execution in the same function */
#define TEST_TIME_START(x) clock_t x##_start = clock();
#define TEST_TIME_PRINT_START(x) do{D("Line %d: start time is %f seconds (%f)\n", __LINE__, (double) (x##_start / CLOCKS_PER_SEC), x##_start);}while(0);
#define TEST_TIME_END(x) {clock_t x##_end = clock(); D("Line %d: time used %f seconds\n", __LINE__, (((double)(x##_end - x##_start))/ CLOCKS_PER_SEC));}

#endif /* BUF_T_DEFINES_H_ */
