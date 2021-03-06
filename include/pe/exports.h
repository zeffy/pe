#pragma once
#include <phnt_windows.h>
#include <phnt.h>

#include <ctime>

namespace pe
{
  class exports : public _IMAGE_EXPORT_DIRECTORY
  {
  public:
    exports() = delete;
    std::time_t timestamp() const;
    const char *name() const;
  };
}

#include "exports.inl"
