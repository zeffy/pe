#pragma once
#include <phnt_windows.h>
#include <phnt.h>

#include <cstdint>
#include <span>

namespace pe
{
  class section : public _IMAGE_SECTION_HEADER
  {
  public:
    section() = delete;

    std::string_view name() const;
    std::uint8_t *data();
    const std::uint8_t *data() const;
    std::uint32_t size() const;
    bool contains_code() const;
    bool contains_initialized_data() const;
    bool contains_uninitialized_data() const;
    std::uint32_t relocation_count() const;
    bool discardable() const;
    bool cached() const;
    bool paged() const;
    bool shared() const;
    bool executable() const;
    bool readable() const;
    bool writable() const;
  };
}

#include "section.inl"
