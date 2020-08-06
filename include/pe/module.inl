#include <phnt_windows.h>
#include <phnt.h>

#include <wil/result.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>

#include <cstdint>
#include <string_view>
#include <optional>
#include <span>
#include <mutex>

#include "module.h"
#include "segment.h"
#include "exports.h"
#include <rtl.hpp>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace pe
{
  inline std::wstring_view module::base_name() const
  {
    auto crit = static_cast<nt::rtl::critical_section *>(NtCurrentPeb()->LoaderLock);
    std::lock_guard<nt::rtl::critical_section> guard(*crit);

    auto const Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    for ( auto Entry = Head->Flink; Entry != Head; Entry = Entry->Flink ) {
      auto Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

      if ( Module->DllBase == this )
        return std::wstring_view(Module->BaseDllName.Buffer, Module->BaseDllName.Length / sizeof(wchar_t));
    }
    return {};
  }

  inline std::wstring_view module::full_name() const
  {
    auto crit = static_cast<nt::rtl::critical_section *>(NtCurrentPeb()->LoaderLock);
    std::lock_guard<nt::rtl::critical_section> guard(*crit);

    const auto Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    for ( auto Entry = Head->Flink; Entry != Head; Entry = Entry->Flink ) {
      auto Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

      if ( Module->DllBase == this )
        return std::wstring_view(Module->FullDllName.Buffer, Module->FullDllName.Length / sizeof(wchar_t));
    }
    return {};
  }

  inline struct _IMAGE_DOS_HEADER *module::dos_header()
  {
    return reinterpret_cast<IMAGE_DOS_HEADER *>(this);
  }

  inline const struct _IMAGE_DOS_HEADER *module::dos_header() const
  {
    return reinterpret_cast<const struct _IMAGE_DOS_HEADER *>(this);
  }

  inline struct _IMAGE_NT_HEADERS *module::nt_header()
  {
    const auto dosheader = this->dos_header();
    if ( dosheader->e_magic != IMAGE_DOS_SIGNATURE )
      return nullptr;

    const auto ntheader = this->rva_to<struct _IMAGE_NT_HEADERS>(dosheader->e_lfanew);
    if ( ntheader->Signature != IMAGE_NT_SIGNATURE )
      return nullptr;

    return ntheader;
  }

  inline const struct _IMAGE_NT_HEADERS *module::nt_header() const
  {
    return const_cast<module *>(this)->nt_header();
  }

  inline size_t module::size() const
  {
    const auto dosheader = this->dos_header();
    if ( dosheader->e_magic != IMAGE_DOS_SIGNATURE )
      return 0;

    const auto ntheader = this->nt_header();
    if ( !ntheader
      || !ntheader->FileHeader.SizeOfOptionalHeader
      || ntheader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
      return 0;

    return ntheader->OptionalHeader.SizeOfImage;
  }

  inline std::optional<std::span<pe::section>> module::sections()
  {
    const auto ntheader = this->nt_header();
    if ( !ntheader )
      return std::nullopt;

    return std::span { static_cast<pe::section *>(IMAGE_FIRST_SECTION(ntheader)), ntheader->FileHeader.NumberOfSections };
  }

  inline std::optional<std::span<const pe::section>> module::sections() const
  {
    const auto ntheader = this->nt_header();
    if ( !ntheader )
      return std::nullopt;

    return std::span { static_cast<const pe::section *>(IMAGE_FIRST_SECTION(ntheader)), ntheader->FileHeader.NumberOfSections };
  }

  inline class pe::exports *module::exports()
  {
    const auto ntheader = this->nt_header();
    if ( !ntheader
      || !ntheader->FileHeader.SizeOfOptionalHeader
      || ntheader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC
      || !ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
      || !ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size )
      return nullptr;

    return this->rva_to<class pe::exports>(
      ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
  }

  inline const class pe::exports *module::exports() const
  {
    return const_cast<pe::module *>(this)->exports();
  }

  inline void module::hide_from_module_lists() const
  {
    auto loaderLock = static_cast<nt::rtl::critical_section *>(NtCurrentPeb()->LoaderLock);
    std::lock_guard<nt::rtl::critical_section> guard(*loaderLock);
    PPEB_LDR_DATA loaderData = NtCurrentPeb()->Ldr;

    for ( auto Entry = loaderData->InLoadOrderModuleList.Flink; Entry != &loaderData->InLoadOrderModuleList; Entry = Entry->Flink ) {
      auto Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
      if ( Module->DllBase == this ) {
        RemoveEntryList(Entry);
        break;
      }
    }
    for ( auto Entry = loaderData->InMemoryOrderModuleList.Flink; Entry != &loaderData->InMemoryOrderModuleList; Entry = Entry->Flink ) {
      auto Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
      if ( Module->DllBase == this ) {
        RemoveEntryList(Entry);
        break;
      }
    }
    for ( auto Entry = loaderData->InInitializationOrderModuleList.Flink; Entry != &loaderData->InInitializationOrderModuleList; Entry = Entry->Flink ) {
      auto Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
      if ( Module->DllBase == this ) {
        RemoveEntryList(Entry);
        break;
      }
    }
  }

  inline pe::module *get_module(wchar_t const *name)
  {
    if ( !name ) {
      return static_cast<pe::module *>(NtCurrentPeb()->ImageBaseAddress);
    } else {
      auto crit = static_cast<nt::rtl::critical_section *>(NtCurrentPeb()->LoaderLock);
      std::lock_guard<nt::rtl::critical_section> guard(*crit);

      auto const Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
      for ( auto Entry = Head->Flink; Entry != Head; Entry = Entry->Flink ) {
        auto Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        if ( !Module->InMemoryOrderLinks.Flink )
          continue;

        if ( static_cast<nt::rtl::unicode_string_view *>(&Module->BaseDllName)->iequals(name) )
          return static_cast<pe::module *>(Module->DllBase);
      }
    }
    return nullptr;
  }

  inline pe::module *get_module_from_address(void *pc)
  {
    void *Unused;
    return static_cast<pe::module *>(RtlPcToFileHeader(pc, &Unused));
  }

  inline const pe::module *get_module_from_address(const void *pc)
  {
    return pe::get_module_from_address(const_cast<void *>(pc));
  }

  inline pe::module *instance_module()
  {
    return reinterpret_cast<pe::module *>(&__ImageBase);
  }
}
