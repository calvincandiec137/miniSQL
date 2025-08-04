#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
typedef uint32_t page_num_t;

#define STACK_MAX 256

typedef int64_t Value;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

#endif
