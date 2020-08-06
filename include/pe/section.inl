#include <phnt_windows.h>
#include <phnt.h>

#include <cstdint>
#include <string_view>
#include <optional>
#include <span>

#include "module.h"
#include "section.h"

namespace pe
{
  inline std::string_view section::name() const
  {
    std::size_t count;
    for ( count = 0; count < IMAGE_SIZEOF_SHORT_NAME; ++count ) {
      if ( !this->Name[count] )
        break;
    }
    return { reinterpret_cast<const char *>(this->Name), count };
  }

  inline std::uint8_t *section::data()
  {
    if ( this->VirtualAddress ) {
      if ( const auto module = pe::get_module_from_address(this) )
        return module->rva_to<std::uint8_t>(this->VirtualAddress);
    }
    return nullptr;
  }

  inline const std::uint8_t *section::data() const
  {
    if ( this->VirtualAddress ) {
      if ( const auto module = pe::get_module_from_address(this) )
        return module->rva_to<const std::uint8_t>(this->VirtualAddress);
    }
    return nullptr;
  }

  inline std::uint32_t section::size() const
  {
    return this->Misc.VirtualSize;
  }

  inline bool section::contains_code() const
  {
    return this->Characteristics & IMAGE_SCN_CNT_CODE;
  }

  inline bool section::contains_initialized_data() const
  {
    return this->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA;
  }

  inline bool section::contains_uninitialized_data() const
  {
    return this->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA;
  }

  inline std::uint32_t section::relocation_count() const
  {
    if ( (this->Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL)
      && this->NumberOfRelocations == 0xFFFF ) {

      if ( const auto module = pe::get_module_from_address(this) )
        return module->rva_to<const IMAGE_RELOCATION>(this->PointerToRelocations)->RelocCount;
    }
    return this->NumberOfRelocations;
  }

  inline bool section::discardable() const
  {
    return this->Characteristics & IMAGE_SCN_MEM_DISCARDABLE;
  }

  inline bool section::cached() const
  {
    return !(this->Characteristics & IMAGE_SCN_MEM_NOT_CACHED);
  }

  inline bool section::paged() const
  {
    return !(this->Characteristics & IMAGE_SCN_MEM_NOT_PAGED);
  }

  inline bool section::shared() const
  {
    return this->Characteristics & IMAGE_SCN_MEM_SHARED;
  }

  inline bool section::executable() const
  {
    return this->Characteristics & IMAGE_SCN_MEM_EXECUTE;
  }

  inline bool section::readable() const
  {
    return this->Characteristics & IMAGE_SCN_MEM_READ;
  }

  inline bool section::writable() const
  {
    return this->Characteristics & IMAGE_SCN_MEM_WRITE;
  }
}
