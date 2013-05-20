#ifndef _COMMON_H_
#define _COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <assert.h>

#include <string>


#define ASSERT(x) assert(x)

typedef enum
{
    RESULT_CODE_OK = 0,
    RESULT_CODE_ERROR,
} ResultCode;

std::string to_string(int value);
std::string to_string(uint8_t value);

#endif

