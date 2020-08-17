#pragma once
#include <phnt_windows.h>
#include <phnt.h>

#include <cstdint>
#include <string_view>
#include <optional>
#include <span>

#include "module.h"
#include "section.h"

inline std::string_view pe::section::name() const
{
  std::size_t count;
  for ( count = 0; count < IMAGE_SIZEOF_SHORT_NAME; ++count ) {
    if ( !this->Name[count] )
      break;
  }
  return { reinterpret_cast<const char *>(this->Name), count };
}

inline std::uint8_t *pe::section::data()
{
  if ( this->VirtualAddress ) {
    if ( const auto module = pe::get_module_from_address(this) )
      return module->rva_to<std::uint8_t>(this->VirtualAddress);
  }
  return nullptr;
}

inline const std::uint8_t *pe::section::data() const
{
  if ( this->VirtualAddress ) {
    if ( const auto module = pe::get_module_from_address(this) )
      return module->rva_to<const std::uint8_t>(this->VirtualAddress);
  }
  return nullptr;
}

inline std::uint32_t pe::section::size() const
{
  return this->Misc.VirtualSize;
}

inline bool pe::section::contains_code() const
{
  return this->Characteristics & IMAGE_SCN_CNT_CODE;
}

inline bool pe::section::contains_initialized_data() const
{
  return this->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA;
}

inline bool pe::section::contains_uninitialized_data() const
{
  return this->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA;
}

inline bool pe::section::contains_data() const
{
  return this->Characteristics & (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_CNT_UNINITIALIZED_DATA);
}

inline std::uint32_t pe::section::relocation_count() const
{
  if ( (this->Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL)
    && this->NumberOfRelocations == 0xFFFF ) {

    if ( const auto module = pe::get_module_from_address(this) )
      return module->rva_to<const IMAGE_RELOCATION>(this->PointerToRelocations)->RelocCount;
  }
  return this->NumberOfRelocations;
}

inline bool pe::section::discardable() const
{
  return this->Characteristics & IMAGE_SCN_MEM_DISCARDABLE;
}

inline bool pe::section::cached() const
{
  return !(this->Characteristics & IMAGE_SCN_MEM_NOT_CACHED);
}

inline bool pe::section::paged() const
{
  return !(this->Characteristics & IMAGE_SCN_MEM_NOT_PAGED);
}

inline bool pe::section::shared() const
{
  return this->Characteristics & IMAGE_SCN_MEM_SHARED;
}

inline bool pe::section::executable() const
{
  return this->Characteristics & IMAGE_SCN_MEM_EXECUTE;
}

inline bool pe::section::readable() const
{
  return this->Characteristics & IMAGE_SCN_MEM_READ;
}

inline bool pe::section::writable() const
{
  return this->Characteristics & IMAGE_SCN_MEM_WRITE;
}
