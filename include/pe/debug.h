#pragma once
#include <phnt_windows.h>
#include <phnt.h>

#include <ctime>
#include <cstdint>

namespace pe
{
  class debug : public _IMAGE_DEBUG_DIRECTORY
  {
  public:
    debug() = delete;
    std::time_t timestamp() const;
    std::uint32_t type() const;
  };
}

#include "debug.inl"
