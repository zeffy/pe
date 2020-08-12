#pragma once
#include <phnt_windows.h>
#include <phnt.h>

#include <ctime>

#include "exports.h"
#include "module.h"

namespace pe
{
  inline std::time_t exports::timestamp() const
  {
    return this->TimeDateStamp;
  }

  inline const char *exports::name() const
  {
    return pe::get_module_from_address(this)->rva_to<const char>(this->Name);
  }
}
