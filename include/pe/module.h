#pragma once
#include <phnt_windows.h>
#include <phnt.h>
#include <string>
#include <gsl/span>

namespace pe
{
  class module : public HINSTANCE__
  {
  public:
    module() = delete;
    uintptr_t handle() const;
    template <class T>
    inline auto rva_to(uint32_t rva)
    {
      return reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(this) + rva);
    }
    template <class T>
    inline auto rva_to(uint32_t rva) const
    {
      return reinterpret_cast<const T *>(reinterpret_cast<const uint8_t *>(this) + rva);
    }
    inline void *rva_to(uint32_t rva)
    {
      return reinterpret_cast<uint8_t *>(this) + rva;
    }
    inline const void *rva_to(uint32_t rva) const
    {
      return reinterpret_cast<const uint8_t *>(this) + rva;
    }
    std::wstring base_name() const;
    std::wstring full_name() const;
    IMAGE_DOS_HEADER *dos_header();
    const IMAGE_DOS_HEADER *dos_header() const;
    IMAGE_NT_HEADERS *nt_header();
    const IMAGE_NT_HEADERS *nt_header() const;
    size_t size() const;
    gsl::span<class segment> segments();
    gsl::span<const class segment> segments() const;
    class segment *segment(const char *namename);
    const class segment *segment(const char *name) const;
    class export_directory *export_directory();
    const class export_directory *export_directory() const;
    void *function(const char *name) const;
    void *function(uint32_t num) const;
  };
  class module *get_module(const wchar_t *name = nullptr);
  class module *get_module_from_address(void *pc);
  const class module *get_module_from_address(const void *pc);
  class module *instance_module();
}

#include "module.inl"
