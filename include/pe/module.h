#pragma once
#include <phnt_windows.h>
#include <phnt.h>

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace pe
{
  class module : public HINSTANCE__
  {
  public:
    module() = delete;

    template <class T>
    inline auto rva_to(std::uint32_t rva)
    {
      return reinterpret_cast<T *>(reinterpret_cast<std::uint8_t *>(this) + rva);
    }
    template <class T>
    inline auto rva_to(std::uint32_t rva) const
    {
      return reinterpret_cast<const T *>(reinterpret_cast<const std::uint8_t *>(this) + rva);
    }
    std::wstring_view base_name() const;
    std::wstring_view full_name() const;
    struct _IMAGE_DOS_HEADER *dos_header();
    const struct _IMAGE_DOS_HEADER *dos_header() const;
    IMAGE_NT_HEADERS *nt_header();
    const IMAGE_NT_HEADERS *nt_header() const;
    std::size_t size() const;
    std::optional<std::span<class section>> sections();
    std::optional<std::span<const class section>> sections() const;
    class exports *exports();
    const class exports *exports() const;
    class debug *debug();
    const class debug *debug() const;
    void hide_from_module_lists() const;
  };
  pe::module *get_module(const wchar_t *name = nullptr);
  pe::module *get_module_by_prefix(const wchar_t *prefix);
  pe::module *get_module_from_address(void *pc);
  const pe::module *get_module_from_address(const void *pc);
  extern pe::module *instance_module;
}

#include "module.inl"
