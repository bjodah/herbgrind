#ifndef _HG_RUNTIME
#define _HG_RUNTIME

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
// Pull in this header file so that we can pass memory allocation
// functions to gmp and mpfr.
#include "pub_tool_mallocfree.h"
// Pull in this header file so that we can call the valgrind version
// of printf and dmsg.
#include "pub_tool_libcprint.h"

// Include the rest of the public runtime headers so that the rest of
// the project can just include this one.
#include "hg_shadowop.h"
#include "hg_storage_runtime.h"
#include "hg_mathreplace.h"
#include "hg_op_tracker.h"

// For mpfr_t
#include "mpfr.h"

extern Bool running;

void init_runtime(void);
void cleanup_runtime(void);

// This disables the instrumentation of this tool.
void stopHerbGrind(void);
// This enables the instrumentation of this tool.
void startHerbGrind(void);

// Some memory allocation functions for gmp support
void* gmp_alloc(size_t t);
void* gmp_realloc(void* p, size_t t1, size_t t2);
void gmp_free(void* p, size_t t);

// We wrap these functions because they require slightly different
// types, and we want the coercions to work out.
size_t mpfr_strlen(const char* str);
long int mpfr_strtol(const char* str, char** endptr, int _base);
int mpfr_isspace(int c);
void* mpfr_memmove(void* dest, const void* src, size_t len);
int mpfr_memcmp(const void* ptr1, const void* ptr2, size_t len);
void* mpfr_memset(void* dest, int val, size_t size);
#endif
