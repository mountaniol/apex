#ifndef OPTIMIZATION_H_
#define OPTIMIZATION_H_

/* GCC attributes */
/* This file contains "friendly looking" defines of GCC attributes.
 * All these attributes are related to function.
 * Here is a short explanation:
 * FATTR_HOT             - This is a frequently used function
 * FATTR_COLD            - This is a rarely called function
 * FATTR_NONULL(X,Y,Z)   - Specify that the function agruments (X,Y,Z) are not NULL pointers
 * FATTR_NONULL()        - Specify that all function agruments are not NULL pointer
 * FATTR_PURE            - This function not change program global state, no external memory write, and its output purely depends on input
 * FATTR_CONST           - This function is not ever read external memory and its output depends on its input argumnts only
 * FATTR_UNUSED          - This function can be unused. It is fine, don't warn me
 * FATTR_WARN_UNUSED_RET - The return value of this function MUST be used. Warn me if it is not.
 */

/* This function is hot, used intensively */
#define FATTR_HOT __attribute__((hot))
/* This function is cold, not expected to be used intensively */
#define FATTR_COLD __attribute__((cold))

/*
 * Specify that a function agruments at position (1,2,..) of the function expected to be not NULL.
 * It can be used without arguments:
 * ATTR_NONULL() void *box_steal_data(basket_t *basket, const box_u32_t box_num) *
 * Or with arguments:
 *  ATTR_NONULL(1) void *box_steal_data(basket_t *basket, const box_u32_t box_num)
 */
#define FATTR_NONULL(...) __attribute__((nonnull(__VA_ARGS__)))

/* This function never returns a NULL pointer */
#define FATTR_RET_NONULL __attribute__((returns_nonnull))

/* A 'pure' function is a function which does not affect global state of program.
 * I means: no extarnal memory is changed (no writes).
 * Also, a 'pure' function for every same input ALWAYS returns the same output.
 * The 'pure' functions results will be cashed and no calulation will be executed for hashed inputs.
 * See more about it here, and about difference between 'pute' and 'const' functions:
 * https://stackoverflow.com/questions/29117836/attribute-const-vs-attribute-pure-in-gnu-c
   */
#define FATTR_PURE __attribute__((pure))

/* A 'const' function is like 'pure'.
 * It must not access (read or write) the external memory.
 * A 'const' function must return a value based only on parameters.
 * Also, a 'const' function can not be 'void.'
   Results of 'const' function are cashed for performance improvement. */
#define FATTR_CONST __attribute__((const))

/* This function can be unused, and it is OK. Don't print warning on compilation. */
#define FATTR_UNUSED __attribute__((unused))

/*
 * The return value of this function must be used.
 * If the return value is unused, the compiler will warn about it.
 */
#define FATTR_WARN_UNUSED_RET __attribute__((warn_unused_result))


#endif /* OPTIMIZATION_H_ */
