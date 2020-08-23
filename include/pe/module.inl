#pragma once
#include <phnt_windows.h>
#include <phnt.h>

#include <string_view>
#include <span>
#include <optional>
#include <mutex>
#include <cstdint>

#include <ntrtl.hpp>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/win32_helpers.h>

#include "module.h"
#include "section.h"
#include "exports.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

inline std::wstring_view pe::module::base_name() const
{
  auto cs = static_cast<nt::rtl::critical_section *>(NtCurrentPeb()->LoaderLock);
  std::lock_guard<nt::rtl::critical_section> guard(*cs);

  const auto ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
  for ( auto Next = ModuleListHead->Flink; Next != ModuleListHead; Next = Next->Flink ) {
    const auto Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

    if ( Entry->DllBase == this )
      return std::wstring_view(Entry->BaseDllName.Buffer, Entry->BaseDllName.Length / sizeof(wchar_t));
  }
  return {};
}

inline std::wstring_view pe::module::full_name() const
{
  auto cs = static_cast<nt::rtl::critical_section *>(NtCurrentPeb()->LoaderLock);
  std::lock_guard<nt::rtl::critical_section> guard(*cs);

  const auto ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
  for ( auto Next = ModuleListHead->Flink; Next != ModuleListHead; Next = Next->Flink ) {
    const auto Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

    if ( Entry->DllBase == this )
      return std::wstring_view(Entry->FullDllName.Buffer, Entry->FullDllName.Length / sizeof(wchar_t));
  }
  return {};
}

inline struct _IMAGE_DOS_HEADER *pe::module::dos_header()
{
  return reinterpret_cast<struct _IMAGE_DOS_HEADER *>(this);
}

inline const struct _IMAGE_DOS_HEADER *pe::module::dos_header() const
{
  return reinterpret_cast<const struct _IMAGE_DOS_HEADER *>(this);
}

inline IMAGE_NT_HEADERS *pe::module::nt_header()
{
  const auto dosheader = this->dos_header();
  if ( dosheader->e_magic != IMAGE_DOS_SIGNATURE )
    return nullptr;

  const auto ntheader = this->rva_to<IMAGE_NT_HEADERS>(dosheader->e_lfanew);
  if ( ntheader->Signature != IMAGE_NT_SIGNATURE )
    return nullptr;

  return ntheader;
}

inline const IMAGE_NT_HEADERS *pe::module::nt_header() const
{
  return const_cast<module *>(this)->nt_header();
}

inline size_t pe::module::size() const
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

inline std::optional<std::span<pe::section>> pe::module::sections()
{
  const auto ntheader = this->nt_header();
  if ( !ntheader )
    return std::nullopt;

  return std::span{static_cast<section *>(IMAGE_FIRST_SECTION(ntheader)), ntheader->FileHeader.NumberOfSections};
}

inline std::optional<std::span<const pe::section>> pe::module::sections() const
{
  const auto ntheader = this->nt_header();
  if ( !ntheader )
    return std::nullopt;

  return std::span{static_cast<const pe::section *>(IMAGE_FIRST_SECTION(ntheader)), ntheader->FileHeader.NumberOfSections};
}

inline pe::exports *pe::module::exports()
{
  const auto ntheader = this->nt_header();
  if ( !ntheader
      || !ntheader->FileHeader.SizeOfOptionalHeader
      || ntheader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC
      || !ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
      || !ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size )
    return nullptr;
  return this->rva_to<pe::exports>(
    ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
}

inline const pe::exports *pe::module::exports() const
{
  return const_cast<pe::module *>(this)->exports();
}

inline pe::debug *pe::module::debug()
{
  const auto ntheader = this->nt_header();
  if ( !ntheader
      || !ntheader->FileHeader.SizeOfOptionalHeader
      || ntheader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC
      || !ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress
      || !ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size )
    return nullptr;
  return this->rva_to<pe::debug>(
    ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress);
}

inline const pe::debug *pe::module::debug() const
{
  return const_cast<pe::module *>(this)->debug();
}


inline void pe::module::hide_from_module_lists() const
{
  auto cs = static_cast<nt::rtl::critical_section *>(NtCurrentPeb()->LoaderLock);
  std::lock_guard<nt::rtl::critical_section> guard(*cs);
  PPEB_LDR_DATA loaderData = NtCurrentPeb()->Ldr;

  for ( auto Next = loaderData->InLoadOrderModuleList.Flink; Next != &loaderData->InLoadOrderModuleList; Next = Next->Flink ) {
    const auto Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
    if ( Entry->DllBase == this ) {
      RemoveEntryList(Next);
      break;
    }
  }
  for ( auto Next = loaderData->InMemoryOrderModuleList.Flink; Next != &loaderData->InMemoryOrderModuleList; Next = Next->Flink ) {
    const auto Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
    if ( Entry->DllBase == this ) {
      RemoveEntryList(Next);
      break;
    }
  }
  for ( auto Next = loaderData->InInitializationOrderModuleList.Flink; Next != &loaderData->InInitializationOrderModuleList; Next = Next->Flink ) {
    const auto Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
    if ( Entry->DllBase == this ) {
      RemoveEntryList(Next);
      break;
    }
  }
}

inline pe::module *pe::get_module(const wchar_t *name)
{
  if ( !name ) {
    return static_cast<pe::module *>(NtCurrentPeb()->ImageBaseAddress);
  } else {
    auto cs = static_cast<nt::rtl::critical_section *>(NtCurrentPeb()->LoaderLock);
    std::lock_guard<nt::rtl::critical_section> guard(*cs);

    const auto ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    for ( auto Next = ModuleListHead->Flink; Next != ModuleListHead; Next = Next->Flink ) {
      auto Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
      if ( !Entry->InMemoryOrderLinks.Flink )
        continue;

      if ( static_cast<nt::rtl::unicode_string_view *>(&Entry->BaseDllName)->iequals(name) )
        return static_cast<pe::module *>(Entry->DllBase);
    }
  }
  return nullptr;
}

inline pe::module *pe::get_module_by_prefix(const wchar_t *prefix)
{
  auto cs = static_cast<nt::rtl::critical_section *>(NtCurrentPeb()->LoaderLock);
  std::lock_guard<nt::rtl::critical_section> guard(*cs);

  const auto ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
  for ( auto Next = ModuleListHead->Flink; Next != ModuleListHead; Next = Next->Flink ) {
    auto Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
    if ( !Entry->InMemoryOrderLinks.Flink )
      continue;

    if ( static_cast<nt::rtl::unicode_string_view *>(&Entry->BaseDllName)->istarts_with(prefix) )
      return static_cast<pe::module *>(Entry->DllBase);
  }
  return nullptr;
}

inline pe::module *pe::get_module_from_address(void *pc)
{
  if ( pc ) {
    auto cs = static_cast<nt::rtl::critical_section *>(NtCurrentPeb()->LoaderLock);
    std::lock_guard<nt::rtl::critical_section> guard(*cs);

    const auto ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    for ( auto Next = ModuleListHead->Flink; Next != ModuleListHead; Next = Next->Flink ) {
      auto Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
      const auto module = reinterpret_cast<pe::module *>(Entry->DllBase);

      const auto lo = reinterpret_cast<std::uint8_t *>(Entry->DllBase);
      const auto hi = lo + module->nt_header()->OptionalHeader.SizeOfImage;

      if ( pc >= lo && pc <= hi )
        return module;
    }
  }
  return nullptr;
}

inline const pe::module *pe::get_module_from_address(const void *pc)
{
  return pe::get_module_from_address(const_cast<void *>(pc));
}

inline pe::module *pe::instance_module = reinterpret_cast<pe::module *>(&__ImageBase);

