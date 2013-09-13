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

const std::string to_string(const void *value_p);
const std::string to_string(size_t value);
const std::string to_string(int value);
const std::string to_string(uint8_t value);
const std::string to_string(const uint8_t *data_p, size_t length);
const std::string to_string(const char *string_p);
const std::string to_string(float value);

#endif

