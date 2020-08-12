#pragma once
#include <phnt_windows.h>
#include <phnt.h>

#include <ctime>
#include <cstdint>

#include "debug.h"

namespace pe
{
  inline std::time_t debug::timestamp() const
  {
    return this->TimeDateStamp;
  }

  inline std::uint32_t debug::type() const
  {
    return this->Type;
  }
}
