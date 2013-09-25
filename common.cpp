///////////////////////////////////////////////////////////////////////////////
// includes
///////////////////////////////////////////////////////////////////////////////

#include "common.h"

#include <stdio.h>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

const std::string to_string(const void *value_p)
{
  char buffer[22 + 1];
  snprintf(buffer, 22, "0x%p", value_p);
  return buffer;
}

const std::string to_string(size_t value)
{
    std::stringstream ss;
    std::string s;
    ss << value;
    s = ss.str();
    return s;
}

const std::string to_string(int value)
{
    std::stringstream ss;
    std::string s;
    ss << value;
    s = ss.str();
    return s;
}

const std::string to_string(uint8_t value)
{
    std::stringstream ss;
    std::string s;
    ss << (int)value;
    s = ss.str();
    return s;
}

const std::string to_string(const uint8_t *data_p, size_t length)
{
  char buffer[(2 * length) + 1];
  for (int counter = 0; counter < length; counter++)
  {
      uint8_t value = *(data_p + counter);
      snprintf(buffer + (2 * counter), 3, "%02X", value);
  }
  return buffer;
}

const std::string to_string(const char *string_p)
{
    std::string value(string_p);
    return value;
}

const std::string to_string(float value)
{
    std::stringstream ss;
    std::string s;
    ss << value;
    s = ss.str();
    return s;
}

///////////////////////////////////////////////////////////////////////////////
// private function implementations 
///////////////////////////////////////////////////////////////////////////////


