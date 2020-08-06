#pragma once
#include <phnt_windows.h>
#include <phnt.h>

namespace pe
{
  class exports : public _IMAGE_EXPORT_DIRECTORY
  {
  public:
    exports() = delete;
    __time32_t timestamp() const;
    const char *name() const;
  };
}

#include "exports.inl"
